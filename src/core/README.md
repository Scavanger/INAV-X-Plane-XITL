# Core Architecture Components

## Übersicht

Der `src/core/` Ordner enthält die neuen Architektur-Komponenten für die Refaktorierung von globalen Objekten zu einem sauberen Dependency-Injection-System.

## Komponenten

### EventBus.h
**Typ-sicheres Event-System mit Observer Pattern**

Ermöglicht lose Kopplung zwischen Komponenten durch Event-basierte Kommunikation.

```cpp
// Subscribe auf Events
Plugin()->EventBus()->Subscribe<SimulatorConnectedEvent>(
    [](const auto& event) {
        LOG("Simulator connected: %d", event.isConnected);
    }
);

// Publish Events
SimulatorConnectedEvent event{true};
Plugin()->EventBus()->Publish(event);
```

**Vordefinierte Events:**
- `SimulatorConnectedEvent` - FC Verbindung hergestellt/unterbrochen
- `MSPSimulatorDataReceivedEvent` - Neue MSP Daten empfangen
- `SensorDataUpdatedEvent` - Sensor-Daten aktualisiert
- `OSDStatusChangedEvent` - OSD Status Änderung
- `FlightLoopEvent` - Flight Loop Iteration
- `SimulatorErrorEvent` - Fehler im Simulator

**Custom Events definieren:**
```cpp
// In EventBus.h
class MyCustomEvent {
public:
    std::string message;
    int value = 0;
};

// In deiner Komponente
Plugin()->EventBus()->Subscribe<MyCustomEvent>([this](const auto& e) {
    LOG("Custom event: %s = %d", e.message.c_str(), e.value);
});

Plugin()->EventBus()->Publish(MyCustomEvent{"test", 42});
```

### PluginContext.h/cpp
**Zentrale Verwaltung aller Komponenten (Dependency Injection)**

Verwaltet alle Plugin-Komponenten als Smart Pointers und stellt sie über Getter-Methoden bereit.

```cpp
// Einmalig initialisieren (in XPluginEnable)
PluginContext::Initialize();

// Überall verwenden
auto app = Plugin();  // Kurz-Funktion
app->SimData()->updateFromXPlane();
app->OSD()->drawCallback();
app->MspConnection()->loop();
```

**Verfügbare Komponenten:**
- `EventBus()` - Event-System
- `MspConnection()` - MSP/FC Kommunikation
- `SimData()` - Sensor und Simulations-Daten
- `OSD()` - On-Screen Display System
- `Graph()` - Debug-Graphen
- `Stats()` - Performance-Statistiken
- `Menu()` - X-Plane Menü
- `Sound()` - Audio-System
- `Map()` - Waypoint-Anzeige
- `IPInputWidget()` - TCP IP-Eingabe Widget
- `INI()` - Konfigurationsstruktur

**Singleton Pattern:**
```cpp
// Nur einmal initialisieren
if (!PluginContext::Instance()) {
    PluginContext::Initialize();
}

// Überall anonym zugreifen
auto app = Plugin();  // Plugin() ist shortcut für Instance()
```

**Testing:**
```cpp
// Für Tests: Isolierter Context ohne Singleton
auto testCtx = PluginContext::CreateForTesting();
```

### LegacyAdapter.h
**Rückwärtskompatibilität während Migration**

Enthält Inline-Getter-Funktionen die auf PluginContext delegieren. Ermöglicht Nutzung des alten `g_*` Stils während schrittweise Migration stattfindet.

```cpp
// Diese Funktionen funktionieren vorübergehend:
GetSimData().updateFromXPlane();      // Statt g_simData
GetOSD().drawCallback();              // Statt g_osd
GetMSP().loop();                      // Statt g_msp
GetEventBus().Publish(event);         // Statt EventBus direkt
```

**Status: DEPRECATED**
Diese Funktionen sollten nach Refaktorierung nicht mehr verwendet werden. Sie sind nur für Übergangsphase gedacht.

## Verwendungs-Pattern

### Pattern 1: Komponenten-Initialisierung

```cpp
// In main.cpp XPluginEnable()
PLUGIN_API int XPluginEnable(void) {
    PluginContext::Initialize();
    
    auto app = Plugin();
    
    // Register event listeners
    app->EventBus()->Subscribe<SimulatorConnectedEvent>(
        OnSimulatorConnected
    );
    
    // Initialize components
    app->OSD()->init();
    app->SimData()->init();
    app->Menu()->createMenu();
    
    return 1;
}
```

### Pattern 2: Event-basierte Kommunikation

```cpp
// Komponente A publiziert Event
void ComponentA::update() {
    auto app = Plugin();
    ComponentAUpdatedEvent event{/* ... */};
    app->EventBus()->Publish(event);
}

// Komponente B subscribet
void ComponentB::init() {
    auto app = Plugin();
    app->EventBus()->Subscribe<ComponentAUpdatedEvent>(
        [this](const auto& e) { this->onComponentAUpdated(e); }
    );
}
```

### Pattern 3: Abhängigkeitsauflösung

```cpp
// Funktionen die auf mehrere Komponenten zugreifen
void ProcessSimulatorData() {
    auto app = Plugin();
    
    // Alles über PluginContext abfragen
    auto simData = app->SimData();
    auto osd = app->OSD();
    auto stats = app->Stats();
    
    // Arbeiten
    if (simData->isAirplane) {
        osd->updateFromINAV(data);
        stats->recordUpdate();
    }
}
```

## Migration Roadmap

### Phase 1 ✅
- [x] EventBus implementiert
- [x] PluginContext implementiert
- [x] LegacyAdapter implementiert

### Phase 2
- [ ] Globale extern Deklarationen entfernen
- [ ] main.cpp migrieren
- [ ] Erste Komponente komplett refaktorieren

### Phase 3
- [ ] Alle Komponenten refaktorieren
- [ ] Event-basierte Kommunikation einbauen
- [ ] Tests schreiben

### Phase 4
- [ ] LegacyAdapter entfernen
- [ ] Code-Review
- [ ] Final Deployment

## RAII Pattern - Resource Acquisition Is Initialization

**Wichtig für Phase 2:** Komponenten sollten Constructoren/Destructoren verwenden statt init()/destroy().

### Warum RAII?

| Kriterium | init()/destroy() | RAII Constructor/Destructor |
|-----------|------------------|---------------------------|
| **Automatisch** | ❌ Manuell aufrufen | ✅ Automatisch aufgerufen |
| **Exception Safe** | ❌ Kann Memory Leak sein | ✅ Destruktor läuft immer |
| **Asymmetrie-Fehler** | ❌ Leicht zu vergessen | ✅ Immer symmetrisch |
| **Dependencies** | ❌ Manuell setzen | ✅ Via Constructor Parameter |
| **Testbar** | ❌ Schwierig | ✅ Mock Dependencies eingesetzt |

### RAII Komponenten-Template

```cpp
// ✅ MODERN: RAII Pattern
class MyComponent {
public:
    // Constructor: Dependencies via Parameter Injection
    MyComponent(std::shared_ptr<EventBus> eventBus)
        : _eventBus(eventBus)
    {
        LOG("MyComponent: Initializing");
        
        // Hier: Alle Ressourcen initialisieren
        _eventBus->Subscribe<MyEvent>(
            [this](const auto& e) { this->onEvent(e); }
        );
        
        LOG("MyComponent: Ready");
    }
    
    // Destructor: Automatischer Cleanup
    ~MyComponent() {
        LOG("MyComponent: Cleaning up");
        
        // Hier: Alle Ressourcen freigeben
        // EventBus unsubscribe passiert automatisch
        
        LOG("MyComponent: Done");
    }
    
private:
    std::shared_ptr<EventBus> _eventBus;
};
```

### ❌ NICHT: ALT Pattern (init/destroy)

```cpp
class LegacyComponent {
public:
    void init() { /* ... */ }      // ❌ Manuell aufrufen
    void destroy() { /* ... */ }   // ❌ Manuell aufrufen
};
```

### Initialization Order in ApplicationContext

```cpp
// ApplicationContext.cpp
ApplicationContext::ApplicationContext()
    : _eventBus(std::make_shared<EventBus>()),           // 1. Zuerst
      _simData(std::make_shared<TSimData>()),            // 2. Braucht nichts
      _stats(std::make_shared<TStats>(_eventBus)),       // 3. Braucht EventBus
      _graph(std::make_shared<TGraph>(_eventBus, _simData))  // 4. Braucht beide
{
    // Alle Constructoren sind abgelaufen
    // Alle Komponenten sind ready
}

ApplicationContext::~ApplicationContext() {
    // Destructoren laufen in UMGEKEHRTER Reihenfolge:
    // 1. _graph destruiert (benutzt _eventBus & _simData)
    // 2. _stats destruiert (benutzt _eventBus)
    // 3. _simData destruiert
    // 4. _eventBus destruiert
}
```

### Migration: init() → Constructor

**VORHER (Phase 1):**
```cpp
// simData.h
class TSimData {
public:
    void init();  // Init in Code aufrufen
    void close(); // Close in Code aufrufen
};

// main.cpp
void Initialize() {
    g_simData.init();  // ❌ Manuell
    g_osd.init();      // ❌ Manuell
    g_stats.init();    // ❌ Manuell
}
```

**NACHHER (Phase 2):**
```cpp
// simData.h
class TSimData {
public:
    TSimData(std::shared_ptr<EventBus> eventBus)
        : _eventBus(eventBus) { /* ... */ }  // ✅ Constructor
    
    ~TSimData() { /* ... */ }  // ✅ Destructor
};

// main.cpp
void Initialize() {
    PluginContext::Initialize();  // ✅ Alle Constructoren laufen automatisch
}
```

### Exception Safety

```cpp
// ✅ RAII ist Exception Safe
std::shared_ptr<MyComponent> comp;
try {
    comp = std::make_shared<MyComponent>(eventBus);  // Constructor kann werfen
    // Nutze comp
} catch (const std::exception& e) {
    LOG("Failed: %s", e.what());
    // comp destruiert automatisch - kein Leak!
}
```

Weitere Details siehe [RAII_PATTERN.md](../RAII_PATTERN.md).

## Häufig gestellte Fragen

### F: Wo wird PluginContext initialisiert?
**A:** In `main.cpp` Funktion `XPluginEnable()`. Muss VOR allen anderen Initialisierungen passieren.

### F: Kann ich PluginContext in Unit Tests verwenden?
**A:** Ja! Nutze `PluginContext::CreateForTesting()` für isolierte Test-Kontexte.

### F: Was ist mit Speichermanagement?
**A:** PluginContext nutzt `std::shared_ptr`, Speicher wird automatisch freigegeben wenn letzte Referenz gelöscht wird.

### F: Wie registriere ich neue Events?
**A:** Definiere die Event-Klasse in `EventBus.h`, dann use `Plugin()->EventBus()->Subscribe/Publish`.

### F: Sind Events thread-safe?
**A:** Aktuell NEIN. Das ist für Zukunft geplant. Alle Events in main game loop aufrufen.

### F: Was ist der Unterschied zwischen globalen und PluginContext?
**A:**
- Globals: Versteckte Abhängigkeiten, schwer zu testen, zirkuläre Abhängigkeiten möglich
- PluginContext: Explizite Abhängigkeiten, einfach zu testen, klare Struktur

## Performance-Hinweise

- **EventBus**: Subscription ist O(1), Publish ist O(n) where n = Anzahl Subscriber
- **Smart Pointers**: Minimales Overhead, aber nicht free
- **Events**: Keine Memory Allocation, alles Stack-basiert
- **PluginContext**: Singleton, nur einmal pro Plugin-Session erzeugt

## Debug-Tipps

```cpp
// Debugging: PluginContext Status prüfen
auto app = Plugin();
assert(app != nullptr);  // Context muss initialisiert sein

// Event Subscriptions debuggen
// (wird in Zukunft besser)

// Speicherleck prüfen
// PluginContext::Reset() in XPluginDisable()
// sollte alle Komponenten saubern
```

## Weiterlesen

- [REFACTORING.md](../REFACTORING.md) - Vollständiger Refactoring-Leitfaden
- [MIGRATION_CHECKLIST.md](../MIGRATION_CHECKLIST.md) - Schritt-für-Schritt Anleitung
- [src/core/refactoring_examples.cpp](refactoring_examples.cpp) - Code-Beispiele
- [src/main_refactored.cpp](../main_refactored.cpp) - Beispiel refaktorierte main.cpp
