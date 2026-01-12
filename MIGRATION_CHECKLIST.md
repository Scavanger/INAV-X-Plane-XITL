/**
 * @file MIGRATION_CHECKLIST.md
 * @brief Schritt-für-Schritt Migration vom globalen Ansatz zu PluginContext
 */

# Migration Checkliste: Von Globals zu PluginContext

## Voraussetzungen
- ✅ EventBus.h implementiert
- ✅ PluginContext.h/cpp implementiert
- ✅ LegacyAdapter.h implementiert
- ✅ main_refactored.cpp als Vorlage
- ✅ refactoring_examples.cpp als Referenz

## Phase 1: Basis-Setup (einzelne Komponente migrieren)

### Schritt 1: Wähle eine Komponente
Empfehlung: Beginne mit einer Komponente mit wenigen Abhängigkeiten.
Nicht geeignet: Menu und Graph (zu viele Abhängigkeiten)

### Schritt 2: Dokumentiere aktuelle Abhängigkeiten
```bash
# Finde alle g_* Zugriffe in der Komponente:
grep -n "g_" src/simData.cpp
```

### Schritt 3: Aktualisiere den Header
```cpp
// Füge PluginContext Zugriff hinzu
#include "core/PluginContext.h"

// ENTFERNE:
// extern TSimData g_simData;
// extern TOSD g_osd;

// Die Komponente wird jetzt PluginContext abfragen, wenn nötig
```

### Schritt 4: Aktualisiere die Implementierung
```cpp
// ALT:
void TMyComponent::doSomething() {
    g_simData.update();
    g_osd.draw();
}

// NEU:
void TMyComponent::doSomething() {
    auto app = Plugin();
    app->SimData()->update();
    app->OSD()->draw();
}
```

### Schritt 5: Registriere Event-Listener in init()
```cpp
void TMyComponent::init() {
    auto app = Plugin();
    
    // Subscribe auf relevante Events
    app->EventBus()->Subscribe<SimulatorConnectedEvent>(
        [this](const auto& e) { this->onSimulatorConnected(e); }
    );
}
```

### Schritt 6: Teste Kompilierung
```bash
mkdir -p build && cd build
cmake -GNinja -DOUTPUT_DIR="..." ..
ninja
```

## Phase 2: Komponenten-Dependencies entfernen

### Für jede Komponente:

1. **Header prüfen** (z.B. `simData.h`)
   ```cpp
   // ENTFERNE diese Zeilen:
   extern TSimData g_simData;
   extern TMSPConnection g_msp;
   // ... etc
   ```

2. **Implementierung prüfen** (z.B. `simData.cpp`)
   ```cpp
   // ENTFERNE diese Zeile:
   TSimData g_simData;
   
   // PluginContext erstellt jetzt diese Objekte
   ```

3. **Abhängige Code-Stellen aktualisieren**
   ```cpp
   // Finde: grep -r "g_simData" src/
   // Ersetze mit: Plugin()->SimData()
   ```

## Phase 3: Main.cpp migrieren

1. **Kopiere Vorlage**
   ```bash
   cp src/main_refactored.cpp src/main.cpp
   ```

2. **Behalte X-Plane Integration**
   - Keep `lastUpdateTime`, `should_wait`, `firstRender`
   - Keep `loopId`, `ini` (falls nötig)
   - Diese sind X-Plane spezifisch

3. **Testa Compilation und Plugin Loading**
   - X-Plane sollte beim Start das Plugin laden
   - Keine Fehler in der Developer Console

## Phase 4: Event-basierte Kommunikation

### Für jede Komponenten-Interaktion:

**VORHER:**
```cpp
// Component A
void A::update() {
    g_b.notify();  // A ruft B auf
}

// Component B ruft zurück zu A
extern void handleFromB();
```

**NACHHER:**
```cpp
// Component A: publish Event
void A::update() {
    Plugin()->EventBus()->Publish(ComponentAUpdatedEvent{/*...*/});
}

// Component B: subscribe
void B::init() {
    Plugin()->EventBus()->Subscribe<ComponentAUpdatedEvent>(
        [this](const auto& e) { this->handleUpdate(e); }
    );
}
```

## Phase 5: Cleanup

### Nach erfolgreicher Migration:

1. **Entferne alte Externs**
   ```bash
   grep -r "extern.*g_" src/ --include="*.h"
   # Diese sollten alle verschwunden sein
   ```

2. **Entferne Dummy-Globale aus .cpp**
   ```bash
   grep -r "^[A-Z][a-zA-Z]* g_" src/ --include="*.cpp"
   # Diese sollten alle verschwunden sein
   ```

3. **Lösche LegacyAdapter.h**
   ```bash
   rm src/core/LegacyAdapter.h
   ```

4. **Aktualisiere CMakeLists.txt**
   - Entferne `PluginContext.cpp` Kommentare
   - Finalisiere die neuen Pfade

## Migration Tracking

| Komponente | Status | Datum | Notes |
|-----------|--------|-------|-------|
| simData | [ ] todo | | |
| msp | [ ] todo | | |
| osd | [ ] todo | | |
| graph | [ ] todo | | |
| menu | [ ] todo | | |
| stats | [ ] todo | | |
| map | [ ] todo | | |
| sound | [ ] todo | | |
| serial/ | [ ] todo | | |

## Häufige Probleme und Lösungen

### Problem: "PluginContext nicht initialisiert"
**Ursache:** `PluginContext::Initialize()` wird nicht in `XPluginEnable()` aufgerufen
**Lösung:** Stelle sicher dass `XPluginEnable()` PluginContext initialisiert:
```cpp
PLUGIN_API int XPluginEnable(void) {
    PluginContext::Initialize();  // ← MUSS hier stehen
    // ...
}
```

### Problem: "Zirkuläre Abhängigkeit während Kompilierung"
**Ursache:** Header Includes in falscher Reihenfolge
**Lösung:** Verwende Forward Declarations in Headers:
```cpp
// header.h
class EventBus;  // Forward declare
class PluginContext;

class MyComponent {
    std::shared_ptr<EventBus> eventBus;  // OK
};
```

### Problem: "Events werden nicht ausgelöst"
**Ursache:** Listener ist nicht registriert
**Lösung:** 
```cpp
// RICHTIG:
void init() {
    Plugin()->EventBus()->Subscribe<MyEvent>([this](const auto& e) { 
        this->handle(e); 
    });
}

// FALSCH:
void init() {
    // Listener wird sofort registriert und vergessen:
    Plugin()->EventBus()->Subscribe<MyEvent>([](const auto& e) { 
        // danach reagiert keine Komponente
    });
}
```

### Problem: "Memory Leak bei Event Listeners"
**Ursache:** Lambda hat `this` Pointer, aber Komponente wird gelöscht
**Lösung:** Listener deregistrieren (noch nicht implementiert, aber geplant):
```cpp
// Zukünftige Verbesserung:
auto handle = Plugin()->EventBus()->Subscribe<MyEvent>([this](...) { });
// Im Destruktor:
Plugin()->EventBus()->Unsubscribe(handle);
```

## Hilfreiche Kommandos

```bash
# Finde alle extern Deklarationen
grep -r "extern [A-Z].*g_" src/ --include="*.h"

# Finde alle Globale Definitionen
grep -r "^[A-Z].* g_" src/ --include="*.cpp"

# Finde alle g_* Zugriffe
grep -r "g_[a-zA-Z]*\." src/

# Zähle Migrierte vs. Nicht-Migrierte Komponenten
grep -r "g_" src/ --include="*.cpp" | wc -l

# Kompiliere und zeige nur Fehler
ninja 2>&1 | grep -E "error:"
```

## Vollständige Migration Checkliste

- [ ] Phase 1: Basis-Setup abgeschlossen
- [ ] Phase 2: Alle extern Deklarationen entfernt
- [ ] Phase 3: main.cpp migriert und funktioniert
- [ ] Phase 4: Events implementiert für kritische Interaktionen
- [ ] Phase 5: Cleanup durchgeführt
- [ ] Keine `g_*` Zugriffe mehr vorhanden
- [ ] Alle Tests bestanden
- [ ] Code-Review durchgeführt
- [ ] LegacyAdapter.h gelöscht
- [ ] Dokumentation aktualisiert

## Nächste Schritte nach Migration

1. **Unit Tests schreiben**
   - `PluginContext::CreateForTesting()` verwenden
   - Mock-Komponenten testen

2. **Performance Profiling**
   - Überprüfe Event-Loop Performance
   - Stelle sicher dass kein Memory Leak auftritt

3. **Integration Tests**
   - Test X-Plane Integration
   - Test FC Communication

4. **Code-Review und Merge**
   - Peer-Review durchführen
   - In Main Branch mergen
