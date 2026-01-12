# Refaktorierung: Von globalen Objekten zu PluginContext

## Überblick

Diese Refaktorierung ersetzt die globalen `g_*` Objekte durch ein sauberes Dependency-Injection-System mit einem zentralen `PluginContext` und einem `EventBus` für Event-basierte Kommunikation.

## Architektur

### Vorher (alte Architektur)
```cpp
// Header-Deklarationen
extern TSimData g_simData;
extern MSP g_msp;
extern TOSD g_osd;
// ... weitere globale Objekte

// Verwendung
void someFunction() {
    g_simData.updateFromXPlane();
    g_osd.drawCallback();
    g_msp.loop();
}
```

**Probleme:**
- Direkte globale Abhängigkeiten schwer zu testen
- Keine klaren Abhängigkeitsrichtungen
- Schwierig zu verstehen welche Komponenten miteinander kommunizieren
- Keine einfache Möglichkeit für Event-basierte Kommunikation

### Nachher (neue Architektur)
```cpp
// PluginContext verwaltet alle Komponenten
PluginContext::Initialize();

// Kurze Accessor-Funktion
auto app = Plugin();

// Verwendung mit Smart Pointers
void someFunction() {
    Plugin()->SimData()->updateFromXPlane();
    Plugin()->OSD()->drawCallback();
    Plugin()->MspConnection()->loop();
}

// Oder mit Event-basierter Kommunikation
Plugin()->EventBus()->Subscribe<SimulatorConnectedEvent>([](const auto& event) {
    LOG("Simulator verbunden!");
});
```

**Vorteile:**
- Alle Abhängigkeiten durch PluginContext
- Smart Pointers für automatisches Memory Management
- Event-Bus für lose Kopplung
- Einfach zu testen (PluginContext::CreateForTesting())

## Schrittweise Migration

### Phase 1: Basis-Setup (abgeschlossen)
- ✅ `EventBus.h` - Event-System implementiert
- ✅ `PluginContext.h/cpp` - Zentrale Verwaltung
- ✅ `LegacyAdapter.h` - Rückwärtskompatibilität

### Phase 2: Globale Deklarationen entfernen
Nächste Schritte:
1. Entfernen Sie aus jeder Komponente die `extern g_*` Deklarationen
2. Aktualisieren Sie `main.cpp` um PluginContext zu initialisieren
3. Ersetzen Sie `g_*` Zugriffe mit `Plugin()->Component()`

### Phase 3: Event-basierte Kommunikation einführen
Beispiel für `cbMessage` Refaktorierung:
```cpp
// ALT:
void cbMessage(int code, const uint8_t* messageBuffer, unsigned int length) {
    if (code == MSP_SIMULATOR) {
        g_simData.updateFromINAV(&msg);
        g_osd.updateFromINAV(&msg);
        g_map.onWP(&msg);
    }
}

// NEU:
void cbMessage(int code, const uint8_t* messageBuffer, unsigned int length) {
    auto app = Plugin();
    if (code == MSP_SIMULATOR) {
        auto event = MSPSimulatorDataReceivedEvent{messageBuffer, length};
        app->EventBus()->Publish(event);
        
        // Jede Komponente subscribet separat:
        // - SimData subscribet und ruft updateFromINAV auf
        // - OSD subscribet und ruft updateFromINAV auf
        // - Map subscribet und verarbeitet ihre Daten
    }
}
```

### Phase 4: Komponenten refaktorieren
Jede Komponente sollte:
1. Abhängigkeiten vom PluginContext abrufen (nicht global)
2. Event-Listener in `init()` registrieren
3. Events publishen statt direkt andere Komponenten aufzurufen

## Migrations-Pattern

### Pattern 1: Direkte Abhängigkeit → Injection
```cpp
// ALT (schlecht - versteckte globale Abhängigkeit)
class OSD {
    void updateFromINAV() {
        g_simData.sendToXPlane();  // Wo kommt das her?
    }
};

// NEU (gut - explizit)
class OSD {
private:
    std::shared_ptr<TSimData> simData;
public:
    OSD(std::shared_ptr<TSimData> data) : simData(data) {}
    void updateFromINAV() {
        simData->sendToXPlane();   // Klar wo das herkommt
    }
};
```

### Pattern 2: Zustands-Abfragen → Events
```cpp
// ALT
void floop_cb(...) {
    if (g_msp.isConnected()) {  // Polling
        // ...
    }
}

// NEU
void init() {
    Plugin()->EventBus()->Subscribe<SimulatorConnectedEvent>([this](const auto& e) {
        if (e.isConnected) {
            onConnected();
        }
    });
}
```

### Pattern 3: Bidirektionale Kommunikation → Event Chain
```cpp
// ALT (Komponente A ruft B auf, B ruft A auf → zirkulär)
void A::update() {
    g_b.doSomething();  // B aufrufen
}

void B::doSomething() {
    g_a.onBUpdated();   // Zurück zu A
}

// NEU (über Events)
void A::init() {
    Plugin()->EventBus()->Subscribe<BUpdatedEvent>([this](const auto& e) {
        onBUpdated();
    });
}

void B::update() {
    Plugin()->EventBus()->Publish(BUpdatedEvent{...});
}
```

## Transition Tipps

### 1. Verwenden Sie Legacy-Adapter temporär
```cpp
// Während Migration, können Sie diese verwenden:
#include "core/LegacyAdapter.h"

GetSimData().updateFromXPlane();  // Statt g_simData
GetOSD().drawCallback();          // Statt g_osd
```

### 2. Kompilieren mit neuer Architektur
1. `PluginContext::Initialize()` in `XPluginEnable()` aufrufen
2. Ändern Sie nicht alle Dateien auf einmal
3. Schrittweise refaktorieren

### 3. Events für neue Features
Für neue Features verwenden Sie direkt Events statt neue globale Variablen:
```cpp
// Nicht: extern TNeueFunktion g_neueFunktion;
// Sondern:
class NeueFunktonEvent { /* ... */ };
Plugin()->EventBus()->Publish(NeueFunktonEvent{...});
```

## Testing

Mit der neuen Architektur sind Tests viel einfacher:

```cpp
#include "core/PluginContext.h"

void TestSimulatorConnection() {
    // Erstelle isolierten Context für Test
    auto testContext = PluginContext::CreateForTesting();
    
    // Oder verwende Mocks:
    auto mockSimData = std::make_shared<MockSimData>();
    // ... setup mock
    
    // Test läuft in isolierter Umgebung
}
```

## Checkliste für vollständige Migration

- [ ] Alle `extern g_*` Deklarationen in Header-Dateien entfernen
- [ ] `PluginContext::Initialize()` in `XPluginEnable()` aufrufen
- [ ] `PluginContext::Reset()` in `XPluginDisable()` aufrufen
- [ ] Globale Objekte (`g_simData`, etc.) aus `.cpp` Dateien entfernen
- [ ] Alle `g_*` Zugriffe durch `Plugin()->Component()` ersetzen
- [ ] Event-Listener in Komponenten-Init registrieren
- [ ] Events statt direkte Aufrufe zwischen Komponenten verwenden
- [ ] LegacyAdapter.h kann dann gelöscht werden

## Weiterlesen

- `src/core/EventBus.h` - Event System Dokumentation
- `src/core/PluginContext.h` - Dependency Injection
- `.github/copilot-instructions.md` - Überarbeitete Architektur-Dokumentation
