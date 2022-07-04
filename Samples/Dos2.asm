
;--- this program is supposed to be linked as 16-bit NE binary.

    .model small
    .stack 1024

    .data

text db 13,10,"Hello, world!",13,10,'$'

    .code

    nop ; for WD, "main" must not start at offset 0.

;--- OW WD needs a "main" if symbolic debugging is wanted!

main proc
    mov ah, 09h
    mov dx, offset text
    int 21h
    ret
main endp

start:
    call main
    mov ax, 4c00h
    int 21h

    end start
