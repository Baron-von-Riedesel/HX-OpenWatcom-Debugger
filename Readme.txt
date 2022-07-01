
  1. About
  
  This version of OpenWatcom's WD can debug programs written in C, C++ or
  assembly language. It has improved support for the MS CodeView debug format
  and hence is not restricted to Open Watcom, but should also work with
  MS C(++) compilers, both 16-bit and 32-bit.


  2. Usage

  If symbolic debugging is wanted, both the compiler/assembler and linker
  have to be told that. If the debug information is in codeview format
  and OW wlink ( or jwlink ) is to be used as linker, the cvp option has
  to be set. 
  Start WD with the /tr=HX (32-bit) or /tr=HX16 (16-bit) option.
  If no symbolic debug info has been created, the debugger will just stop at
  the program entry point of the executable.
  If the binary has been linked with symbolic debug info, the debugger will
  stop at main(). Be aware that this symbol (main) is also needed for assembly
  programs.


  3. Modifications

  The files modified compared to Open Watcom v1.9 are:

  a) MADX86.MAD: 
     - page swap in "trace" mode whenever an INT instruction is detected.

  b) STD.TRP: 
     - allows to view and edit the XMM registers.
     - interrupt flag is enabled on debuggee's entry.

  c) DEFAULT.DBG: 
     - initially shows the register window at the upper right corner -
       quite appropriate for assembly programs.

  d) CODEVIEW.DIP: improved support for the CodeView debug format.


  4. Creating the binaries

  You'll need the OpenWatcom v1.9 source as a base. Then copy

   - dip_src/*.c     to  bld/dip/codeview/c
   - dip_src/*.h     to  bld/dip/codeview/h
   - mad_src/*.c     to  bld/mad/x86/c
   - trap_src/*.c    to  bld/trap/lcl/dos/dosr/c
   - trap_src/*.asm  to  bld/trap/lcl/dos/dosr/asm

  and (re)build the project.


  japheth
