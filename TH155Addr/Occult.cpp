#include <Windows.h>
#include <ImageHlp.h>
#include <shlwapi.h>
#include <stdio.h>

static const void *memmem(const void *haystack, size_t haystacklen,
	const void *needle, size_t needlelen)
{
	if (needlelen <= haystacklen)
		for (const char *p = static_cast<const char*>(haystack);
		(p + needlelen) <= (static_cast<const char *>(haystack) + haystacklen);
			p++)
			if (memcmp(p, needle, needlelen) == 0)
				return p;

	return nullptr;
}

static LPCVOID findNearCall(LPCVOID haystack, size_t haystack_len, LPCVOID callee) {
	LPCBYTE begin = static_cast<LPCBYTE>(haystack);
	LPCBYTE const last_possible = begin + haystack_len - sizeof(callee) - 1;
	ptrdiff_t tail = static_cast<LPCBYTE>(callee) - begin - 5;

	for (; begin <= last_possible; begin++, tail--) {
		if (*begin == 0xE8 && !memcmp(begin + 1, &tail, sizeof tail))
			return begin;
	}
	return nullptr;
}

static PIMAGE_SECTION_HEADER GetSection(LOADED_IMAGE &img, PCSTR sectionName)
{
	int cmplen = ::lstrlenA(sectionName);
	if (cmplen > 0 && cmplen <= sizeof(IMAGE_SECTION_HEADER::Name)) {
		for (ULONG i = 0; i < img.NumberOfSections; ++i) {
			if (::StrCmpNA(static_cast<PSTR>(static_cast<PVOID>(img.Sections[i].Name)), sectionName, cmplen) == 0) {
				return &img.Sections[i];
			}
		}
	}
	return nullptr;
}

static bool pseudoMapAndLoad(PLOADED_IMAGE img)
{
	if (img == nullptr) return false;

	HMODULE mainMod = ::GetModuleHandle(nullptr);
	if (mainMod == nullptr) return false;
	PIMAGE_NT_HEADERS nthdr = ImageNtHeader(mainMod);
	if (nthdr == nullptr) return false;

	RtlZeroMemory(img, sizeof(img));
	img->MappedAddress = reinterpret_cast<PUCHAR>(mainMod);
	img->FileHeader = nthdr;
	img->NumberOfSections = nthdr ->FileHeader.NumberOfSections;
	img->Sections = IMAGE_FIRST_SECTION(nthdr);

	return true;
}

extern "C" DWORD_PTR GetCoreBase()
{
	LOADED_IMAGE img;
	if (!::pseudoMapAndLoad(&img)) {
		return 0;
	}

	PIMAGE_SECTION_HEADER const text = GetSection(img, ".text");
	PIMAGE_SECTION_HEADER const rdata = GetSection(img, ".rdata");
	PIMAGE_SECTION_HEADER const data = GetSection(img, ".data");
	if (!text || !rdata || !data) {
		return 0;
	}
	PVOID const textBase = ImageRvaToVa(img.FileHeader, img.MappedAddress, text->VirtualAddress, nullptr);
	PVOID const dataBase = ImageRvaToVa(img.FileHeader, img.MappedAddress, data->VirtualAddress, nullptr);
	PVOID const rdataBase = ImageRvaToVa(img.FileHeader, img.MappedAddress, rdata->VirtualAddress, nullptr);
	DWORD const textSize = (text->Misc.VirtualSize + img.FileHeader->OptionalHeader.SectionAlignment - 1) & ~(img.FileHeader->OptionalHeader.SectionAlignment - 1);
	DWORD const dataSize = (data->Misc.VirtualSize + img.FileHeader->OptionalHeader.SectionAlignment - 1) & ~(img.FileHeader->OptionalHeader.SectionAlignment - 1);
	DWORD const rdataSize = (rdata->Misc.VirtualSize + img.FileHeader->OptionalHeader.SectionAlignment - 1) & ~(img.FileHeader->OptionalHeader.SectionAlignment - 1);

	LPCBYTE const sqvmRttiStr = static_cast<LPCBYTE>(memmem(dataBase, dataSize, ".?AUSQVM@@", 10));
	if (!sqvmRttiStr) {
		return 0;
	}
	DWORD_PTR const sqvmRttiStrRef = sqvmRttiStr - 8 - static_cast<LPBYTE>(dataBase) + img.FileHeader->OptionalHeader.ImageBase + data->VirtualAddress;

	LPCBYTE const sqvmRtti = static_cast<LPCBYTE>(memmem(rdataBase, rdataSize, &sqvmRttiStrRef, sizeof DWORD_PTR));
	if (!sqvmRtti) {
		return 0;
	}

	DWORD_PTR const sqvmRttiRef = sqvmRtti - 12 - static_cast<LPBYTE>(rdataBase) + img.FileHeader->OptionalHeader.ImageBase + rdata->VirtualAddress;
	LPCBYTE const sqvmVtblRtti = static_cast<LPCBYTE>(memmem(rdataBase, rdataSize, &sqvmRttiRef, 4));
	if (!sqvmVtblRtti) {
		return 0;
	}

	DWORD_PTR const sqvmVtblRef = sqvmVtblRtti + 4 - static_cast<LPBYTE>(rdataBase) + img.FileHeader->OptionalHeader.ImageBase + rdata->VirtualAddress;

	LPCBYTE const sqvmCtorHint = static_cast<LPCBYTE>(memmem(textBase, textSize, &sqvmVtblRef, sizeof DWORD_PTR));
	if (!sqvmCtorHint) {
		return 0;
	}

	BYTE const sqvmCtorHead[] = { 0x55, 0x8B, 0xEC, 0x6A, 0xFF };
	LPCBYTE const sqvmCtor = static_cast<LPCBYTE>(memmem(sqvmCtorHint - 0x40, 0x40, sqvmCtorHead, sizeof sqvmCtorHead));
	if (!sqvmCtor) {
		return 0;
	}

	LPCBYTE sq_openHintBase = static_cast<LPCBYTE>(textBase);
	DWORD sq_openHintSize = textSize;
	for (;;) {
		LPCBYTE const sq_openHint = static_cast<LPCBYTE>(findNearCall(sq_openHintBase, sq_openHintSize, sqvmCtor));
		if (!sq_openHint) {
			return 0;
		}
		sq_openHintSize -= sq_openHint - sq_openHintBase;
		sq_openHintBase = sq_openHint + 1;

		BYTE const sq_openHead[] = { 0x55, 0x8B, 0xEC }; // push ebp; mov ebp, esp
		LPCBYTE const sq_open = static_cast<LPCBYTE>(memmem(sq_openHint - 0x48, 0x48, sq_openHead, sizeof sq_openHead));
		if (!sq_open) {
			continue;
		}

		LPCBYTE mysq_createHintbase = static_cast<LPCBYTE>(textBase);
		DWORD mysq_createHintSize = textSize;
		for (;;) {
			LPCBYTE const mysq_createHint = static_cast<LPCBYTE>(findNearCall(mysq_createHintbase, mysq_createHintSize, sq_open));
			if (!mysq_createHint) {
				break;
			}
			mysq_createHintSize -= mysq_createHint - mysq_createHintbase;
			mysq_createHintbase = mysq_createHint + 1;

			LPCBYTE mysq_initHint;
			int mysq_initHintFindOffset = 1;
			for (; mysq_initHintFindOffset <= 0x30; ++mysq_initHintFindOffset) {
				LPCBYTE const mysq_create = mysq_createHint - mysq_initHintFindOffset; // push reg32; push imm32(0x400)
				mysq_initHint = static_cast<LPCBYTE>(findNearCall(textBase, textSize, mysq_create));
				if (mysq_initHint) {
					break;
				}
			}
			if (mysq_initHintFindOffset <= 0x20) {
				BYTE const movToGlobalVarHintOps[] = { 0x89, 0x35 }; // mov ds:[imm32], esi
				LPCBYTE const movToGlobalVarHint = static_cast<LPCBYTE>(memmem(mysq_initHint + 5, 0x20, movToGlobalVarHintOps, sizeof movToGlobalVarHintOps));
				if (!movToGlobalVarHint) {
					continue;
				}
				return *static_cast<const DWORD_PTR *>(static_cast<LPCVOID>(movToGlobalVarHint + 2)) - img.FileHeader->OptionalHeader.ImageBase;
				return 0;
			}
		}
	}
}