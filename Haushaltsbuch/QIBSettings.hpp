#pragma once

extern bool g_highPrecisionRateEnabled;
extern bool g_rateColorizationEnabled;
extern bool g_autoAdjustViewRect;

extern const int g_winningRatePrecision;
extern const int g_winningRateFp;

void QIBSettings_Load();
void QIBSettings_Save();