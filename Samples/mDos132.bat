@echo off
jwasm -Zi dos132.asm
jwlink debug c format dos f dos132.obj op q,m,cvp
