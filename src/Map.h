#pragma once

#include "platform.h"

#include <XPLMMap.h>

namespace MapConstants {
    static constexpr int MAX_MAP_POINTS = 10000;
    static constexpr int MAX_WAYPOINTS = 255;
}

class Map
{
public:

  Map();
  ~Map();

private:

  typedef struct
  {
    float lat;
    float lon;
    uint8_t flags;  //1-action waypoint
  } TCoords;

  XPLMMapLayerID layer = NULL;

  float crossLat = 0;
  float crossLon = 0;

  TCoords waypoints[MapConstants::MAX_WAYPOINTS];
  int waypointsCount = 0;

  int waypointsDownloadState = -1;

                                    
  void createOurMapLayer(const char * mapIdentifier, void * refcon);
  static void createOurMapLayerStatic(const char * mapIdentifier, void * refcon);
  static void willBeDeletedStatic(XPLMMapLayerID layer, void * inRefcon);
  void drawMarkings(XPLMMapLayerID layer, const float * inMapBoundsLeftTopRightBottom, float zoomRatio, float mapUnitsPerUserInterfaceUnit, XPLMMapStyle mapStyle, XPLMMapProjectionID projection, void * inRefcon);
  static void drawMarkingsStatic(XPLMMapLayerID layer, const float * inMapBoundsLeftTopRightBottom, float zoomRatio, float mapUnitsPerUserInterfaceUnit, XPLMMapStyle mapStyle, XPLMMapProjectionID projection, void * inRefcon);

  void addPoint(float lat, float lon);
  void addPointEx(float lat, float lon);

  void retrieveNextWaypoint();


  void clearTracks();
  void startDownloadWaypoints();
  void onWPInfo(const TMSPWPInfo& messageBuffer);
  void onWP(const TMSPWP& messageBuffer);

  void teleport();
};

