#pragma once

#include "../platform.h"

#include <vector>
#include <memory>

#include <ImgWindow.h>

#include "../Utils.h"

static constexpr int MAX_SERIAL_PORTS_WIN   = 32;       /// max number of serial ports
static constexpr int MAX_SERIAL_PORTS_LIN   = 16;       /// max number of serial ports
static constexpr int WIN_COLL_OFS       =  30;      /// offset of collated windows
static constexpr int SITL_FIRST_PORT    =  5760; 
static constexpr int SITL_PORT_COUNT    =  8;
static constexpr int MAX_FONTS          =  32;

static const char* OSD_FILTERING_MODES[] = {
    "Auto",
    "Nearest",
    "Linear"
};

static const char* OSD_MODES[] = {
    "Auto detect",
    "PAL (30x16)",
    "NTSC (30x13)"
    "WTFOS (60x22)",
    "HDZero (50x18)",
    "Avatar (53x20)"
};

class SettingsWindow : public ImgWindow
{
public:

    static std::shared_ptr<SettingsWindow> Instance;

    void loadSettings();
    void saveSettings();

    SettingsWindow(int left, int top, int right, int bot,
                XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle,
                XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows);
    ~SettingsWindow() override;

protected:
    
    // Main function: creates the window's UI
    void buildInterface() override;

private:

    char *serialPort[MAX_SERIAL_PORTS_WIN];
    char *sitlPorts[SITL_PORT_COUNT];

    std::vector<std::string> analogFonts;
    std::vector<const char*> analogFontsDisplayNames;
    std::vector<std::string> hdZeroFonts;
    std::vector<const char*> hdZeroFontsDisplayNames;
    std::vector<std::string> avatarFonts;
    std::vector<const char*> avatarFontsDisplayNames;
    std::vector<std::string> wtfOSFonts;
    std::vector<const char*> wtfOSFontsDisplayNames;

    bool ipAddressValid = true;
    bool hasInvalidIpAddressDecoration = false;

    // Settings variables
    bool autoDetectFcPort = true;
    bool copyAttitudeFromXPlane = true;
    bool muteBeeper = true;
    bool restartOnAirportLoad = false;
    bool osdSmoothing = true;
    int hitlComPort = 4;
    int sitlPortIndex = 0;
    char sitlIpAddress[16] = "127.0.0.1";
    int osdFilteringMode = 0;
    int analogFontIndex = 0;
    int hdZeroFontIndex = 0;
    int avatarFontIndex = 0;
    int wtfOSFontIndex = 0;

    static void HelpMarker(const char* desc);

};