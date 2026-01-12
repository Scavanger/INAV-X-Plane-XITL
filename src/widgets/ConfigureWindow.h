#pragma once

#include "../platform.h"

#include "SettingsWindow.h"
#include "GraphSelectWindow.h"

#include <ImgWindow.h>
#include <fa-solid-900.inc>
#include <IconsFontAwesome5.h>

constexpr float FONT_SIZE = 15.0f;

namespace ConfigureImgWindow
{
    static void configure()
    {
        ImgWindow::sFontAtlas = std::make_shared<ImgFontAtlas>();

        ImgWindow::sFontAtlas->AddFontFromFileTTF("./Resources/fonts/DejaVuSansMono.ttf", FONT_SIZE);

        // Now we merge some icons from the OpenFontsIcons font into the above font
        // (see `imgui/docs/FONTS.txt`)
        ImFontConfig config;
        config.MergeMode = true;

        // We only read very selectively the individual glyphs we are actually using
        // to safe on texture space
        static ImVector<ImWchar> icon_ranges;
        ImFontGlyphRangesBuilder builder;

        // Add all icons that are actually used (they concatenate into one string)
        builder.AddText(reinterpret_cast<const char *>(ICON_FA_TRASH_ALT ICON_FA_SEARCH
                                                        ICON_FA_EXTERNAL_LINK_SQUARE_ALT
                                                        ICON_FA_WINDOW_MAXIMIZE ICON_FA_WINDOW_MINIMIZE
                                                        ICON_FA_WINDOW_RESTORE ICON_FA_WINDOW_CLOSE));

        builder.BuildRanges(&icon_ranges);

        // Merge the icon font with the text font
        ImgWindow::sFontAtlas->AddFontFromMemoryCompressedTTF(fa_solid_900_compressed_data,
                                                            fa_solid_900_compressed_size,
                                                            FONT_SIZE,
                                                            &config,
                                                            icon_ranges.Data);


        int screenLeft, screenTop, screenRight, screenBottom;
        XPLMGetScreenBoundsGlobal(&screenLeft, &screenTop, &screenRight, &screenBottom);
    
        const int settingsPad = 75;
        const int settingWidth = 650;
        const int settigsHeight = 625;

        int left = screenLeft + settingsPad;
        int right = left + settingWidth;
        int top = screenTop - settingsPad;
        int bottom = top - settigsHeight;
            
        SettingsWindow::Instance = std::make_shared<SettingsWindow>(left, top, right, bottom);
        SettingsWindow::Instance->SetVisible(false);

        Plugin()->GetEventBus()->Subscribe("MenuOpenSettings", []()
        {
            SettingsWindow::Instance->loadSettings();
            SettingsWindow::Instance->SetVisible(true); 
        });

        const int graphPadTop = 420;
        const int graphPadRight = 20;
        const int graphWidth = 400;
        const int graphHeight = 115;

        left = screenRight - graphPadRight - graphWidth;
        right = left + graphWidth;
        top = screenTop - graphPadTop;
        bottom = top - graphHeight;

        GraphSelectWindow::instance = std::make_shared<GraphSelectWindow>(left, top, right, bottom, xplm_WindowDecorationNone, xplm_WindowLayerFlightOverlay);
        GraphSelectWindow::instance->SetVisible(false);
        Plugin()->GetEventBus()->Subscribe("MenuOpenCloseGraph", []()
        {
           GraphSelectWindow::instance->loadSettings();
           GraphSelectWindow::instance->SetVisible(true); 
        });
    }

    static void cleanup()
    {
        ImgWindow::sFontAtlas.reset();
    }
}