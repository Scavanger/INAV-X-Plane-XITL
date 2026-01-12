#pragma once

#define ENABLE_LOG

#ifndef XPLM300
#error This is made to be compiled against the XPLM300 SDK
#endif

#include <GL/glew.h>

#ifndef APIENTRY
    #define APIENTRY
#endif

#if IBM
    #define _WINSOCK_DEPRECATED_NO_WARNINGS
    #ifndef WIN32_LEAN_AND_MEAN
        #define WIN32_LEAN_AND_MEAN
    #endif
    #include <Winsock2.h>  
    #include <WS2tcpip.h>
    #include <windows.h>
#endif
    

#if LIN || APL
#define MAX_PATH 1260
#endif
