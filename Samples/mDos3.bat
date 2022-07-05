@echo off

rem 16-bit MS C/C++
rem there are a few peculiarities
rem - module initapp is included ( defines dummy InitApp, called
rem   by the MS C startup code in slibcew ).
rem - module stdosfil is needed to ensure that files 0, 1 and 2 are
rem   in text mode ( translation of LF to CR/LF )
rem - program patchne will mark the application as DPMI.
rem these things are needed because the MS 16-bit VC provides
rem a protected-mode library for Windows only ( slibcew.lib ).

\msvc\bin\cl -nologo -c -Zi -I\msvc\include dos3.c
\msvc\bin\link /A:16/NOLOGO/CO/NOD/MAP:F dos3 \hx\lib16\initapp \hx\lib16\stdosfil,,dos3.map,\msvc\lib\slibcew \hx\lib16\slx \hx\lib16\kernel16, dos3.def
patchne Dos3.exe

