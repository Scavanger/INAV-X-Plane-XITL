
#include "platform.h"

#include <math.h>
#include <cstring>
#include <XPLMDisplay.h>

#include "core/PluginContext.h"
#include "core/EventBus.h"
#include "settings/SettingNames.h"

#include "OSD.h"
#include "Utils.h"

#include "fonts/Fonts.h"

namespace OSDConstants {
    static constexpr int OSD_MARGIN_PERCENT = 1;

    static constexpr int NOISE_TEXTURE_WIDTH = 1024;
    static constexpr int NOISE_TEXTURE_HEIGHT = 1024;

    static constexpr int INERFERENCE_TEXTURE_WIDTH = 1024;
    static constexpr int INERFERENCE_TEXTURE_HEIGHT = 128;

    static constexpr int OSD_MARGIN = 30;

    typedef enum {
        DP_SUB_CMD_CLEAR_SCREEN = 2,
        DP_SUB_CMD_WRITE_STRING = 3,
        DP_SUB_CMD_DRAW_SCREEN = 4,
        DP_SUB_CMD_SET_OPTIONS = 5
    } mspDisplayportSubCmd_t;
}

OSD::OSD()
{
    auto eventBus = Plugin()->GetEventBus();

    this->osdData.assign(OSD_MAX_ROWS * OSD_MAX_COLS, 0);
    this->toastData.assign((OSDConstants::TOAST_MAX_ROWS + 2) * (OSDConstants::TOAST_MAX_COLS + 2), 0);

    Plugin()->Fonts()->setFontType(OsdType::WtfOS);

    if (glewInit() != GLEW_OK)
    {
        Utils::LOG("Unable to init GLEW");
    }

    this->osdRenderer = std::make_unique<OsdRenderer>();

    fs::path assetFileName = Utils::GetPluginDirectory() / "assets" / "noise.png";
    int id = this->osdRenderer->loadInterferenceTexture(assetFileName, true);
    if (id >= 0)
    {
        this->noiseTexture = id;
    }
    else
    {
        Utils::LOG("Failed to load noise texture from {}", assetFileName.string());
    }

    assetFileName = Utils::GetPluginDirectory() / "assets" / "interference.png";
    id = this->osdRenderer->loadInterferenceTexture(assetFileName, true);
    if (id >= 0)
    {
        this->interferenceTexture = id;
    }
    else
    {
        Utils::LOG("Failed to load interference texture from {}", assetFileName.string());
    }

    eventBus->Subscribe<Double3DPointEventArg>("UpdateHomeLocation", [this](const Double3DPointEventArg &event)
    {
        this->home_lattitude = event.latitude;
        this->home_longitude = event.longitude;
        this->home_elevation = event.altitude;
    }); 

    eventBus->Subscribe<Double3DPointEventArg>("UpdatePosition", [this](const Double3DPointEventArg &event)
    {
        this->current_lattitude = event.latitude;
        this->current_longitude = event.longitude;
        this->current_elevation = event.altitude;
    });

    eventBus->Subscribe<FloatEventArg>("UpdateRoll", [this](const FloatEventArg &event)
    {
        this->roll = event.value;
    });

    eventBus->Subscribe<MSPMessageEventArg>("MSPMessage", [this](const MSPMessageEventArg &event)
    {
        TMSPSimulatorFromINAV simData;
        if (event.command != MSP_SIMULATOR || event.messageBuffer.size() < MSPConstants::MSP_SIMULATOR_RESPOSE_MIN_LENGTH || event.messageBuffer.size() > sizeof(simData))
        {
            return;
        }

        std::memcpy(&simData, event.messageBuffer.data(), event.messageBuffer.size());
        this->updateFromINAV(simData.osdData);
    });

   updateFont();

#ifdef DEBUG_BUILD
    eventBus->Subscribe("MenuDebugDrawTestOSD", [this]()
    {
        auto font = Plugin()->Fonts()->GetCurrentFont();
        if (!font) {
            return;
        }
        for (int i = 0; i < font->getRows() * font->getCols(); i++)
        {
            this->osdData[i] = OSDConstants::makeCharMode(i % 512, 0);
        }
    });

    eventBus->Subscribe("MenuDebugClearOSD", [this]()
    {
        this->clear();
    });
#endif
    
    eventBus->Subscribe("AirportLoaded", [this]()
    {   
        if (!this->isConnected) {
             this->makeToast("INAV-X-Plane-XITL", XITL_VERSION_STRING, 10000);
        }
        
        Utils::SetView();
    });
    
    eventBus->Subscribe("FontChanged",[this]()
    {
        this->updateFont();
    }); 

    eventBus->Subscribe<DrawCallbackEventArg>(
        "DrawCallback", 
        [this](const DrawCallbackEventArg &event)
        {
            this->drawOSD();
            OsdType currentOsdType = Plugin()->Fonts()->getCurrentFontType();
            if (this->videoLink != VS_NONE && this->isConnected && (currentOsdType == AnalogPAL || currentOsdType == AnalogNTSC))
            {
                const float amount = this->getNoiseAmount();
                this->drawNoise(amount);
                this->drawInterference(amount);
            } 
        });

    eventBus->Subscribe<OsdToastEventArg>(
        "MakeToast",
        [this](const OsdToastEventArg &event)
        {
            this->makeToast(event.messageLine1, event.messageLine2, event.durationMs);
        });

    eventBus->Subscribe<SimulatorConnectedEventArg>(
        "SimulatorConnected",
        [this](const SimulatorConnectedEventArg &event)
        {
            this->isConnected = event.status == ConnectionStatus::ConnectedHitl || event.status == ConnectionStatus::ConnectedSitl;
            if (!this->isConnected)
            {
                this->disconnect();
            }
        });

    eventBus->Subscribe<SettingsChangedEventArg>(
        "SettingsChanged",
        [this](const SettingsChangedEventArg &event)
        {
            if (event.sectionName != SettingsSections::SECTION_OSD)
            {
                return;
            }

            if (event.settingName == SettingsKeys::SETTINGS_OSD_VISIBLE)
            {
                this->visible = event.getValueAs<bool>(true);
            }
            else if (event.settingName == SettingsKeys::SETTINGS_OSD_FILTER_MODE)
            {
                this->filteringMode = event.getValueAs<TOsdFilteringMode>(Auto);
                // Force reload of textures with new filtering mode
                updateFont();
            }
            else if (event.settingName == SettingsKeys::SETTINGS_VIDEOLINK_SIMULATION)
            {
                this->videoLink = event.getValueAs<TVideoLinkSimulation>(VS_NONE);
            }
        });
}

void OSD::drawOSD()
{
    if (!this->visible)
        return;

    auto font = Plugin()->Fonts()->GetCurrentFont();

    if (!font) {
        return; 
    }


    int rows = font->getRows();
    int cols = font->getCols();

    int windowWidth, windowHeight;
    XPLMGetScreenSize(&windowWidth, &windowHeight);

    uint32_t ticks = Utils::GetTicks();
    bool blink = (ticks % 266) < 133;

    std::vector<uint16_t> osdDataRef;
    bool isToastActive = this->toastEndTime > 0;   
    if (isToastActive)
    {
            osdDataRef = this->osdData;
            const int lineOffset = OSD_MAX_COLS / 2 - 1 - OSDConstants::TOAST_MAX_COLS / 2;
            for (int i = 0; i < OSDConstants::TOAST_MAX_ROWS + 2; i++)
            {
                const int start = i * (OSDConstants::TOAST_MAX_COLS + 2);
                std::copy(this->toastData.begin() + start, this->toastData.begin() + start + OSDConstants::TOAST_MAX_COLS + 2, osdDataRef.begin() + i * OSD_MAX_COLS + lineOffset);
            }

        // Clear toast
        if (ticks > this->toastEndTime)
        {
            resetToast();
        }
    }

    const float textureAspectRatio = static_cast<float>(this->textureWidth) / static_cast<float>(this->textureHeight);

    int cellWidth = windowWidth / this->textureWidth;
    int cellHeight = static_cast<int>(windowHeight / textureAspectRatio);

    const int avaiableWidth = windowWidth - 2 * OSDConstants::OSD_MARGIN;
    const int avaiableHeight = windowHeight - 2 * OSDConstants::OSD_MARGIN;

    if (cellWidth * cols > avaiableWidth)
    {       
        cellWidth = avaiableWidth / cols;
        cellHeight = static_cast<int>(cellWidth / textureAspectRatio);
    }

    if (cellHeight * rows > avaiableHeight)
    {
        cellHeight = avaiableHeight / rows;
        cellWidth = static_cast<int>(cellHeight * textureAspectRatio);
    }

    int xOffset = (windowWidth - cellWidth * cols) / 2;
    int yOffset = (windowHeight - cellHeight * rows) / 2;

    this->osdRenderer->drawOSD(isToastActive ? osdDataRef : this->osdData, rows, cols, cellWidth, cellHeight, xOffset, yOffset, blink);
}

void OSD::drawNoise(float amount)
{
    if (this->noiseTexture < 0)
    {
        return;
    }

    int sx, sy;
    XPLMGetScreenSize(&sx, &sy);

    float size = sx * 1.2f;

    static float dx = 0;
    static float dy = 0;
    static uint32_t t = Utils::GetTicks();

    uint32_t t1 = Utils::GetTicks();
    if ((t1 - t) > 40)
    {
        t = t1;
        dx = -(float)(size - sx) * rand() / RAND_MAX;
        dy = -(float)(size - sy) * rand() / RAND_MAX;
        size = size + size * rand() / RAND_MAX;
    }

    this->osdRenderer->drawInterferenceTexture(this->noiseTexture, static_cast<int>(dx), static_cast<int>(dy), static_cast<int>(size), static_cast<int>(size), amount * amount * amount * amount);
}

void OSD::drawInterference(float amount)
{
    if (this->interferenceTexture < 0)
    {
        return;
    }

    int sx, sy;
    XPLMGetScreenSize(&sx, &sy);

    float sizeX = sx * 1.2f;
    float sizeY = sx * 1.2f / OSDConstants::INERFERENCE_TEXTURE_WIDTH * OSDConstants::INERFERENCE_TEXTURE_HEIGHT;

    static float dx = 0;
    static float dy = 0;
    static uint32_t t = Utils::GetTicks();
    static float delay = 1;

    uint32_t t1 = Utils::GetTicks();
    if ((t1 - t) > 40)
    {
        if ((t1 - t) < (((1.0f - amount) * delay) * 3000.0f))
        {
            dy = 10000;
            return;
        }

        t = Utils::GetTicks();
        dx = -(float)(sizeX - sx) * rand() / RAND_MAX;
        dy = (float)sy * rand() / RAND_MAX;

        sizeX = sizeX + sizeX * rand() / RAND_MAX;
        sizeY = sizeY * (rand() / RAND_MAX + 0.3f);

        if (1.0f * rand() / RAND_MAX > (pow(amount, 0.25)))
            dy = 10000;

        delay = 2.0f * rand() / RAND_MAX;
    }

    this->osdRenderer->drawInterferenceTexture(this->interferenceTexture, static_cast<int>(dx), static_cast<int>(dy), static_cast<int>(sizeX), static_cast<int>(sizeY), amount);
}

void OSD::resetToast()
{
    std::fill(this->toastData.begin(), this->toastData.end(), 0);
    // Draw frame for toast
    for (int r = 0; r < OSDConstants::TOAST_MAX_ROWS + 2; r++)
    {
        for (int c = 0; c < OSDConstants::TOAST_MAX_COLS + 2; c++)
        {
            int pos = r * (OSDConstants::TOAST_MAX_COLS + 2) + c; 
            if ((r == 0 || r == OSDConstants::TOAST_MAX_ROWS + 1 ) && (c != 0 || c != OSDConstants::TOAST_MAX_COLS + 1))
            {
                this->toastData[pos] = OSDConstants::makeCharMode(347, 0); 
            }
            else if (c == 0)
            {
                this->toastData[pos] = OSDConstants::makeCharMode(346, 0); // left vertical line
            }
            
            else if (c == OSDConstants::TOAST_MAX_COLS + 1)
            {
                this->toastData[pos] = OSDConstants::makeCharMode(351, 0); // right vertical line
            }
        }
    }
    std::string header = " XITL ";
    int startCol = (OSDConstants::TOAST_MAX_COLS + 2 - static_cast<int>(header.length())) / 2;
    for (size_t i = 0; i < header.size(); i++)
    {
        char c = header[i];
        this->toastData[i + startCol] = OSDConstants::makeCharMode(c, 0);
    }
    this->toastEndTime = 0;
}

void OSD::updateFromINAV(const TMSPSimulatorOSD& message)
{
    if (message.newFormat.osdRows == 0)
    {
        return; // old format nodata [255, unfilled] or new format no data [255, 0 ]
    }

    int formatVersion = (message.newFormat.osdRows >> 5) & 7;

    if (formatVersion != 0)
    {
        return;
    }

    int message_osdRows = message.newFormat.osdRows & 0x1f;
    int message_osdCols = message.newFormat.osdCols & 0x3f;

    if (message_osdRows > OSD_MAX_ROWS)
    {
        return; // invalid data
    }

    if (message_osdCols > OSD_MAX_COLS)
    {
        return; // invalid data
    }


    int osdRow = message.newFormat.osdRow & 0x1f;
    int osdCol = message.newFormat.osdCol & 0x3f;

    Plugin()->Fonts()->setFontTypeByOsdSize(message_osdRows, message_osdCols);

    this->updateFromINAVRowData(osdRow, osdCol, message.newFormat.osdRowData, message_osdRows);
}

void OSD::updateFromINAVRowData(int osdRow, int osdCol, const uint8_t (&data)[MSPConstants::OSD_BUFFER_SIZE], int decodeRowsCount)
{
    auto plugin = Plugin();

    auto font = plugin->Fonts()->GetCurrentFont();
    if (font == nullptr)
    {
        return; // no font loaded
    }

    int rows = font->getRows();
    int cols = font->getCols();

    if (osdRow >= rows)
    {
        return; // invalid data
    }

    if (osdCol >= cols)
    {
        return; // invalid data
    }

    // Special case for Analog NTSC: always 13 rows
    if (Plugin()->Fonts()->getCurrentFontType() == OsdType::AnalogNTSC)
    {
       rows = NTSC_ROWS;
    }

    bool highBank = false;
    bool blink = false;
    int count;

    int byteCount = 0;
    while (byteCount < (400 - 3 - 2))
    {
        uint8_t c = data[byteCount++];
        if (c == 0)
        {
            c = data[byteCount++];
            count = (c & 0x3f);
            if (count == 0)
            {
                break; // stop
            }
            highBank ^= (c & 64) != 0;
            blink ^= (c & 128) != 0;
            c = data[byteCount++];
        }
        else if (c == 255)
        {
            highBank = !highBank;
            c = data[byteCount++];
            count = 1;
        }
        else
        {
            count = 1;
        }

        while (count > 0)
        {
            this->osdData[osdRow * OSD_MAX_COLS + osdCol] = OSDConstants::makeCharMode((c | (highBank ? 0x100 : 0)), (blink ? OSDConstants::MAX7456_MODE_BLINK : 0));
            osdCol++;
            if (osdCol == cols)
            {
                osdCol = 0;
                osdRow++;
                if (osdRow == decodeRowsCount)
                {
                    osdRow = 0;
                    Plugin()->GetEventBus()->Publish("OSDFrameUpdated");
                }
            }
            count--;
        }
    }
}

void OSD::clear()
{
    std::fill(this->osdData.begin(), this->osdData.end(), 0);
}

void OSD::updateFont()
{
    auto font = Plugin()->Fonts()->GetCurrentFont();
    if (font != nullptr)
    {
        bool smoothed = false;
        if (this->filteringMode == Auto)
        {
            smoothed = !font->isAnalog();
        }
        else if (this->filteringMode == Linear)
        {
            smoothed = true;
        }
        
        this->osdRenderer->loadOSDTextures(font->getTextures(), font->getCharWidth(), font->getCharHeight(), smoothed);
    }
    else
    {
        Utils::LOG("No font loaded, OSD textures not initialized");
    }

    this->textureWidth = font != nullptr ? font->getCharWidth() : 0;
    this->textureHeight = font != nullptr ? font->getCharHeight() : 0;
}


float OSD::getNoiseAmount()
{
    
    float d = MathUtils::LatDistanceM(this->home_lattitude, this->home_longitude, this->home_elevation,
                                  this->current_lattitude, this->current_longitude, this->current_elevation);

    float maxD;
    switch (this->videoLink)
    {
    case VS_2KM:
        maxD = 2000;
        break;
    case VS_10KM:
        maxD = 10000;
        break;
    default:
        maxD = 50000;
        break;
    }

    float res = d / maxD;
    float s = sin(this->roll / 180.0f * 3.14f);
    res += s * s * 0.2f;
    if (res > 0.99f)
        res = 0.99f;

    if (res < 0.475f)
        res = 0.475f;
    return res;

}

void OSD::makeToast(std::string line1, std::string line2, int durationMs)
{
    resetToast();

    if (line1.length() > OSDConstants::TOAST_MAX_COLS)
    {
        line1 = line1.substr(0, OSDConstants::TOAST_MAX_COLS);
    }

    if (line2.length() > OSDConstants::TOAST_MAX_COLS)
    {
        line2 = line2.substr(0, OSDConstants::TOAST_MAX_COLS);
    }

    line1 = Utils::ToUpper(line1);
    line2 = Utils::ToUpper(line2);

    int line1StartCol = (OSDConstants::TOAST_MAX_COLS + 2 - static_cast<int>(line1.length())) / 2;
    int line2StartCol = (OSDConstants::TOAST_MAX_COLS + 2 - static_cast<int>(line2.length())) / 2;

    for (size_t i = 0; i < line1.length(); i++)
    {
        char c = line1.c_str()[i];
        this->toastData[1 * (OSDConstants::TOAST_MAX_COLS + 2) + line1StartCol + i] = OSDConstants::makeCharMode(c, 0);
    }

    for (size_t i = 0; i < line2.length(); i++)
    {
        char c = line2.c_str()[i];
        this->toastData[2  * (OSDConstants::TOAST_MAX_COLS + 2) + line2StartCol + i] = OSDConstants::makeCharMode(c, 0);
    }

    this->toastEndTime = Utils::GetTicks() + durationMs;
}

void OSD::disconnect()
{
    Plugin()->Fonts()->setFontType(OsdType::WtfOS);
    this->clear();
    this->makeToast("DISCONNECTED", "", 5000);
}
