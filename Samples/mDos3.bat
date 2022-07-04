@echo off
\msvc\bin\cl -nologo -c -Zi -I\msvc\include dos3.c
\msvc\bin\link /A:16/NOLOGO/CO/NOD/MAP:F dos3 \hx\lib16\initapp,,dos3.map,\msvc\lib\slibcew \hx\lib16\slx \hx\lib16\kernel16, dos3.def
patchne Dos3.exe

