#pragma once

#define MAX_SHORTCUT 32

typedef struct {
	TCHAR path[1025];
	WORD accel;
} SHORTCUT;

bool Shortcut_Construct(SHORTCUT *shortcuts, int count);
void Shortcut_Finalize();
HACCEL Shortcut_GetAccelHandle();
bool Shortcut_GetElement(SHORTCUT &ret, DWORD index);
int Shortcut_Count();
