@echo off
jwasm -nologo -Zi Dos1.asm
jwlink debug c format dos f Dos1.obj op q,cvp
