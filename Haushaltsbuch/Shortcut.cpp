#include "pch.hpp"
#include "shortcut.hpp"
#include "resource.h"

#define MINIMAL_USE_PROCESSHEAPARRAY
#include "MinimalArray.hpp"

static HACCEL s_accelHandle;
static Minimal::ProcessHeapArrayT<SHORTCUT> s_scArray;

HACCEL Shortcut_GetAccelHandle()
{
	return s_accelHandle;
}

bool Shortcut_Construct(SHORTCUT *shortcuts, int count)
{
	HACCEL accelHandle = NULL;
	Minimal::ProcessHeapArrayT<SHORTCUT> scArray;

	if (count > 0) {
		Minimal::ProcessHeapArrayT<ACCEL> accArray;
		for (int i = 0; i < count; ++i)
		{
			scArray.Push(shortcuts[i]);

			ACCEL accel;
			int key = LOBYTE(shortcuts[i].accel);
			int mod = HIBYTE(shortcuts[i].accel);
			accel.fVirt = mod | FVIRTKEY | FNOINVERT;
			accel.key = key;
			accel.cmd = ID_SHORTCUT_BASE + i;
			accArray.Push(accel);
		}
		accelHandle = ::CreateAcceleratorTable(accArray.GetRaw(), accArray.GetSize());
		if (!accelHandle) return false;
	}

	if (s_accelHandle) {
		::DestroyAcceleratorTable(s_accelHandle);
	}
	s_accelHandle = accelHandle;

	s_scArray.Clear();
	s_scArray += scArray;
	return true;
}

void Shortcut_Finalize()
{
	if (s_accelHandle) {
		DestroyAcceleratorTable(s_accelHandle);
	}
}

bool Shortcut_GetElement(SHORTCUT &ret, DWORD index)
{
	if (index >= s_scArray.GetSize())
		return false;
	ret = s_scArray[index];
	return true;
}

int Shortcut_Count()
{
	return s_scArray.GetSize();
}
