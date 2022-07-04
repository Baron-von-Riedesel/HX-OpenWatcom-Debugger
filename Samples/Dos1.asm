
;--- 16bit DPMI application, linked as MZ. HDPMI=32
;--- must NOT be set to debug such programs.

    .286
    .model small
    .stack 2048
    .dosseg

LF  equ 10
CR  equ 13

    .data

szHello db "welcome in protected-mode",CR,LF,0

    .code

    nop	; for WD, main must not start at offset 0

main proc c
    mov si, offset szHello
nextchar:
    lodsb
    and al,al
    jz stringdone
    mov dl,al
    mov ah,2
    int 21h
    jmp nextchar
stringdone:
    ret
main endp

start:
    mov ax, @data
    mov ds, ax
    mov bx, ss
    sub bx, ax
    shl bx, 4
    mov ss, ax
    add sp, bx
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
    mov bp, sp
    mov ax, 0000    ;start a 16-bit client
    call far ptr [bp]
    jc initfailed
    add sp, 4
    call main
    mov ax, 4C00h   ;normal client exit
    int 21h

nohost:
    call error
    db "no DPMI host installed",CR,LF,'$'
nomem:
    call error
    db "not enough DOS memory for initialisation",CR,LF,'$'
initfailed:
    call error
    db "DPMI initialisation failed",CR,LF,'$'
error:
    push cs
    pop ds
    pop dx
    mov ah, 9
    int 21h
    mov ax, 4C00h
    int 21h

    end start
