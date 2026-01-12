#include "platform.h"

#include <XPLMProcessing.h>
#include <XPLMDisplay.h>

#include "core/PluginContext.h"

#include "Utils.h"

static constexpr const char* pluginName = "INAV XITL";
static constexpr const char* pluginSig = "com.scavanger.inav.xplane-xitl";
static constexpr const char* pluginDesc = "INAV Hardware/Software In The Loop";

bool firstRender = true;
XPLMFlightLoopID flightLoopId;

// this flightloop callback will be called every frame to update the targets
float Flightloop(float elapsed1, float elapsed2, int ctr, void *refcon)
{
    Plugin()->GetEventBus()->Publish("FlightLoop", FlightLoopEventArg{elapsed1, ctr});
    return -1;
}

int DrawCallback(XPLMDrawingPhase inPhase, int inIsBefore, void *inRefcon)
{
    Plugin()->GetEventBus()->Publish("DrawCallback", DrawCallbackEventArg{inPhase, inIsBefore});
    return 1;
}

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc)
{
    Utils::LOG("Plugin start");

    strcpy(outName, pluginName);
    strcpy(outSig, pluginSig);
    strcpy(outDesc, pluginDesc);

    XPLMEnableFeature("XPLM_USE_NATIVE_PATHS", 1);
    XPLMRegisterDrawCallback(&DrawCallback, xplm_Phase_Window, 0, NULL);
    return 1;
}

PLUGIN_API void XPluginStop(void)
{
    Utils::LOG("Plugin stop");

    XPLMUnregisterDrawCallback(&DrawCallback, xplm_Phase_FirstCockpit, 0, NULL);
}

PLUGIN_API int XPluginEnable(void)
{
    Utils::LOG("Plugin enable");

    try
    {
        PluginContext::Initialize();
    }
    catch (const std::exception &e)
    {
        Utils::LOG("Error at PluginContext initialization: {}", e.what());
        return 0;
    }

    XPLMCreateFlightLoop_t params;
    params.structSize = sizeof(XPLMCreateFlightLoop_t);
    params.callbackFunc = &Flightloop;
    params.phase = xplm_FlightLoop_Phase_AfterFlightModel;
    params.refcon = NULL;
    flightLoopId = XPLMCreateFlightLoop(&params);
    XPLMScheduleFlightLoop(flightLoopId, -1, true);

    return 1;
}

PLUGIN_API void XPluginDisable(void)
{
    Utils::LOG("Plugin disable");
    XPLMDestroyFlightLoop(flightLoopId);
}

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFrom, int inMsg, void * inParam)
{
    if (inMsg == XPLM_MSG_AIRPORT_LOADED)
    {
        Plugin()->GetEventBus()->Publish("AirportLoaded");
        firstRender = false;
    }
} 
