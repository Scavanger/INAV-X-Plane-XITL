#pragma once

#include "../platform.h"

#include "EventBus.h"

// Forward-Declarations statt Includes (verhindert zirkuläre Dependencies)
class Settings;
class Menu;
class Fonts;
class MSP;
class SimData;
class OSD;
class Graph;
class DataRefs;
class Map;

#include <memory>
#include <stdexcept>

/**
 * @brief Application Context - Zentrale Verwaltung aller Komponenten
 * 
 * Diese Klasse ersetzen die globalen g_* Objekte und stellt eine saubere
 * Abhängigkeits-Injektions-Architektur bereit. Sie können auf mehrere
 * PluginContext-Instanzen für Tests erstellen.
 * 
 * Verwendung:
 *   auto app = PluginContext::Instance();
 *   app->SimData()->updateFromXPlane();
 *   app->EventBus()->Subscribe<SimulatorConnectedEvent>([...]);
 */

class PluginContext
{
private:
    // Singleton-Instanz
    static std::unique_ptr<PluginContext> instance;
    
    // Komponenten als Smart Pointers
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

    /**
     * @brief Private Konstruktor für Singleton Pattern
     */
    PluginContext();

public:
    /**
     * @brief Destruktor
     */
    ~PluginContext();

    /**
     * @brief Initialisiert die Singleton-Instanz
     * Muss vor dem ersten Aufruf von Instance() erfolgen
     * 
     * @throws std::runtime_error wenn bereits initialisiert
     */
    static void Initialize();

    /**
     * @brief Liefert die Singleton-Instanz
     * 
     * @throws std::runtime_error wenn nicht initialisiert
     * @return Zeiger auf die globale PluginContext Instanz
     */
    static PluginContext* Instance();

    /**
     * @brief Setzt die Singleton-Instanz zurück (vor allem für Tests)
     */
    static void Reset();

    /**
     * @brief Liefert den EventBus
     * Wird für Event-basierte Kommunikation zwischen Komponenten verwendet
     */
    std::shared_ptr<::EventBus> GetEventBus() const { return _eventBus; }

    /**
     * @brief Liefert das Fonts Management System
     */
    std::shared_ptr<::Fonts> Fonts() const { return _fonts; }


    /**
     * @brief Liefert das Menu-System
     */
    std::shared_ptr<::Menu> Menu() const { return _menu; }

    /**
     * @brief Liefert die Map-Komponente
     */
    //std::shared_ptr<TMap> Map() const { return _map; }

    std::shared_ptr<::Settings> Settings() const { return _settings; }

    /**
     * @brief Test-Hilfsmethode: Erstellt einen isolierten Context für Tests
     * Diese Instanz ist nicht die Singleton-Instanz
     * 
     * @return Ein neuer, unabhängiger PluginContext für Tests
     */
    static PluginContext CreateForTesting();
};

/**
 * @brief Komfort-Funktion um auf den PluginContext zuzugreifen
 * 
 * Equivalent zu PluginContext::Instance(), aber mit kürzerem Namen
 * für häufige Verwendung
 * 
 * @return Zeiger auf die PluginContext Instanz
 */
inline PluginContext* Plugin()
{
    return PluginContext::Instance();
}
