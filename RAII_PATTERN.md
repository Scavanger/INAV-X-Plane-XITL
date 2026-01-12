# RAII Pattern für Component Lifecycle Management

## Überblick: init()/destroy() → Constructor/Destructor

Die Idee ist ausgezeichnet: Statt expliziter `init()`/`destroy()`/`close()` Aufrufe sollten wir **RAII (Resource Acquisition Is Initialization)** nutzen, um Ressourcen automatisch zu verwalten.

## Warum RAII besser ist

### Probleme mit init()/destroy()

```cpp
// ❌ ALTMUSTER - Fehleranfällig
PLUGIN_API int XPluginEnable(void) {
    ApplicationContext::Initialize();
    auto app = App();
    
    app->OSD()->init();          // Manuell aufrufen
    app->Stats()->init();        // Leicht zu vergessen
    app->SimData()->init();
    
    return 1;
}

PLUGIN_API void XPluginDisable(void) {
    auto app = App();
    
    app->OSD()->destroy();       // MUSS aufgerufen werden
    app->Stats()->close();       // Was ist close() vs destroy()?
    app->SimData()->destroy();   // Wenn wir einen vergessen = Memory Leak!
    
    ApplicationContext::Reset();
}
```

**Probleme:**
- ❌ Explizite Aufrufe leicht zu vergessen
- ❌ Asymmetrisch: init() muss mit destroy() gepaart sein
- ❌ Keine Exception Safety (wenn init() wirft, werden andere nicht aufgerufen)
- ❌ Schwer zu refaktorieren (neue Komponenten = neue init/destroy Aufrufe)
- ❌ Destruktoren nicht garantiert aufgerufen

### RAII Lösung - Automatisch

```cpp
// ✅ RAII-PATTERN - Automatisch
PLUGIN_API int XPluginEnable(void) {
    ApplicationContext::Initialize();  // Konstruktoren laufen
    // Alles ist initialisiert und ready!
    return 1;
}

PLUGIN_API void XPluginDisable(void) {
    ApplicationContext::Reset();  // Destruktoren laufen automatisch
    // Alles ist sauber aufgeräumt!
}
```

**Vorteile:**
- ✅ Keine expliziten Aufrufe nötig
- ✅ Symmetrisch: Constructor ↔ Destructor
- ✅ Exception safe (RAII garantiert cleanup)
- ✅ Skalierbar: Neue Komponenten = automatisch
- ✅ Destruktoren GARANTIERT aufgerufen (auch bei Exceptions)

## Implementierungs-Leitfaden

### Phase 1: Component Constructor

**VOR (init() Methode):**
```cpp
class TStats {
public:
    void init();          // Manuell aufrufen
private:
    XPLMDataRef df_yaw;
};

void TStats::init() {
    df_yaw = XPLMFindDataRef("sim/flightmodel/position/p");
    // ... weitere DataRefs ...
}
```

**NACHHER (Constructor):**
```cpp
class TStats {
public:
    TStats();             // Automatisch aufgerufen
    ~TStats();            // Automatisch aufgerufen
private:
    XPLMDataRef df_yaw;
};

TStats::TStats() {
    LOG("TStats initializing");
    df_yaw = XPLMFindDataRef("sim/flightmodel/position/p");
    // ... weitere DataRefs ...
}

TStats::~TStats() {
    LOG("TStats cleaning up");
    // X-Plane unregisters DataRefs automatisch
}
```

### Phase 2: Abhängigkeiten im Constructor

**Wenn Komponente andere Komponenten braucht:**

```cpp
class TOSD {
public:
    TOSD(std::shared_ptr<EventBus> eventBus, 
         std::shared_ptr<TSimData> simData);
    ~TOSD();
private:
    std::shared_ptr<EventBus> _eventBus;
    std::shared_ptr<TSimData> _simData;
};

TOSD::TOSD(std::shared_ptr<EventBus> eventBus,
           std::shared_ptr<TSimData> simData)
    : _eventBus(eventBus), _simData(simData)
{
    LOG("TOSD initializing");
    
    // Subscribe auf Events
    _eventBus->Subscribe<SimulatorConnectedEvent>(
        [this](const auto& e) { this->onConnected(e); }
    );
    
    // Initialize OpenGL, Fonts, etc.
    initializeRenderer();
    loadFonts();
}

TOSD::~TOSD() {
    LOG("TOSD cleaning up");
    // EventBus unsubscribe (wenn implementiert)
    // OpenGL cleanup
    // Font resources freigeben
    destroyRenderer();
}
```

### Phase 3: ApplicationContext mit Dependency Injection

```cpp
class ApplicationContext {
private:
    std::shared_ptr<EventBus> _eventBus;
    std::shared_ptr<MSP> _mspConnection;
    std::shared_ptr<TSimData> _simData;
    std::shared_ptr<TOSD> _osd;
    
public:
    ApplicationContext() {
        LOG("ApplicationContext: Creating components");
        
        // Initialisiere in korrekter Reihenfolge
        _eventBus = std::make_shared<EventBus>();
        LOG("  - EventBus created");
        
        _mspConnection = std::make_shared<MSP>();
        LOG("  - MSP created");
        
        _simData = std::make_shared<TSimData>();
        LOG("  - SimData created");
        
        // OSD braucht EventBus & SimData → Pass als Dependencies
        _osd = std::make_shared<TOSD>(_eventBus, _simData);
        LOG("  - OSD created");
        
        // ... weitere Komponenten ...
        
        LOG("ApplicationContext: All components initialized");
    }
    
    ~ApplicationContext() {
        LOG("ApplicationContext: Destroying components");
        
        // Destruktoren laufen in UMGEKEHRTER Initialisierungs-Reihenfolge!
        // Das ist wichtig für Abhängigkeiten
        
        _osd.reset();           // TOSD Destruktor läuft
        LOG("  - OSD destroyed");
        
        _simData.reset();       // TSimData Destruktor läuft
        LOG("  - SimData destroyed");
        
        _mspConnection.reset(); // MSP Destruktor läuft
        LOG("  - MSP destroyed");
        
        _eventBus.reset();      // EventBus Destruktor läuft
        LOG("  - EventBus destroyed");
        
        LOG("ApplicationContext: All components destroyed");
    }
};
```

## Praktische Beispiele

### Beispiel 1: Simple Komponente (TStats)

**VOR (init/close Pattern):**
```cpp
// stats.h
class TStats {
public:
    void init();
    void close();
private:
    std::vector<XPLMDataRef> dataRefs;
};

// stats.cpp
void TStats::init() {
    dataRefs.push_back(XPLMRegisterDataAccessor("inav/yaw", ...));
    dataRefs.push_back(XPLMRegisterDataAccessor("inav/pitch", ...));
}

void TStats::close() {
    for (auto ref : dataRefs) {
        XPLMUnregisterDataAccessor(ref);  // Manual cleanup
    }
    dataRefs.clear();
}

// In main.cpp
app->Stats()->init();   // ❌ Muss manuell aufgerufen werden
// ... später ...
app->Stats()->close();  // ❌ Muss manuell aufgerufen werden
```

**NACHHER (RAII Pattern):**
```cpp
// stats.h
class TStats {
public:
    TStats();   // ✅ Automatisch beim Erstellen
    ~TStats();  // ✅ Automatisch beim Löschen
private:
    std::vector<XPLMDataRef> dataRefs;
};

// stats.cpp
TStats::TStats() {
    LOG("TStats: Initializing DataRefs");
    dataRefs.push_back(XPLMRegisterDataAccessor("inav/yaw", ...));
    dataRefs.push_back(XPLMRegisterDataAccessor("inav/pitch", ...));
    LOG("TStats: Ready");
}

TStats::~TStats() {
    LOG("TStats: Cleaning up DataRefs");
    for (auto ref : dataRefs) {
        XPLMUnregisterDataAccessor(ref);  // Automatisch beim Löschen
    }
    dataRefs.clear();
    LOG("TStats: Cleaned");
}

// In main.cpp
ApplicationContext::Initialize();  // ✅ TStats Constructor läuft automatisch
// ... später ...
ApplicationContext::Reset();       // ✅ TStats Destructor läuft automatisch
```

### Beispiel 2: Komponente mit Dependencies (TOSD)

**VOR:**
```cpp
// osd.h
class TOSD {
public:
    void init();        // MUSS nach SimData::init() aufgerufen werden!
    void destroy();
private:
    TSimData* simData;  // Raw pointer - unsafe!
    EventBus* eventBus;
};

// In main.cpp
app->SimData()->init();      // Init-Reihenfolge ist wichtig!
app->OSD()->init();          // OSD MUSS NACH SimData initialisiert sein
                             // ❌ Nichts garantiert diese Reihenfolge!

// Cleanup
app->OSD()->destroy();       // Destroy-Reihenfolge ist wichtig!
app->SimData()->destroy();   // OSD MUSS VOR SimData zerstört werden
                             // ❌ Leicht falsche Reihenfolge möglich!
```

**NACHHER:**
```cpp
// osd.h
class TOSD {
public:
    // Constructor mit Abhängigkeiten
    TOSD(std::shared_ptr<EventBus> eventBus,
         std::shared_ptr<TSimData> simData);
    ~TOSD();
private:
    std::shared_ptr<EventBus> _eventBus;  // Safe - shared_ptr!
    std::shared_ptr<TSimData> _simData;
};

// osd.cpp
TOSD::TOSD(std::shared_ptr<EventBus> eventBus,
           std::shared_ptr<TSimData> simData)
    : _eventBus(eventBus), _simData(simData)
{
    LOG("TOSD: Initializing");
    
    // Subscribe auf Events
    _eventBus->Subscribe<SimulatorConnectedEvent>(
        [this](const auto& e) { this->onConnected(e); }
    );
    
    // Nun wissen wir dass simData garantiert initialisiert ist
    if (_simData->isAirplane) {
        LOG("TOSD: Airplane detected, initializing OSD");
    }
}

TOSD::~TOSD() {
    LOG("TOSD: Cleaning up");
    // EventBus & SimData sind NOCH VERFÜGBAR wenn OSD gelöscht wird!
    // (weil sie shared_ptr sind und andere Referenzen haben)
}

// In ApplicationContext::ApplicationContext()
_simData = std::make_shared<TSimData>();
_osd = std::make_shared<TOSD>(_eventBus, _simData);
// ✅ Reihenfolge ist garantiert korrekt!

// In ApplicationContext::~ApplicationContext()
// Destruktoren laufen in umgekehrter Reihenfolge
_osd.reset();      // OSD Destruktor - simData ist noch verfügbar
_simData.reset();  // SimData Destruktor
// ✅ Korrekte Cleanup-Reihenfolge automatisch!
```

### Beispiel 3: Mit Event-Subscription Cleanup

```cpp
// graph.h
class TGraph {
public:
    TGraph(std::shared_ptr<EventBus> eventBus);
    ~TGraph();
private:
    std::shared_ptr<EventBus> _eventBus;
    // TODO: EventListener handle speichern um zu unsubscribe
};

// graph.cpp
TGraph::TGraph(std::shared_ptr<EventBus> eventBus)
    : _eventBus(eventBus)
{
    LOG("TGraph: Subscribing to events");
    
    _eventBus->Subscribe<SensorDataUpdatedEvent>(
        [this](const auto& e) { this->onSensorDataUpdated(e); }
    );
    
    _eventBus->Subscribe<FlightLoopEvent>(
        [this](const auto& e) { this->onFlightLoop(e); }
    );
}

TGraph::~TGraph() {
    LOG("TGraph: Unsubscribing from events");
    
    // ZUKÜNFTIGE VERBESSERUNG:
    // Wenn EventBus::Unsubscribe() implementiert wird:
    // _eventBus->Unsubscribe(sensorDataHandle);
    // _eventBus->Unsubscribe(flightLoopHandle);
    
    // Für jetzt: EventBus::Clear() wird vom ApplicationContext aufgerufen
    // nach allen Komponenten-Destruktoren
}
```

## Initialisierungs-Reihenfolge

**Wichtig:** Abhängigkeiten müssen in korrekter Reihenfolge initialisiert werden:

```cpp
ApplicationContext::ApplicationContext() {
    // 1. BASIS-KOMPONENTEN zuerst (keine Abhängigkeiten)
    _eventBus = std::make_shared<EventBus>();
    
    // 2. UNABHÄNGIGE KOMPONENTEN
    _mspConnection = std::make_shared<MSP>();
    _simData = std::make_shared<TSimData>();
    
    // 3. ABHÄNGIGE KOMPONENTEN (übergeben ihre Dependencies)
    _osd = std::make_shared<TOSD>(_eventBus, _simData);
    _graph = std::make_shared<TGraph>(_eventBus, _simData);
    
    // 4. WEITERE ABHÄNGIGKEITEN
    _stats = std::make_shared<TStats>(_eventBus);
    _menu = std::make_shared<TMenu>(_eventBus, _simData, _osd);
}

~ApplicationContext() {
    // UMGEKEHRTE REIHENFOLGE (wichtig!)
    _menu.reset();      // Braucht OSD, SimData, EventBus
    _stats.reset();     // Braucht EventBus
    _graph.reset();     // Braucht EventBus, SimData
    _osd.reset();       // Braucht EventBus, SimData
    _simData.reset();   // Unabhängig
    _mspConnection.reset();
    _eventBus.reset();
}
```

## Transition Plan

### Phase A: Bestehender Code
```cpp
// AKTUELL: Gemischtes Pattern
ApplicationContext::Initialize();
auto app = App();

app->OSD()->init();           // Init-Methode
app->Stats()->init();
// ...

// ... später ...

app->OSD()->destroy();        // Destroy-Methode
app->Stats()->close();
```

### Phase B: Hybrid (mit LegacyAdapter)
```cpp
// WÄHREND MIGRATION:
// Neue Komponenten haben Constructoren/Destruktoren
class TNewComponent {
public:
    TNewComponent(std::shared_ptr<EventBus> eventBus);
    ~TNewComponent();
};

// Alte Komponenten verwenden noch init()/destroy()
// LegacyAdapter wrapper können helfen
```

### Phase C: Vollständige RAII
```cpp
// NACH MIGRATION:
ApplicationContext::Initialize();  // Alles automatisch initialisiert
// ... work ...
ApplicationContext::Reset();       // Alles automatisch aufgeräumt

// Keine expliziten init/destroy Aufrufe mehr!
```

## Best Practices

### ✅ DO: Constructor für Initialisierung

```cpp
class MyComponent {
public:
    MyComponent(std::shared_ptr<EventBus> eventBus)
        : _eventBus(eventBus)
    {
        // Initialisiere Ressourcen
        _resource = acquireResource();
        
        // Subscribe auf Events
        _eventBus->Subscribe<MyEvent>([this](const auto& e) {
            this->handle(e);
        });
        
        LOG("MyComponent ready");
    }
    
    ~MyComponent() {
        // Cleanup automatisch
        releaseResource(_resource);
        LOG("MyComponent cleaned");
    }
};
```

### ❌ DON'T: Init-Methoden mit Hidden Dependencies

```cpp
class BadComponent {
private:
    EventBus* _eventBus;  // ❌ Wer setzt das?
    
public:
    void init() {  // ❌ Muss manuell aufgerufen werden
        _eventBus = App()->GetEventBus().get();  // ❌ Globale Abhängigkeit!
    }
};
```

### ✅ DO: Member-Initialisierung im Constructor

```cpp
class Component {
private:
    std::vector<Resource> _resources;
    
public:
    Component() {
        // Initialisiere Member-Variablen
        _resources.reserve(10);
        _resources.push_back(createResource());
    }
    
    ~Component() {
        // Automatisches Cleanup durch Destruktoren der Member-Variablen
        _resources.clear();
    }
};
```

### ✅ DO: Exception Safety

```cpp
class SafeComponent {
private:
    std::unique_ptr<Resource> _resource;
    
public:
    SafeComponent() {
        try {
            _resource = std::make_unique<Resource>();
            LOG("SafeComponent initialized");
        } catch (const std::exception& e) {
            LOG("SafeComponent initialization failed: %s", e.what());
            throw;  // Propagate, destructor will clean up
        }
    }
    
    ~SafeComponent() noexcept {
        // Destruktor läuft GARANTIERT, auch bei Exception
        _resource.reset();
    }
};
```

## Zusammenfassung

| Aspekt | init()/destroy() | Constructor/Destructor |
|--------|------------------|----------------------|
| **Automatisch** | ❌ Manuell | ✅ Automatisch |
| **Exception Safe** | ❌ Nein | ✅ Ja (RAII) |
| **Reihenfolge** | ❌ Fehleranfällig | ✅ Garantiert |
| **Skalierbar** | ❌ Neue Aufrufe | ✅ Automatisch |
| **Memory Safe** | ❌ Wenn vergessen | ✅ Garantiert |
| **Code Clarity** | ❌ Init/Destroy separat | ✅ Mit Konstruktion |

**Ergebnis:** RAII (Constructor/Destructor) ist der moderne, sichere C++ Standard! 🚀
