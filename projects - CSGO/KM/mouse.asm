EXTERNDEF _KeAcquireSpinLockAtDpcLevel:PROC
EXTERNDEF _KeReleaseSpinLockFromDpcLevel:PROC
EXTERNDEF _IofCompleteRequest:PROC
EXTERNDEF _IoReleaseRemoveLockEx:PROC
EXTERNDEF memmove:PROC

.data
WPP_RECORDER_INITIALIZED dq 0;
WPP_GLOBAL_Control dq 0;
.code

WPP_RECORDER_SF proc
	ret
WPP_RECORDER_SF endp

MouseClassReadCopyData proc
	mov    r11,rsp
	mov    QWORD PTR [r11+8h],rbx
	mov    QWORD PTR [r11+10h],rbp
	mov    QWORD PTR [r11+18h],rsi
	push   rdi
	push   r12
	push   r13
	push   r14
	push   r15
	sub    rsp,50h
	inc    DWORD PTR [rcx+0a8h]
	mov    rsi,rdx
	mov    eax,DWORD PTR [rcx+54h]
	mov    rdi,rcx
	mov    r13,QWORD PTR [rdx+0b8h]
	lea    ebp,[rax+rax*2]
	mov    ebx,DWORD PTR [r13+8h]
	shl    ebp,3h
	mov    edx,DWORD PTR [rdi+88h]
	cmp    ebp,ebx
	cmovae ebp,ebx
	sub    edx,DWORD PTR [rdi+78h]
	add    edx,DWORD PTR [rdi+68h]
	mov    r12d,ebp
	cmp    ebp,edx
	cmovae r12d,edx
	mov    r14,QWORD PTR [rsi+18h]
	mov    rdx,QWORD PTR [rdi+78h]
	mov    rcx,r14
	mov    r8d,r12d
	mov    r15d,r12d
	call   memmove
	add    r14,r15
	mov    ebx,ebp
	sub    ebx,r12d
	je     J1A5
	mov    rdx,QWORD PTR [rdi+68h]
	mov    r8,rbx
	mov    rcx,r14
	call   memmove
	mov    rcx,QWORD PTR [rdi+68h]
	add    rcx,rbx
	mov    QWORD PTR [rdi+78h],rcx
	jmp    J1B0
J1A5:
	add    QWORD PTR [rdi+78h],r15
J1B0:
	mov    ebx,ebp
	mov    rax,0aaaaaaaaaaaaaaabh
	mul    rbx
	shr    rdx,4h
	sub    DWORD PTR [rdi+54h],edx
	jne    J1FF
	mov    BYTE PTR [rdi+42h],1h
J1FF:
	mov    QWORD PTR [rsi+38h],rbx
	lea    r11,[rsp+50h]
	mov    rbx,QWORD PTR [r11+30h]
	xor    eax,eax
	mov    rsi,QWORD PTR [r11+40h]
	mov    DWORD PTR [r13+8h],ebp
	mov    rbp,QWORD PTR [r11+38h]
	mov    rsp,r11
	pop    r15
	pop    r14
	pop    r13
	pop    r12
	pop    rdi
	ret
MouseClassReadCopyData endp




MouseClassDequeueRead proc
	xor    edx,edx
	lea    r8,[rcx+98h]
J9:
	mov    rcx,QWORD PTR [r8]
	cmp    rcx,r8
	je     J47
	cmp    QWORD PTR [rcx+8h],r8
	jne    J4C
	mov    rax,QWORD PTR [rcx]
	cmp    QWORD PTR [rax+8h],rcx
	jne    J4C
	mov    QWORD PTR [r8],rax
	lea    rdx,[rcx-0a8h]
	mov    QWORD PTR [rax+8h],r8
	xor    eax,eax
	xchg   QWORD PTR [rdx+68h],rax
	test   rax,rax
	jne    J42
	mov    QWORD PTR [rcx+8h],rcx
	xor    edx,edx
	mov    QWORD PTR [rcx],rcx
J42:
	test   rdx,rdx
	je     J9
J47:
	mov    rax,rdx
	ret
	int    3
J4C:
	mov    ecx,3h
	int    29h
MouseClassDequeueRead endp


MouseClassServiceCallback proc
	mov    rax,rsp
	mov    QWORD PTR [rax+8h],rbx
	mov    QWORD PTR [rax+10h],rsi
	mov    QWORD PTR [rax+18h],rdi
	mov    QWORD PTR [rax+20h],r9
	push   rbp
	push   r12
	push   r13
	push   r14
	push   r15
	mov    rbp,rsp
	sub    rsp,70h
	mov    r13,r9
	mov    rbx,r8
	mov    r14,rdx
	mov    r15,rcx



	
	lea    rax, WPP_RECORDER_INITIALIZED
	xor    esi,esi
	cmp    WPP_RECORDER_INITIALIZED, rax
	jne    J61
	mov    rcx,QWORD PTR WPP_GLOBAL_Control
	cmp    WORD PTR [rcx+48h],si
	je     J61
	mov    rcx,QWORD PTR [rcx+40h]
	lea    r9d,[rsi+32h]
	lea    r8d,[rsi+3h]
	mov    dl,5h
	call   WPP_RECORDER_SF

J61:


	mov    rdi,QWORD PTR [r15+40h]
	sub    ebx,r14d
	mov    r12d,esi
	mov    DWORD PTR [r13+0h],esi
	lea    rcx,[rdi+90h]
	call   QWORD PTR _KeAcquireSpinLockAtDpcLevel
	nop    DWORD PTR [rax+rax*1+0h]
	lea    rax,[rbp-10h]
	mov    rcx,rdi
	mov    QWORD PTR [rbp-8h],rax
	lea    rax,[rbp-10h]
	mov    QWORD PTR [rbp-10h],rax
	call   MouseClassDequeueRead
	mov    rsi,rax
	xor    r9d,r9d
	mov    rax,0aaaaaaaaaaaaaaabh
	test   rsi,rsi
	je     J1aa
	mov    r13,QWORD PTR [rsi+0b8h]
	mov    r12d,ebx
	mov    r8d,DWORD PTR [r13+8h]
	cmp    ebx,r8d
	cmovae r12d,r8d
	mul    r12
	mov    rax,QWORD PTR [rbp+48h]
	shr    rdx,4h
	add    DWORD PTR [rax],edx
	lea    rax, WPP_RECORDER_INITIALIZED
	cmp    WPP_RECORDER_INITIALIZED,rax
	jne    J11d
	mov    rcx, QWORD PTR WPP_GLOBAL_Control
	cmp    WORD PTR [rcx+48h],r9w
	je     J11d
	mov    rax,QWORD PTR [rsi+18h]
	mov    rcx,QWORD PTR [rcx+40h]
	mov    QWORD PTR [rsp+50h],rax
	mov    QWORD PTR [rsp+48h],r14
	mov    DWORD PTR [rsp+40h],r8d
	mov    DWORD PTR [rsp+38h],ebx
	mov    QWORD PTR [rsp+30h],rsi
	mov    QWORD PTR [rsp+28h],r15
	call   WPP_RECORDER_SF

J11d:

	mov    rax,0fffff78000000014h
	mov    rax,QWORD PTR [rax]
	lea    rdx,WPP_RECORDER_INITIALIZED
	cmp    WPP_RECORDER_INITIALIZED,rdx
	jne    J15e
	mov    rcx, QWORD PTR WPP_GLOBAL_Control
	mov    DWORD PTR [rsp+40h],r12d
	mov    QWORD PTR [rsp+38h],rax
	mov    QWORD PTR [rsp+30h],rsi
	mov    rcx,QWORD PTR [rcx+40h]
	mov    QWORD PTR [rsp+28h],r15
	call   WPP_RECORDER_SF

J15e:

	mov    rcx,QWORD PTR [rsi+18h]
	mov    r8,r12
	mov    rdx,r14
	call   memmove
	mov    QWORD PTR [rsi+38h],r12
	lea    rcx,[rbp-10h]
	xor    r8d,r8d
	mov    DWORD PTR [rsi+30h],r8d
	add    rsi,0a8h
	mov    DWORD PTR [r13+8h],r12d
	mov    rax,QWORD PTR [rbp-8h]
	cmp    QWORD PTR [rax],rcx
	jne    J495
	mov    r13,QWORD PTR [rbp+48h]
	lea    rcx,[rbp-10h]
	mov    QWORD PTR [rsi],rcx
	mov    QWORD PTR [rsi+8h],rax
	mov    QWORD PTR [rax],rsi
	mov    QWORD PTR [rbp-8h],rsi

J1aa:

	mov    eax,r12d
	add    r14,rax
	sub    ebx,r12d
	lea    r12,WPP_RECORDER_INITIALIZED
	xor    esi,esi
	cmp    WPP_RECORDER_INITIALIZED,r12
	jne    J1e4
	mov    rcx, QWORD PTR WPP_GLOBAL_Control
	cmp    WORD PTR [rcx+48h],si
	je     J1e4
	mov    rcx,QWORD PTR [rcx+40h]
	mov    DWORD PTR [rsp+30h],ebx
	mov    QWORD PTR [rsp+28h],r15
	call   WPP_RECORDER_SF

J1e4:

	test   ebx,ebx
	je     J41d
	cmp    WPP_RECORDER_INITIALIZED,r12
	jne    J22f
	mov    rcx, QWORD PTR WPP_GLOBAL_Control
	cmp    WORD PTR [rcx+48h],si
	je     J22f
	mov    eax,DWORD PTR [rdi+54h]
	mov    r9d,36h
	mov    rcx,QWORD PTR [rcx+40h]
	mov    DWORD PTR [rsp+38h],ebx
	lea    edx,[rax+rax*2]
	mov    eax,DWORD PTR [rdi+88h]
	shl    edx,3h
	sub    eax,edx
	mov    DWORD PTR [rsp+30h],eax
	mov    QWORD PTR [rsp+28h],r15
	call   WPP_RECORDER_SF

J22f:

	mov    ecx,DWORD PTR [rdi+88h]
	cmp    ecx,ebx
	mov    r12d,ecx
	cmovae r12d,ebx
	sub    ecx,DWORD PTR [rdi+70h]
	mov    ebx,DWORD PTR [rdi+68h]
	add    ebx,ecx
	lea    rax,WPP_RECORDER_INITIALIZED
	cmp    WPP_RECORDER_INITIALIZED,rax
	jne    J287
	mov    rcx, QWORD PTR WPP_GLOBAL_Control
	cmp    WORD PTR [rcx+48h],si
	je     J287
	mov    rcx,QWORD PTR [rcx+40h]
	mov    r9d,38h
	mov    DWORD PTR [rsp+38h],ebx
	mov    DWORD PTR [rsp+30h],r12d
	mov    QWORD PTR [rsp+28h],r15
	call   WPP_RECORDER_SF
	lea    rax,WPP_RECORDER_INITIALIZED

J287:

	cmp    r12d,ebx
	mov    esi,r12d
	cmovae esi,ebx
	cmp    WPP_RECORDER_INITIALIZED,rax
	jne    J2cc
	mov    rcx, QWORD PTR WPP_GLOBAL_Control
	xor    eax,eax
	cmp    WORD PTR [rcx+48h],ax
	je     J2cc
	mov    rcx,QWORD PTR [rcx+40h]
	lea    r9d,[rax+39h]
	mov    rax,QWORD PTR [rdi+70h]
	mov    QWORD PTR [rsp+40h],rax
	mov    QWORD PTR [rsp+38h],r14
	mov    DWORD PTR [rsp+30h],esi
	mov    QWORD PTR [rsp+28h],r15
	call   WPP_RECORDER_SF

J2cc:

	mov    rcx,QWORD PTR [rdi+70h]
	mov    rdx,r14
	mov    r8d,esi
	mov    ebx,esi
	call   memmove
	add    QWORD PTR [rdi+70h],rbx
	add    r14,rbx
	mov    rdx,QWORD PTR [rdi+68h]
	mov    eax,DWORD PTR [rdi+88h]
	mov    rcx,QWORD PTR [rdi+70h]
	add    rax,rdx
	cmp    rcx,rax
	jb     J301
	mov    QWORD PTR [rdi+70h],rdx
	mov    rcx,rdx

J301:

	mov    ebx,r12d
	sub    ebx,esi
	je     J362
	lea    rdx,WPP_RECORDER_INITIALIZED
	mov    rax,rcx
	cmp    WPP_RECORDER_INITIALIZED,rdx
	jne    J350
	mov    rdx, QWORD PTR WPP_GLOBAL_Control
	xor    r8d,r8d
	cmp    WORD PTR [rdx+48h],r8w
	je     J350
	mov    QWORD PTR [rsp+40h],rcx
	lea    r9d,[r8+03ah]
	mov    rcx,QWORD PTR [rdx+40h]
	mov    QWORD PTR [rsp+38h],r14
	mov    DWORD PTR [rsp+30h],ebx
	mov    QWORD PTR [rsp+28h],r15
	call   WPP_RECORDER_SF
	mov    rax,QWORD PTR [rdi+70h]

J350:

	mov    r8,rbx
	mov    rdx,r14
	mov    rcx,rax
	call   memmove
	add    QWORD PTR [rdi+70h],rbx

J362:

	mov    ecx,r12d
	mov    rax,0aaaaaaaaaaaaaaabh
	mul    rcx
	shr    rdx,4h
	add    DWORD PTR [rdi+54h],edx
	mov    ecx,DWORD PTR [r13+0h]
	add    ecx,edx
	mov    eax,ecx
	mov    DWORD PTR [r13+0h],ecx
	lea    r12,WPP_RECORDER_INITIALIZED
	xor    esi,esi
	cmp    WPP_RECORDER_INITIALIZED,r12
	jne    J41d
	mov    rcx, QWORD PTR WPP_GLOBAL_Control
	cmp    WORD PTR [rcx+48h],si
	je     J41d
	mov    rcx,QWORD PTR [rcx+40h]
	mov    DWORD PTR [rsp+48h],eax
	mov    rax,QWORD PTR [rdi+78h]
	mov    QWORD PTR [rsp+40h],rax
	mov    rax,QWORD PTR [rdi+70h]
	mov    QWORD PTR [rsp+38h],rax
	mov    eax,DWORD PTR [rdi+54h]
	mov    DWORD PTR [rsp+30h],eax
	mov    QWORD PTR [rsp+28h],r15
	call   WPP_RECORDER_SF
	jmp    J41d

J3d5:

	mov    rcx,rdi
	call   MouseClassDequeueRead
	mov    rbx,rax
	test   rax,rax
	je     J422
	mov    rdx,rax
	mov    rcx,rdi
	call   MouseClassReadCopyData
	mov    DWORD PTR [rbx+30h],eax
	lea    rcx,[rbp-10h]
	mov    rdx,QWORD PTR [rbp-8h]
	lea    rax,[rbx+0a8h]
	cmp    QWORD PTR [rdx],rcx
	jne    J495
	mov    QWORD PTR [rax+8h],rdx
	lea    rcx,[rbp-10h]
	mov    QWORD PTR [rax],rcx
	mov    QWORD PTR [rdx],rax
	mov    QWORD PTR [rbp-8h],rax

J41d:

	cmp    DWORD PTR [rdi+54h],esi


	ja     J3d5

J422:

	lea    rcx,[rdi+90h]
	call   QWORD PTR _KeReleaseSpinLockFromDpcLevel
	nop    DWORD PTR [rax+rax*1+0h]

J435:

	mov    rbx,QWORD PTR [rbp-10h]
	lea    rax,[rbp-10h]
	cmp    rbx,rax
	je     J49c
	lea    rax,[rbp-10h]
	cmp    QWORD PTR [rbx+8h],rax
	jne    J495
	mov    rax,QWORD PTR [rbx]
	cmp    QWORD PTR [rax+8h],rbx
	jne    J495
	lea    rcx,[rbp-10h]
	mov    QWORD PTR [rbp-10h],rax
	mov    QWORD PTR [rax+8h],rcx
	mov    dl,6h
	lea    rcx,[rbx-0a8h]
	call   QWORD PTR _IofCompleteRequest
	nop    DWORD PTR [rax+rax*1+0h]
	lea    rcx,[rdi+20h]
	mov    r8d,20h
	lea    rdx,[rbx-0a8h]
	call   QWORD PTR _IoReleaseRemoveLockEx
	nop    DWORD PTR [rax+rax*1+0h]
	jmp    J435

J495:

	mov    ecx,3h
	int    29h

J49C:

	cmp    WPP_RECORDER_INITIALIZED,r12
	jne    J4c7
	mov    rcx, QWORD PTR WPP_GLOBAL_Control
	cmp    WORD PTR [rcx+48h],si
	je     J4c7
	mov    rcx,QWORD PTR [rcx+40h]
	mov    r9d,3ch
	mov    dl,5h
	lea    r8d,[r9-39h]
	call   WPP_RECORDER_SF

J4c7:

	lea    r11,[rsp+70h]
	mov    rbx,QWORD PTR [r11+30h]
	mov    rsi,QWORD PTR [r11+38h]
	mov    rdi,QWORD PTR [r11+40h]
	mov    rsp,r11
	pop    r15
	pop    r14
	pop    r13
	pop    r12
	pop    rbp
	ret
MouseClassServiceCallback endp

end

