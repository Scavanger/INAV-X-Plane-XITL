#include "GraphSelectWindow.h"

#include "../settings/Settings.h"
#include "../settings/SettingNames.h"
#include "../core/PluginContext.h"
#include "../core/EventBus.h"

std::shared_ptr<GraphSelectWindow> GraphSelectWindow::instance = nullptr;

GraphSelectWindow::GraphSelectWindow(int left, int top, int right, int bot, XPLMWindowDecoration decoration, XPLMWindowLayer layer) : ImgWindow(left, top, right, bot, decoration, layer)
{
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = nullptr; // Disable imgui ini file

    SetWindowTitle("Graph Select");
    SetVisible(true);
}

GraphSelectWindow::~GraphSelectWindow()
{
    instance.reset();
}

void GraphSelectWindow::loadSettings()
{
    selectedGraphType = Plugin()->Settings()->GetSettingAs<int>(SettingsSections::SECTION_GRAPH, SettingsKeys::SETTINGS_GRAPH_TYPE, 0);
}

void GraphSelectWindow::buildInterface()
{
    ImGui::Text("Select Graph Type:");
    if (ImGui::Combo("Graph Type", &selectedGraphType, GRAPH_TYPES, IM_ARRAYSIZE(GRAPH_TYPES)))
    {
        Plugin()->GetEventBus()->Publish<GraphTypeChangedEventArg>("SetGraphType", GraphTypeChangedEventArg(selectedGraphType));
    }

    ImGui::Dummy(ImVec2(0.0f, 20.0f));
    if (ImGui::Button("Close", ImVec2(100.0f, 30.0f)))
    {
        
        Plugin()->Settings()->SetSetting(SettingsSections::SECTION_GRAPH, SettingsKeys::SETTINGS_GRAPH_TYPE, selectedGraphType);
        Plugin()->Settings()->save();
        Plugin()->GetEventBus()->Publish("MenuOpenCloseGraph");
        SetVisible(false);
    }
}
