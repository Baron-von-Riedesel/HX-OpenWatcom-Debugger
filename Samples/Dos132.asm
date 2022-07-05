
;--- 32bit DPMI application, linked as MZ.

    .386
    .model small
    .stack 2048
    .dosseg

LF  equ 10
CR  equ 13


    .data

szWelcome db "welcome in protected-mode",CR,LF,0

    .code

    nop	; to make WD find it, main must not start at offset 0

main proc
    mov esi, offset szWelcome
nextchar:
    lodsb
    and al, al
    jz done
    mov dl, al
    mov ah, 2
    int 21h
    jmp nextchar
done:
    ret
main endp

start:
    call main
    mov ax, 4C00h   ;normal client exit
    int 21h

;--- the 16bit startup

_TEXT16 segment use16 word public 'CODE'

start16:
    mov ax, ss
    mov cx, es
    sub ax, cx
    mov bx, sp
    shr bx, 4
    inc bx
    add bx, ax
    mov ah, 4Ah     ;free unused memory
    int 21h

    mov ax, 1687h   ;DPMI host installed?
    int 2Fh
    and ax, ax
    jnz nohost
    push es         ;save DPMI entry address
    push di
    and si, si      ;requires host client-specific DOS memory?
    jz nomemneeded
    mov bx, si
    mov ah, 48h     ;alloc DOS memory
    int 21h
    jc nomem
    mov es, ax
nomemneeded:
    mov ax, DGROUP
    mov ds, ax
    mov bp, sp
    mov ax, 0001        ;start a 32-bit client
    call far ptr [bp]   ;initial switch to protected-mode
    jc initfailed

;--- now in protected-mode

;--- create a 32bit code selector and jump to 32bit code

    mov cx, 1
    mov ax, 0   ;alloc descriptor
    int 31h
    mov bx, ax
    mov cx, _TEXT
    mov dx, cx
    shl dx, 4
    shr cx, 12
    mov ax, 7   ;set base address
    int 31h
    mov dx, -1
    mov cx, 0
    mov ax, 8
    int 31h     ;set descriptor limit to 64 kB
    mov cx, cs
    lar cx, cx
    shr cx, 8
    or ch, 40h
    mov ax, 9
    int 31h     ;set code descriptors attributes to 32bit
    push ebx
    push offset start
    retd        ;jump to 32-bit code

nohost:
    mov dx, offset dErr1
    jmp error
nomem:
    mov dx, offset dErr2
    jmp error
initfailed:
    mov dx, offset dErr3
error:
    push cs
    pop ds
    mov ah, 9
    int 21h
    mov ax, 4C00h
    int 21h

dErr1 db "no DPMI host installed",CR,LF,'$'
dErr2 db "not enough DOS memory for initialisation",CR,LF,'$'
dErr3 db "DPMI initialisation failed",CR,LF,'$'

_TEXT16 ends

    end start16
