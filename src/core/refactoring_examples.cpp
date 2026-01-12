/**
 * @file refactoring_examples.cpp
 * @brief Praktische Beispiele für Refaktorierung verschiedener Szenarien
 * 
 * Dieses Dokument enthält Code-Beispiele die zeigen, wie man verschiedene
 * globale Abhängigkeits-Muster in die neue Architektur migriert.
 */

// ====
// BEISPIEL 1: Komponente mit direkter Abhängigkeit von anderen Komponenten
// ====

// VORHER: OSD ruft direkt SimData auf
// osd.cpp
#include "simData.h"
extern SimData g_simData;  // Globale Abhängigkeit

void TOSD::updateFromINAV(const TMSPSimulatorFromINAV* message)
{
    // ... update logic ...
    g_simData.sendToXPlane();  // Direkte globale Abhängigkeit
}

// NACHHER: OSD erhält SimData über PluginContext
// osd.cpp (refactored)
#include "core/PluginContext.h"

void TOSD::updateFromINAV(const TMSPSimulatorFromINAV* message)
{
    // ... update logic ...
    // Verwende PluginContext statt globaler Variable
    auto app = Plugin();
    app->SimData()->sendToXPlane();
}

// ODER NOCH BESSER: Mit Events (völlig entkoppelt)
// osd.cpp (fully refactored)
void OSD::init()
{
    auto app = Plugin();
    
    // Subscribe auf Events statt direkt andere Komponenten aufzurufen
    app->EventBus()->Subscribe<MSPSimulatorDataReceivedEvent>(
        [this](const auto& event) {
            this->onSimulatorDataReceived(event);
        }
    );
}

void TOSD::onSimulatorDataReceived(const MSPSimulatorDataReceivedEvent& event)
{
    // Verarbeite Daten
    // Kein direkter Aufruf anderer Komponenten nötig
}

// ====
// BEISPIEL 2: Bidirektionale Abhängigkeiten → Event Chain
// ====

// VORHER: Zirkelbezug zwischen Komponenten
// menu.cpp
extern SimData g_simData;
extern TOSD g_osd;

void TMenu::onConnectClicked()
{
    g_simData.connect();     // Ruft SimData auf
}

// simData.cpp
extern TMenu g_menu;
extern TOSD g_osd;

void SimData::onConnected()
{
    g_menu.updateMenuState();  // Ruft Menu zurück → Zirkelbezug!
    g_osd.onConnected();
}

// NACHHER: Mit Events (saubere Abhängigkeitsrichtung)
// menu.cpp (refactored)
void TMenu::onConnectClicked()
{
    auto app = Plugin();
    
    // Publiziere Event, anstatt andere Komponente direkt aufzurufen
    UserRequestedConnectionEvent event;
    event.portName = selectedPort;
    app->EventBus()->Publish(event);
}

// simData.cpp (refactored)
void SimData::init()
{
    auto app = Plugin();
    
    // Subscribe auf Verbindungsanfrage
    app->EventBus()->Subscribe<UserRequestedConnectionEvent>(
        [this](const auto& event) {
            this->connectToFC(event.portName);
        }
    );
}

void TSimData::onConnected()
{
    auto app = Plugin();
    
    // Publiziere Event, dass Verbindung hergestellt wurde
    // Menu und OSD reagieren separat auf dieses Event
    SimulatorConnectedEventArg event;
    event.isConnected = true;
    app->EventBus()->Publish(event);
}

// menu.cpp (refactored) - reagiert auf das Event
void TMenu::init()
{
    auto app = Plugin();
    
    app->EventBus()->Subscribe<SimulatorConnectedEvent>(
        [this](const auto& event) {
            if (event.isConnected) {
                this->updateMenuState();
            }
        }
    );
}

// osd.cpp (refactored) - reagiert auch auf das Event
void OSD::init()
{
    auto app = Plugin();
    
    app->EventBus()->Subscribe<SimulatorConnectedEventArg>(
        [this](const auto& event) {
            if (event.isConnected) {
                this->onConnected();
            }
        }
    );
}

// ====
// BEISPIEL 3: Polling-basiert → Event-basiert
// ====

// VORHER: Polling in der Haupt-Schleife
// main.cpp
void floop_cb(...)
{
    if (g_msp.isConnected())  // Polling
    {
        if ((GetTicks() - lastUpdateTime) > MSP_PERIOD_MS)
        {
            g_simData.updateFromXPlane();
            // ...
        }
    }
}

// NACHHER: Event-basiert
// main.cpp (refactored)
void floop_cb(...)
{
    auto app = Plugin();
    
    if (app->MspConnection()->isConnected())
    {
        if ((GetTicks() - lastUpdateTime) > MSP_PERIOD_MS)
        {
            app->SimData()->updateFromXPlane();
            
            // Publish Event dass Update erfolgreich war
            SensorDataUpdatedEventArg event;
            event.updateTime = GetTicks();
            app->EventBus()->Publish(event);
            
            lastUpdateTime = GetTicks();
        }
    }
}

// Komponenten können nun auf dieses Event reagieren:
// graph.cpp
void TGraph::init()
{
    auto app = Plugin();
    
    app->EventBus()->Subscribe<SensorDataUpdatedEventArg>(
        [this](const auto& event) {
            this->onSensorDataUpdated(event);
        }
    );
}

// ====
// BEISPIEL 4: Logging und Error Handling
// ====

// VORHER: Direkte Aufrufe beim Error
// msp.cpp
if (error)
{
    g_menu.actionDisconnect();      // Direkt Menu aufrufen
    playSound("error.wav");          // Global function
    Utils::LOG("Error occurred");
    g_osd.showMsg("ERROR");          // Direkt OSD aufrufen
}

// NACHHER: Mit Error-Event
// msp.cpp (refactored)
if (error)
{
    auto app = Plugin();
    
    SimulatorErrorEvent event;
    event.errorMessage = "MSP communication failed";
    event.shouldDisconnect = true;
    app->EventBus()->Publish(event);
}

// main.cpp
void OnSimulatorError(const SimulatorErrorEventArg& event)
{
    auto app = Plugin();
    
    Utils::LOG("Simulator Error: {}", event.errorMessage);
    
    if (event.shouldDisconnect)
    {
        app->Menu()->actionDisconnect();
    }
    
    app->Sound()->playSound("error.wav");
    app->OSD()->showMsg(event.errorMessage.c_str());
}

// In XPluginEnable:
// app->EventBus()->Subscribe<SimulatorErrorEvent>(OnSimulatorError);

// ====
// BEISPIEL 5: Testing - Komponenten mit neuer Architektur testen
// ====

// Test mit neuer Architektur
#include "core/PluginContext.h"
#include <cassert>

void TestOSDUpdatesOnSimulatorData()
{
    // Erstelle isolierten Test-Context
    auto testCtx = PluginContext::CreateForTesting();
    
    // Oder noch besser: Mock die Abhängigkeiten
    auto mockEventBus = std::make_shared<EventBus>();
    auto mockSimData = std::make_shared<MockTSimData>();
    
    // Erstelle OSD mit Mock-Abhängigkeiten
    auto osd = std::make_shared<TOSD>();
    
    bool callbackWasCalled = false;
    
    // Setze Event-Listener der komponente
    mockEventBus->Subscribe<SensorDataUpdatedEvent>(
        [&callbackWasCalled](const auto& event) {
            callbackWasCalled = true;
        }
    );
    
    // Trigger Event
    SensorDataUpdatedEventArg event;
    mockEventBus->Publish(event);
    
    // Verify
    assert(callbackWasCalled);
    Utils::LOG("Test passed!");
}

// ====
// BEISPIEL 6: Schrittweise Migration eines Headers
// ====

// graph.h VORHER
#pragma once
#include "platform.h"
#include "simData.h"

class Graph {
private:
    // Versteckte Abhängigkeit!
public:
    void updateGraphs() {
        // Direkt auf globale Variablen zugreifen
        extern SimData g_simData;
        auto val = g_simData.someValue;
    }
};

extern Graph g_graph;

// graph.h NACHHER (Phase 1: Legacy Adapter)
#pragma once
#include "platform.h"
#include "core/LegacyAdapter.h"

class Graph {
private:
    std::shared_ptr<EventBus> eventBus;
public:
    void init(std::shared_ptr<EventBus> bus) {
        eventBus = bus;
        eventBus->Subscribe<SensorDataUpdatedEventArg>(
            [this](const auto& e) { this->onSensorDataUpdated(e); }
        );
    }
    
    void updateGraphs() {
        // Verwende Adapter statt g_simData
        auto& simData = GetSimData();
        auto val = simData.someValue;
    }
};

// graph.h NACHHER (Phase 2: Vollständige Refaktorierung)
#pragma once
#include "platform.h"

class Graph {
private:
    std::shared_ptr<EventBus> eventBus;
    std::shared_ptr<SimData> simData;
    
    void onSensorDataUpdated(const SensorDataUpdatedEventArg& event);
    
public:
    Graph(std::shared_ptr<EventBus> bus, std::shared_ptr<SimData> sim)
        : eventBus(bus), simData(sim) {}
    
    void init() {
        eventBus->Subscribe<SensorDataUpdatedEvent>(
            [this](const auto& e) { this->onSensorDataUpdated(e); }
        );
    }
    
    void updateGraphs() {
        auto val = simData->someValue;  // Explizite Abhängigkeit
    }
};

// ====
// BEISPIEL 7: RAII Pattern - Constructor/Destructor statt init/destroy
// ====

// ❌ ALTMUSTER: init() / destroy() Methods
// stats.h (OLD)
class DataRefs {
public:
    void init();      // Manuell aufrufen!
    void close();     // Manuell aufrufen!
private:
    std::vector<XPLMDataRef> dataRefs;
};

// stats.cpp (OLD)
void DataRefs::init() {
    dataRefs.push_back(XPLMRegisterDataAccessor("inav/yaw", ...));
    dataRefs.push_back(XPLMRegisterDataAccessor("inav/pitch", ...));
    Utils::LOG("TStats initialized");
}

void DataRefs::close() {
    for (auto ref : dataRefs) {
        XPLMUnregisterDataAccessor(ref);
    }
    dataRefs.clear();
    Utils::LOG("TStats closed");
}

// In main.cpp (OLD)
app->Stats()->init();   // ❌ Muss manuell aufgerufen werden
// ... später ...
app->Stats()->close();  // ❌ Muss manuell aufgerufen werden

// PROBLEME:
// - Leicht zu vergessen
// - Asymmetrisch (init + close müssen gepaart sein)
// - Keine Exception Safety
// - Schwer zu refaktorieren

// ====

// ✅ RAII PATTERN: Constructor / Destructor
// stats.h (NEW)
class DataRefs {
public:
    // Constructor: Automatisch aufgerufen beim Erstellen
    DataRefs(std::shared_ptr<EventBus> eventBus);
    
    // Destructor: Automatisch aufgerufen beim Löschen
    ~DataRefs();
    
private:
    std::shared_ptr<EventBus> _eventBus;
    std::vector<XPLMDataRef> dataRefs;
};

// stats.cpp (NEW)
DataRefs::DataRefs(std::shared_ptr<EventBus> eventBus)
    : _eventBus(eventBus)
{
    Utils::LOG("TStats: Constructor - Initializing DataRefs");
    
    dataRefs.push_back(XPLMRegisterDataAccessor("inav/yaw", ...));
    dataRefs.push_back(XPLMRegisterDataAccessor("inav/pitch", ...));
    
    // Subscribe auf Events
    _eventBus->Subscribe<FlightLoopEvent>(
        [this](const auto& e) { this->onFlightLoop(e); }
    );
    
    Utils::LOG("TStats: Constructor - Ready");
}

DataRefs::~DataRefs() {
    Utils::LOG("TStats: Destructor - Cleaning up DataRefs");
    
    for (auto ref : dataRefs) {
        XPLMUnregisterDataAccessor(ref);  // Automatisch beim Löschen
    }
    dataRefs.clear();
    
    Utils::LOG("TStats: Destructor - Done");
}

// In main.cpp (NEW)
ApplicationContext::Initialize();  // ✅ TStats Constructor läuft automatisch
// ... work ...
ApplicationContext::Reset();       // ✅ TStats Destructor läuft automatisch

// VORTEILE:
// - Automatisch - keine Aufrufe nötig
// - Symmetrisch - Constructor ↔ Destructor
// - Exception Safe - RAII garantiert Cleanup
// - Dependency Injection - Dependencies im Constructor
// - Skalierbar - neue Komponenten = automatisch

// ====

// ✅ KOMPLEXES BEISPIEL: Komponente mit Dependencies

// osd.h
class OSD {
public:
    // Dependencies via Constructor Injection
    OSD(std::shared_ptr<EventBus> eventBus,
         std::shared_ptr<SimData> simData);
    
    ~OSD();
    
private:
    std::shared_ptr<EventBus> _eventBus;
    std::shared_ptr<SimData> _simData;
    std::unique_ptr<OSDRenderer> _renderer;
    std::vector<FontPtr> _fonts;
};

// osd.cpp
OSD::OSD(std::shared_ptr<EventBus> eventBus,
           std::shared_ptr<SimData> simData)
    : _eventBus(eventBus), _simData(simData)
{
    Utils::LOG("TOSD: Constructor - Initializing");
    
    try {
        // Initialize Renderer
        _renderer = std::make_unique<OSDRenderer>();
        Utils::LOG("  - Renderer created");
        
        // Load Fonts
        _fonts.push_back(std::make_shared<FontAnalog>());
        _fonts.push_back(std::make_shared<FontHDZero>());
        Utils::LOG("  - Fonts loaded");
        
        // Subscribe auf Events
        _eventBus->Subscribe<SimulatorConnectedEvent>(
            [this](const auto& e) { this->onConnected(e); }
        );
        
        _eventBus->Subscribe<MSPSimulatorDataReceivedEvent>(
            [this](const auto& e) { this->onDataReceived(e); }
        );
        
        Utils::LOG("TOSD: Constructor - Ready");
    }
    catch (const std::exception& e) {
        Utils::LOG("TOSD: Constructor failed - {}", e.what());
        // Destruktor wird automatisch aufgerufen - Cleanup garantiert
        throw;
    }
}

OSD::~OSD() {
    Utils::LOG("TOSD: Destructor - Cleaning up");
    
    // Cleanup in umgekehrter Reihenfolge:
    _fonts.clear();          // Fonts destruieren
    _renderer.reset();       // Renderer destruieren
    
    // EventBus ist noch verfügbar (shared_ptr hat andere Referenzen)
    Utils::LOG("TOSD: Destructor - Done");
}

// In ApplicationContext
ApplicationContext::ApplicationContext()
    : _eventBus(std::make_shared<EventBus>()),
      _simData(std::make_shared<TSimData>()),
      _osd(std::make_shared<TOSD>(_eventBus, _simData))  // Dependencies injiziert
{
    Utils::LOG("ApplicationContext: All components initialized");
}

ApplicationContext::~ApplicationContext() {
    Utils::LOG("ApplicationContext: Cleaning up in reverse order");
    
    // Destruktoren laufen automatisch in umgekehrter Reihenfolge:
    // 1. _osd destruiert (braucht _simData & _eventBus)
    // 2. _simData destruiert
    // 3. _eventBus destruiert
    
    Utils::LOG("ApplicationContext: Cleanup complete");
}

// ====

// ✅ VERGLEICH: init() Pattern vs. RAII Pattern

// INIT PATTERN (❌ ALTMODISCH)
void LegacyInitialization() {
    auto app = App();
    
    // Initialwerte setzen
    app->Stats()->init();
    app->Graph()->init();
    app->OSD()->init();
    // ❌ Was wenn eines schlägt fehl? Weitere nicht initialisiert!
    // ❌ Was wenn wir eines vergessen? Memory Leak!
}

// RAII PATTERN (✅ MODERN)
void ModernInitialization() {
    ApplicationContext::Initialize();  // Alles automatisch
    // ✅ Alle Komponenten sind guarantiert initialisiert
    // ✅ Exception Safe - wenn einer schlägt, andere werden aufgeräumt
}

// ====

// ✅ MIGRATION STRATEGIE

// SCHRITT 1: Komponente bekommt Constructor
class MyComponent {
public:
    MyComponent(std::shared_ptr<EventBus> eventBus)
        : _eventBus(eventBus)
    {
        // Initialisierung im Constructor
        Utils::LOG("MyComponent initialized");
    }
    
    ~MyComponent() {
        // Cleanup im Destruktor
        Utils::LOG("MyComponent destroyed");
    }
    
private:
    std::shared_ptr<EventBus> _eventBus;
};

// SCHRITT 2: Komponente hat KEINE init() Methode mehr
// (oder behält sie für Backward Compatibility aber deprecated)

// SCHRITT 3: ApplicationContext erstellt Komponenten mit Constructors
auto component = std::make_shared<MyComponent>(eventBus);
// ✅ Constructor läuft automatisch

// SCHRITT 4: ApplicationContext destruiert Komponenten automatisch
component.reset();  // oder component geht out of scope
// ✅ Destruktor läuft automatisch
