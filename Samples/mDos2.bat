@echo off
jwasm -nologo -Zi Dos2.asm
jwlink debug c format win dpmi f Dos2.obj op q,cvp @Dos2.lbc
