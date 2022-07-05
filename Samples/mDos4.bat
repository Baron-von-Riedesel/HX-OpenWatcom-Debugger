@echo off

rem using MS C(++) 
rem old style debug info (-Z7 instead of -Zi) is needed!
rem jwlink's cvpack has problems with "new" MS debug info - to be fixed...
rem \msvc71\bin\cl -nologo -c -Z7 -I\msvc71\include dos4.c

rem using OW C

\watcom\binnt\wcc386 -bc -bt=nt -d2 -i=\watcom\h\nt -i=\watcom\h dos4.c
jwlink debug c format win pe hx f dos4.obj libpath \watcom\lib386\nt libpath \watcom\lib386 lib { \hx\lib\dkrnl32.lib \hx\lib\duser32.lib } op q,m,cvp,stub=dpmist32.bin

