#include "platform.h"

#include <math.h>
#include <string>
#include <regex>
#include <cstring>

#include <XPLMMap.h>
#include <XPLMGraphics.h>
#include <XPLMScenery.h>

#include "core/PluginContext.h"
#include "core/EventBus.h"
#include "settings/SettingNames.h"

#include "Map.h"
#include "Utils.h"


namespace MapConstants {
    static constexpr float INAV_LAT_LON_SCALE = 10000000.0f;
    static constexpr float DISTANCE_BETWEEN_TWO_LONGITUDE_POINTS_AT_EQUATOR = 1.113195f; // inav lon to cm
    static constexpr float LAT_LON_DIFF = 2000.0f / DISTANCE_BETWEEN_TWO_LONGITUDE_POINTS_AT_EQUATOR / INAV_LAT_LON_SCALE;
}

Map::Map()
{
    if (XPLMMapExists(XPLM_MAP_USER_INTERFACE))
    {
        createOurMapLayer(XPLM_MAP_USER_INTERFACE, NULL);
    }

    // Listen for any new map objects that get created
    XPLMRegisterMapCreationHook(Map::createOurMapLayerStatic, this);

    auto eventBus = Plugin()->GetEventBus();

    eventBus->Subscribe("MenuMapDownloadWaypoints", [this]()
    {
        this->startDownloadWaypoints();
    });

    eventBus->Subscribe("MenuMapTeleport", [this]()
    {
        this->teleport();
    });

    eventBus->Subscribe<MSPMessageEventArg>("MSPMessage", [this](const MSPMessageEventArg &event)
    {
        TMSPWPInfo wpInfo; 
        TMSPWP wp;
        if (event.command == MSP_WP_GETINFO && event.messageBuffer.size() >= sizeof(wpInfo))
        {
            std::memcpy(&wpInfo, event.messageBuffer.data(), sizeof(wpInfo));
            this->onWPInfo(wpInfo);            
        }
        else if (event.command == MSP_WP && event.messageBuffer.size() >= sizeof(wp))
        {
            std::memcpy(&wp, event.messageBuffer.data(), sizeof(wp));
            this->onWP(wp);
        }
    });
}

Map::~Map()
{
    XPLMDestroyMapLayer(this->layer);
}

void Map::createOurMapLayerStatic(const char *mapIdentifier, void *refcon)
{
    if (refcon != nullptr)
    {
        static_cast<Map *>(refcon)->createOurMapLayer(mapIdentifier, refcon);
    }
}

void Map::createOurMapLayer(const char *mapIdentifier, void *refcon)
{
    if (!this->layer &&                                  // Confirm we haven't created our markings layer yet (e.g., as a result of a previous callback), or if we did, it's been destroyed
        !strcmp(mapIdentifier, XPLM_MAP_USER_INTERFACE)) // we only want to create a layer in the normal user interface map (not the IOS)
    {
        XPLMCreateMapLayer_t params;
        params.structSize = sizeof(XPLMCreateMapLayer_t);
        params.mapToCreateLayerIn = XPLM_MAP_USER_INTERFACE;
        params.willBeDeletedCallback = &Map::willBeDeletedStatic;
        params.prepCacheCallback = NULL;
        params.showUiToggle = 1;
        params.refcon = this;
        params.layerType = xplm_MapLayer_Markings;
        params.drawCallback = &Map::drawMarkingsStatic;
        params.iconCallback = NULL;
        params.labelCallback = NULL;
        params.layerName = "INAV XITL";
        // Note: this could fail (return NULL) if we hadn't already confirmed that params.mapToCreateLayerIn exists in X-Plane already
        this->layer = XPLMCreateMapLayer(&params);
    }
}

void Map::willBeDeletedStatic(XPLMMapLayerID layer, void *inRefcon)
{
    if (inRefcon != nullptr)
    {
        Map *map = static_cast<Map *>(inRefcon);
        if (map->layer == layer)
        {
            map->layer = NULL;
        }
    }
}

void Map::drawMarkingsStatic(XPLMMapLayerID layer, const float *inMapBoundsLeftTopRightBottom, float zoomRatio, float mapUnitsPerUserInterfaceUnit, XPLMMapStyle mapStyle, XPLMMapProjectionID projection, void *inRefcon)
{
    if (inRefcon != nullptr)
    {
        Map *map = static_cast<Map *>(inRefcon);
        map->drawMarkings(layer, inMapBoundsLeftTopRightBottom, zoomRatio, mapUnitsPerUserInterfaceUnit, mapStyle, projection, inRefcon);
    }
}

void Map::drawMarkings(XPLMMapLayerID layer, const float *inMapBoundsLeftTopRightBottom, float zoomRatio, float mapUnitsPerUserInterfaceUnit, XPLMMapStyle mapStyle, XPLMMapProjectionID projection, void *inRefcon)
{
    XPLMSetGraphicsState(
        0 /* no fog */,
        0 /* 0 texture units */,
        0 /* no lighting */,
        0 /* no alpha testing */,
        1 /* do alpha blend */,
        1 /* do depth testing */,
        0 /* no depth writing */
    );

    float x;
    float y;

    if (this->waypointsCount > 1)
    {
        glColor3f(1, 0, 1);
        glBegin(GL_LINE_STRIP);
        for (int i = 0; i < this->waypointsCount; i++)
        {
            if (waypoints[i].flags & 1)
                continue;
            XPLMMapProject(projection, waypoints[i].lat, waypoints[i].lon, &x, &y);
            glVertex2f(x, y);
        }
        glEnd();

        for (int i = 0; i < this->waypointsCount; i++)
        {
            if (waypoints[i].flags & 1)
                continue;
            const float width = XPLMMapScaleMeter(projection, this->crossLat, this->crossLon) * 10;
            XPLMMapProject(projection, waypoints[i].lat, waypoints[i].lon, &x, &y);
            glBegin(GL_LINE_LOOP);
            glVertex2f(x - width, y - width);
            glVertex2f(x - width, y + width);
            glVertex2f(x + width, y + width);
            glVertex2f(x + width, y - width);
            glEnd();
        }
    }
}


void Map::startDownloadWaypoints()
{
    this->waypointsDownloadState = 0;
    this->waypointsCount = 0;

    Plugin()->GetEventBus()->Publish<MSPMessageEventArg>("SendMSPMessage", MSPMessageEventArg(MSP_WP_GETINFO));
}

void Map::onWPInfo(const TMSPWPInfo& messageBuffer)
{
    Utils::LOG("Got WP Info command, valid = {}, count = {}", messageBuffer.waypointsListValid, messageBuffer.waypointsCount);

    if (this->waypointsDownloadState != 0)
        return;
    if (!messageBuffer.maxWaypoints || (messageBuffer.waypointsCount == 0))
    {
        Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Waypoints download", "No waypoints defined", 3000));
    }
    else
    {
        this->waypointsDownloadState = 1;
        this->waypointsCount = messageBuffer.waypointsCount;

        for (int i = 0; i < this->waypointsCount; i++)
        {
            this->waypoints[i].lat = 0;
            this->waypoints[i].lon = 0;
        }

        this->retrieveNextWaypoint();
    }
}

void Map::onWP(const TMSPWP& messageBuffer)
{
    Utils::LOG("Got WP command, index = {}", messageBuffer.index);

    if ((messageBuffer.index < 1) || (messageBuffer.index > this->waypointsCount))
        return;

    this->waypoints[messageBuffer.index - 1].lat = messageBuffer.lat / MapConstants::INAV_LAT_LON_SCALE;
    this->waypoints[messageBuffer.index - 1].lon = messageBuffer.lon / MapConstants::INAV_LAT_LON_SCALE;
    this->waypoints[messageBuffer.index - 1].flags = messageBuffer.flags;
    this->waypointsDownloadState++;
    if (this->waypointsDownloadState <= this->waypointsCount)
    {
        this->retrieveNextWaypoint();
    }
    else
    {
        std::string s = "Downloaded " + std::to_string(this->waypointsCount) + " waypoints";
        Plugin()->GetEventBus()->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Waypoints download", s, 3000));
    }
}

void Map::retrieveNextWaypoint()
{
    Plugin()->GetEventBus()->Publish<MSPMessageEventArg>("SendMSPMessage", MSPMessageEventArg(MSP_WP, std::vector<uint8_t>{static_cast<uint8_t>(this->waypointsDownloadState)}));
}

void Map::teleport()
{
    auto eventBus = Plugin()->GetEventBus();

    std::string str = Utils::GetClipboardText();
    
    if (str.empty())
    {
        eventBus->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Teleport failed", "No coords in clipboard", 3000));
        Utils::LOG("TELEPORT: Clipboard is empty");
        return;
    }

    // Regex pattern to match coordinates: optional minus, digits, optional decimal point and digits
    // Matches formats like: 51.5074,-0.1278 or 51.5074, -0.1278 or 51.5074; -0.1278 etc.
    std::regex coordPattern(R"((-?\d+\.?\d*)\s*[,;]\s*(-?\d+\.?\d*))");
    std::smatch matches;

    if (!std::regex_search(str, matches, coordPattern) || matches.size() < 3)
    {
        eventBus->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Teleport failed", "Unable to parse coords", 3000));
        Utils::LOG("TELEPORT: Could not parse coordinates from: {}", str);
        return;
    }

    double latitude;
    double longitude;
    
    try
    {
        latitude = std::stod(matches[1].str());
        longitude = std::stod(matches[2].str());
    }
    catch (const std::exception& e)
    {
        eventBus->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Teleport failed", "Invalid coordinate values", 3000));
        Utils::LOG("TELEPORT: Exception parsing coordinates: {}", e.what());
        return;
    }

    if (isnan(latitude) || isnan(longitude) || latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0)
    {
        eventBus->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Teleport failed", "Invalid coordinate ranges", 3000));
        Utils::LOG("TELEPORT: Invalid coordinates - lat: {}, lon: {}", latitude, longitude);
        return;
    }

    // Find current plane position above terrain
    XPLMDataRef df_lattitude = XPLMFindDataRef("sim/flightmodel/position/latitude");
    XPLMDataRef df_longitude = XPLMFindDataRef("sim/flightmodel/position/longitude");
    XPLMDataRef df_elevation = XPLMFindDataRef("sim/flightmodel/position/elevation");

    double plane_latitude = XPLMGetDatad(df_lattitude);
    double plane_longitude = XPLMGetDatad(df_longitude);
    double plane_elevation = XPLMGetDatad(df_elevation);

    double xLocal;
    double yLocal;
    double zLocal;

    XPLMWorldToLocal(plane_latitude, plane_longitude, plane_elevation, &xLocal, &yLocal, &zLocal);

    XPLMProbeRef probeRef = XPLMCreateProbe(xplm_ProbeY);

    XPLMProbeInfo_t info;
    info.structSize = sizeof(XPLMProbeInfo_t);
    XPLMProbeResult status = XPLMProbeTerrainXYZ(probeRef, (float)xLocal, (float)yLocal, (float)zLocal, &info);

    if (status != xplm_ProbeHitTerrain)
    {
        XPLMDestroyProbe(probeRef);
        eventBus->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Teleport failed", "Unable to probe height", 3000));
        Utils::LOG("TELEPORT: Unable to find terrain height at target point");
        return;
    }

    double aboveTerrain = yLocal - info.locationY + 0.1; //+10cm

    if (aboveTerrain < 0)
        aboveTerrain = 100;
    if (aboveTerrain > 2000)
        aboveTerrain = 2000;

    // place plane to new coords
    XPLMWorldToLocal(latitude, longitude, 0, &xLocal, &yLocal, &zLocal);

    info.structSize = sizeof(XPLMProbeInfo_t);
    status = XPLMProbeTerrainXYZ(probeRef, (float)xLocal, (float)yLocal, (float)zLocal, &info);

    if (status != xplm_ProbeHitTerrain)
    {
        XPLMDestroyProbe(probeRef);
        eventBus->Publish<OsdToastEventArg>("MakeToast", OsdToastEventArg("Teleport failed", "Unable to probe height", 3000));
        Utils::LOG("TELEPORT: Unable to find terrain height at target point");
        return;
    }

    XPLMDataRef df_local_x;
    XPLMDataRef df_local_y;
    XPLMDataRef df_local_z;

    df_local_x = XPLMFindDataRef("sim/flightmodel/position/local_x");
    df_local_y = XPLMFindDataRef("sim/flightmodel/position/local_y");
    df_local_z = XPLMFindDataRef("sim/flightmodel/position/local_z");

    xLocal = info.locationX;
    yLocal = info.locationY + aboveTerrain;
    zLocal = info.locationZ;

    XPLMSetDatad(df_local_x, xLocal);
    XPLMSetDatad(df_local_y, yLocal);
    XPLMSetDatad(df_local_z, zLocal);

    XPLMLocalToWorld(xLocal, yLocal, zLocal, &plane_latitude, &plane_longitude, &plane_elevation);

    eventBus->Publish<Double3DPointEventArg>("UpdateHomeLocation", Double3DPointEventArg(plane_latitude, plane_longitude, plane_elevation));

    XPLMDestroyProbe(probeRef);
}
