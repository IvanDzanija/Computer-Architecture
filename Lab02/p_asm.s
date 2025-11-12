# Registers EAX, ECX, and EDX are caller-saved, and the rest are callee-saved.
.intel_syntax noprefix

.global potprogram_asm

potprogram_asm:
    push  ebp
    mov   ebp, esp

    mov   eax, [ebp+12]
    add   eax, [ebp+8]
    imul  eax, [ebp+16]

    pop   ebp
    ret

.global igra_registrima

igra_registrima:
    # cdecl prolog
    push ebp
    mov ebp, esp

    # callee saved registers
    push ebx

    mov eax, 42 # eax <- 42, eax is caller saved

    mov ebx, 0x42 # eax <- 0x42, ebx is callee saved

    # write 0xffff to top 16 bits of edx
    or edx, 0xffff0000 # edx <- ffff|old_lower_16_bits
    # write 0xdd to lower 8 bits of edx
    and edx, 0xffffff00 # edx <- old_top_24_bits|00
    or edx, 0xdd # edx <- old_top_24_bits|dd
    # edx is caller saved

    #cdecl epilog
    pop ebx
    pop ebp
    ret

.global zbrajanje_asm

zbrajanje_asm:
    # cdecl prolog
    push ebp
    mov ebp, esp

    # Accumulator - eax, counter - ecx, limit - edx
    mov eax, 0 # Initialize accumulator
    mov ecx, 0 # Initialize counter
    mov edx, [ebp+8] # Load n into edx

L1: add eax, ecx
    add ecx, 1
    cmp ecx, edx
    jl L1

    # cdecl epilog
    pop ebp
    ret


.global zbrajanje_vektora_x87

zbrajanje_vektora_x87:
    # cdecl prolog
    push ebp
    mov ebp, esp

    # save context
    push ebx
    push edi


    # Need 5 registers?
    # A -> eax, B -> ebx, index -> ecx, count -> edx, R -> edi
    mov ecx, 0 # counter = 0
    mov edx, [ebp+16] # limit = count
    mov eax, [ebp+8] # pointer to first element of a
    mov ebx, [ebp+12] # pointer to first element of b
    mov edi, [ebp+20] # pointer to first element of R

L2: fld DWORD PTR [eax+ecx*4]
    fld DWORD PTR [ebx+ecx*4]
    faddp
    fstp DWORD PTR [edi+ecx*4]
    inc ecx

    cmp ecx, edx
    jl L2

    # context
    pop edi
    pop ebx

    # cdecl epilog
    pop ebp
    ret

.global zbrajanje_vektora_sse

zbrajanje_vektora_sse:
    #cdecl prolog
    push ebp
    mov ebp, esp

    # save context
    push ebx
    push edi

    # A -> eax, B -> ebx, index -> ecx, count -> edx, R -> edi
    mov ecx, 0 # counter = 0
    mov eax, [ebp+8] # pointer to first element of a
    mov ebx, [ebp+12] # pointer to first element of b
    mov edx, [ebp+16] # limit = count
    mov edi, [ebp+20] # pointer to first element of R

L3: movaps xmm0, [eax+ecx*4]
    addps xmm0, [ebx+ecx*4]
    movaps [edi+ecx*4], xmm0
    add ecx, 4

    cmp ecx, edx
    jl L3

    # context
    pop edi
    pop ebx

    # cdecl epilog
    pop ebp
    ret
