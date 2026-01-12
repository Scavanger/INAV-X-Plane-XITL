#pragma once

#include "platform.h"

#include <math.h>
#include <vector>
#include <stdarg.h>
#include <filesystem>
#include <algorithm>
#include <format>
#include <string_view>
#include <chrono>

#if IBM
#include <ws2tcpip.h>
#endif

#if LIN
#include <dlfcn.h>
#include <gtk/gtk.h>
#include <arpa/inet.h>
#endif

#if APL
#include <CoreFoundation/CoreFoundation.h>
#include <Foundation/Foundation.h>
#include <AppKit/AppKit.h>
#include <dlfcn.h>
#include <string>
#include <iostream>
#include <sstream>
#include <stdio.h>
#endif
#include <XPLMUtilities.h>
#include <XPLMPlugin.h>
#include <XPLMDisplay.h>
#include "XPLMDataAccess.h"


namespace fs = std::filesystem;
namespace Utils
{
    // Forward declaration
    static uint32_t GetTicks();

#if APL

    // Mac specific: this converts file paths from HFS (which we get from the SDK) to Unix (which the OS wants).char*
    // See this for more info:
    // http://www.xsquawkbox.net/xpsdk/mediawiki/FilePathsAndMacho
    static int ConvertPath(const char *inPath, char *outPath, int outPathMaxLen)
    {

        CFStringRef inStr = CFStringCreateWithCString(kCFAllocatorDefault, inPath, kCFStringEncodingMacRoman);
        if (inStr == NULL)
            return -1;
        CFURLRef url = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, inStr, kCFURLHFSPathStyle, 0);
        CFStringRef outStr = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
        if (!CFStringGetCString(outStr, outPath, outPathMaxLen, kCFURLPOSIXPathStyle))
            return -1;
        CFRelease(outStr);
        CFRelease(url);
        CFRelease(inStr);
        return 0;
    }
#endif

    static fs::path GetPluginDirectory()
    {
        char pluginPath[MAX_PATH] = {0};
        XPLMGetPluginInfo(XPLMGetMyID(), NULL, pluginPath, NULL, NULL);
        XPLMExtractFileAndPath(pluginPath);
        return fs::path(pluginPath);
    }

    template <typename... Args>
    static void LOG(std::string_view fmt, Args &&...args)
    {
#ifdef ENABLE_LOG

        auto now = std::chrono::system_clock::now();
        std::string message = std::vformat(fmt, std::make_format_args(args...));
        std::string msg = std::format("INAV XITL[{:%T}]: {}\n", now, message);

        XPLMDebugString(msg.c_str());
#if IBM
        OutputDebugString(msg.c_str());
#endif

#endif
    }

    static void DisableBrakes()
    {
        // disable parking brakes
        XPLMDataRef df_parkBrake = XPLMFindDataRef("sim/flightmodel/controls/parkbrake");
        if (df_parkBrake != NULL)
        {
            XPLMSetDataf(df_parkBrake, 0);
        }
    }

    static void SetView()
    {
        XPLMCommandRef command_ref = XPLMFindCommand("sim/view/forward_with_nothing");
        if (NULL != command_ref)
        {
            XPLMCommandOnce(command_ref);
        }

        // set FOV = 115
        XPLMDataRef df_fov = XPLMFindDataRef("sim/graphics/view/field_of_view_deg");
        if (df_fov != NULL)
        {
            XPLMSetDataf(df_fov, 110.0f);
        }

        // disable g load effects
        XPLMDataRef df_gload = XPLMFindDataRef("sim/graphics/settings/dim_gload");
        if (df_gload != NULL)
        {
            XPLMSetDatai(df_gload, 0);
        }
    }

    static float Clampf(float value, float minValue, float maxValue)
    {
        if (value < minValue)
            value = minValue;
        if (value > maxValue)
            value = maxValue;
        return value;
    }

    static int16_t ClampToInt16(float value)
    {
        return (int16_t)round(Clampf(value, INT16_MIN, INT16_MAX));
    }

    static void DelayMS(uint32_t valueMS)
    {
#if IBM
        Sleep(valueMS);
#elif LIN || APL
        usleep(valueMS * 1000);
#endif
    }

#if LIN || APL

    static bool IsDebuggerPresent()
    {
        return false;
    }

#endif

    static uint32_t GetTicks()
    {
#if LIN
        enum
        {
#ifdef CLOCK_BOOTTIME
            boot_time_id = CLOCK_BOOTTIME
#else
            boot_time_id = 7
#endif
        };
        struct timespec spec;
        clock_gettime(boot_time_id, &spec);
        return (uint32_t)(((uint64_t)spec.tv_sec) * 1000 + ((uint64_t)spec.tv_nsec) / 1000000);
#elif APL
        struct timespec spec;
        clock_gettime(CLOCK_MONOTONIC, &spec);
        return (uint32_t)(((uint64_t)spec.tv_sec) * 1000 + ((uint64_t)spec.tv_nsec) / 1000000);
#elif IBM
        return (uint32_t)GetTickCount64();
#endif
    }

#if IBM

    static std::string GetClipboardText()
    {
        std::string result;
        if (OpenClipboard(NULL))
        {
            const char *clip = (const char *)GetClipboardData(CF_TEXT);
            CloseClipboard();

            if (clip)
            {
                result = std::string(clip);
            }
        }
        return result;
    }
#endif

#if LIN

    static std::string GetClipboardText()
    {
        std::string result;
        gtk_init(0, NULL);

        GtkClipboard *clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
        if (clipboard)
        {
            gchar *text = gtk_clipboard_wait_for_text(clipboard);

            if (text)
            {
                result = std::string(text);
                g_free(text);
            }
        }
        return result;
    }
#endif

// Untested code on APL
#if APL
    static std::string GetClipboardText()
    {
        std::string result;
        @autoreleasepool
        {
            NSPasteboard *pasteboard = [NSPasteboard generalPasteboard];
            NSString *content = [pasteboard stringForType:NSPasteboardTypeString];

            if (content != nil)
            {
                // Convert NSString to a C-style UTF8 string and then to std::string
                result = std::string([content UTF8String]);
            }
        }
        return result;
    }
#endif

    // subPath = "assets\\fonts"
    static std::vector<fs::path> GetFontPaths(fs::path subPath, bool directories)
    {
        fs::path path = GetPluginDirectory() / "assets" / subPath;
        std::vector<fs::path> fontList;
        if (fs::exists(path))
        {
            for (auto dirEntry = fs::recursive_directory_iterator(path); dirEntry != fs::recursive_directory_iterator(); ++dirEntry)
            {
                dirEntry.disable_recursion_pending();
                if (dirEntry->is_directory() && directories)
                    fontList.push_back(dirEntry->path());
                if (dirEntry->is_regular_file() && !directories)
                    fontList.push_back(dirEntry->path());
            }
        }
        return fontList;
    }

    static void ReplaceAll(std::string &str, const std::string &from, const std::string &to)
    {
        if (from.empty())
            return;
        size_t start_pos = 0;
        while ((start_pos = str.find(from, start_pos)) != std::string::npos)
        {
            str.replace(start_pos, from.length(), to);
            start_pos += to.length();
        }
    }

    static void CapitalizeFirstLetter(std::string &str)
    {
        if (!str.empty())
        {
            str[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(str[0])));
        }
    }

    static std::string ToLower(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c)
                       { return std::tolower(c); });
        return result;
    }

    static std::string ToUpper(const std::string &str)
    {
        std::string result = str;
        std::transform(result.begin(), result.end(), result.begin(), [](unsigned char c)
                       { return std::toupper(c); });
        return result;
    }

    static bool ValidateIpAddress(const std::string ipAddress)
    {
        struct sockaddr_in sa;
#if IBM
        return InetPton(AF_INET, ipAddress.c_str(), &(sa.sin_addr)) == 1;
#else
        int result = inet_pton(AF_INET, ipAddress.c_str(), &(sa.sin_addr));
        return result != 0;
#endif
    }

    static inline uint8_t getLowerByte(uint16_t value)
    {
        return static_cast<uint8_t>(value & 0xFF);
    }

    static inline uint8_t getUpperByte(uint16_t value)
    {
        return static_cast<uint8_t>((value & 0xFF00) >> 8);
    }

}
