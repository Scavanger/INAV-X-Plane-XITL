#include "PluginContext.h"
#include "../Utils.h"
#include "../settings/Settings.h"
#include "../Menu.h"
#include "../fonts/Fonts.h"
#include "../MSP.h"
#include "../SimData.h"
#include "../OSD.h"
#include "../Graph.h"
#include "../DataRefs.h"
#include "../Map.h"
#include "../widgets/ConfigureWindow.h"
#include "../widgets/SettingsWindow.h"

std::unique_ptr<PluginContext> PluginContext::instance = nullptr;

PluginContext::PluginContext()
    : _eventBus(std::shared_ptr<EventBus>(new EventBus()))
{
    Utils::LOG("PluginContext initialized");
}

PluginContext::~PluginContext()
{
    Utils::LOG("PluginContext destroyed");
}

void PluginContext::Initialize()
{
    if (instance)
    {
        throw std::runtime_error("PluginContext already initialized");
    }
    instance = std::unique_ptr<PluginContext>(new PluginContext());
    
    // Extra initialization if needed
    ConfigureImgWindow::configure();

    // Initialize components to ensure they are created
    instance->_dataRefs = std::make_shared<::DataRefs>();
    instance->_menu = std::make_shared<::Menu>();
    instance->_fonts = std::make_shared<::Fonts>();
    instance->_mspConnection = std::make_shared<::MSP>();
    instance->_simData = std::make_shared<::SimData>();
    instance->_osd = std::make_shared<::OSD>();
    instance->_graph = std::make_shared<::Graph>();    
    instance->_map = std::make_shared<::Map>();
    // Must be last as other components may depend on it - puplishes events on load 
    instance->_settings = std::make_shared<::Settings>();
}

PluginContext* PluginContext::Instance()
{
    if (!instance)
    {
        throw std::runtime_error("PluginContext not initialized. Call Initialize() first.");
    }
    return instance.get();
}

void PluginContext::Reset()
{
    instance.reset();

    // Extra cleanup if needed
    ConfigureImgWindow::cleanup();
}

PluginContext PluginContext::CreateForTesting()
{
    return PluginContext();
}
