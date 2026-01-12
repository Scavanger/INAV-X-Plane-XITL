#pragma once

#include "../platform.h"

#include <vector>
#include <memory>

#include <ImgWindow.h>

#include "../Utils.h"

static const char* GRAPH_TYPES[] = {
    "MSP Updates",
    "Attitude Output",
    "Attitude Estimation",
    "Accelerometer",
    "Gyroscope",
    "Debug Altitude",
    "Debug Custom"
};

class GraphSelectWindow : public ImgWindow
{  
public:

    static std::shared_ptr<GraphSelectWindow> instance;

    GraphSelectWindow(int left, int top, int right, int bot, XPLMWindowDecoration decoration = xplm_WindowDecorationRoundRectangle, XPLMWindowLayer layer = xplm_WindowLayerFloatingWindows);
    ~GraphSelectWindow() override;
    void loadSettings();

protected:
    
    // Main function: creates the window's UI
    void buildInterface() override;

private:
    int selectedGraphType = 0;
 };