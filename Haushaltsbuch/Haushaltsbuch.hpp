#pragma once

#include "MinimalIniFile.hpp"

#define MINIMAL_USE_PROCESSHEAPSTRING
#include "MinimalString.hpp"

extern HINSTANCE g_hInstance;
extern Minimal::ProcessHeapString g_appPath;
extern MinimalIniFile g_settings;
