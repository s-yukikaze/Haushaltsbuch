	.586
	.model flat, C
	.code

	public memcpy
memcpy proc
	push esi
	push edi
	mov edi, [esp+12]
	mov esi, [esp+16]
	mov edx, [esp+20]
	mov eax, edi
	mov ecx, edx
	shr ecx, 2
	and edx, 3
	rep movsd
	mov ecx, edx
	rep movsb
	pop edi
	pop esi
	ret
memcpy endp

	public memset
memset proc
	push edi
	mov edx, [esp+16]
	mov eax, [esp+12]
	mov edi, [esp+8]
	mov ecx, edx
	shr ecx, 2
	and edx, 3
	push edi
	rep stosd
	mov ecx, edx
	rep stosb
	pop eax
	pop edi
	ret
memset endp

	public _chkstk
_chkstk proc
	push ecx
	cmp eax, 4096
	lea ecx, [esp + 8]
	jb probe_finish
probe_loop:
	sub ecx, 4096
	sub eax, 4096
	test [ecx], eax
	cmp eax, 4096
	jae probe_loop
probe_finish:
	sub ecx, eax
	test [ecx], eax

	mov eax, esp
	mov esp, ecx
	
	mov ecx, [eax]
	mov eax, [eax + 4]
	
	push eax
	ret

_chkstk endp
	end