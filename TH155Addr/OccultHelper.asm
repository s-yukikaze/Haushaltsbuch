	.586p
	.model flat, C
	.code

	assume fs:nothing

	extrn GetCoreBase:near

TryGetCoreBaseEH proc
	; restore esp
	mov esp, [esp+8]
	;  restore the SEH chain
	pop dword ptr fs:[0]
	; restore registers
	add esp, 4
	popad

	xor eax, eax
	retn
TryGetCoreBaseEH endp

TryGetCoreBase proc
	;  save registers
	pushad
	;  register my SEH handler
	mov eax, offset TryGetCoreBaseEH
	push eax
	push dword ptr fs:[0]
	mov dword ptr fs:[0], esp
	;  call my procedure
	call GetCoreBase
	;  restore the SEH chain
	pop dword ptr fs:[0]
	; restore esp
	add esp, 36 ; pushad (32o) + push eax (4o)
	retn
TryGetCoreBase endp
end