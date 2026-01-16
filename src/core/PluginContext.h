#pragma once

#include "../platform.h"

#include "EventBus.h"

#include <memory>
#include <stdexcept>

class Settings;
class Menu;
class Fonts;
class MSP;
class SimData;
class OSD;
class Graph;
class DataRefs;
class Map;

class PluginContext
{
private:
    static std::unique_ptr<PluginContext> instance;
    
    std::shared_ptr<::EventBus> _eventBus;
    std::shared_ptr<::Fonts> _fonts;
    std::shared_ptr<::MSP> _mspConnection;
    std::shared_ptr<::SimData> _simData;
    std::shared_ptr<::OSD> _osd;
    std::shared_ptr<::Graph> _graph;
    std::shared_ptr<::DataRefs> _dataRefs;
    std::shared_ptr<::Menu> _menu;
    std::shared_ptr<::Map> _map;
    std::shared_ptr<::Settings> _settings;

    PluginContext();

public:
    ~PluginContext();

    static void Initialize();
    static PluginContext* Instance();
    static void Reset();

    std::shared_ptr<::EventBus> GetEventBus() const { return _eventBus; }
    std::shared_ptr<::Fonts> Fonts() const { return _fonts; }
    std::shared_ptr<::Menu> Menu() const { return _menu; }
    std::shared_ptr<::Settings> Settings() const { return _settings; }
    static PluginContext CreateForTesting();
};

inline PluginContext* Plugin()
{
    return PluginContext::Instance();
}
