@echo off
\msvc71\bin\cl -nologo -c -Zi -I\msvc71\include dos4.c
\msvc71\bin\link dos4.obj \hx\lib\dkrnl32.lib libc.lib /NOD /DEBUG /LIBPATH:\msvc71\lib /STUB:\hx\bin\dpmist32.bin

