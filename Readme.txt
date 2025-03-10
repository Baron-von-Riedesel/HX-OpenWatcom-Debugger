
  1. About
  
   This version of OpenWatcom's WD can debug programs written in C, C++ or
  assembly language. It has improved support for the MS CodeView debug format
  and hence is not restricted to Open Watcom, but should also work with
  MS C(++) compilers, both 16-bit and 32-bit, as long as the CodeView format
  is CV4 or CV5.

   The HX trap helper files supplied with the debugger should be able to
  handle DPMI mode switches. So they aren't restricted for debugging binaries
  in the usual HX formats ( that is, NE for 16-bit and PE/PX for 32-bit ),
  but may be able to handle any DOS binary.

   Although the focus of this package is to debug HX binaries, the default
  Open Watcom trap file, STD.TRP, is also supplied. So the debugger can also
  be used to debug DOS real-mode applications without the HX trap files; in
  this case, the /TR cmdline option isn't needed. Additionally, all files
  needed to debug CauseWay DOS extender binaries are also included. 
  
   Supported debug formats are codeview and dwarf. The watcom debug format may
  also work, but hasn't been tested and file WATCOM.DIP, which contains the
  support for this format, isn't included.


  2. Usage

  2.1 Prepare for Debugging

   If symbolic debugging is wanted, both the compiler/assembler and linker
  have to be told that. If the debug information is in codeview format
  and OW wlink ( or jwlink ) is to be used as linker, the cvp option has
  to be set.

   If the program to debug is a protected-mode application, but in a format
  that cannot be handled by the HX program loaders ( NE, PE/PX ), DPMI host
  HDPMI should be at least v3.20; if this isn't the case, environment variable
  HDPMI=32 must not be set.


  2.2 Debugging

   Start WD with the /tr=HX (32-bit) or /tr=HX16 (16-bit) option, depending
  what type of protected-mode application the program to debug is - if it is
  a real-mode program, both variants are ok and should behave identically.
  If no symbolic debug info has been created, the debugger will just stop at
  the program entry point of the executable.
   If the binary has been linked with symbolic debug info, the debugger will
  stop at main(). Be aware that this symbol (main) is also needed for assembly
  programs in this case.
   To debug graphic applications locally, WD has to be launched with the /swap
  cmdline option.


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

  e) WD.EXE: VESA save/restore, text mode detection, ...


  4. Creating the binaries

  You'll need:

  - the OpenWatcom v1.9 source
  - the HX runtime
  - the HX development package.

  a) Install HXDEV's Open Watcom support. See OWSUPP\Readme.txt
     for details. The Open Watcom linker wlink must know "system hxdos"
     and the HX startup modules must be in Watcom's lib386\dos directory.
     please verify that "system hxdos" (defined in file HX.LNK) has
     CSTRTDHR.OBJ as startup module ( that's for the Watcom calling
     convention ).
     

  b) Files to copy to the Open Watcom source tree:

   - dip_src/*.c         to  bld/dip/codeview/c
   - dip_src/*.h         to  bld/dip/codeview/h
   - mad_src/x86/*.c     to  bld/mad/x86/c
   - trap_src/dosr/*.asm to  bld/trap/lcl/dos/dosr/asm
   - trap_src/dosr/*.c   to  bld/trap/lcl/dos/dosr/c
   - trap_src/dosx/*.c   to  bld/trap/lcl/dos/dosx/c
   - trap_src/dosx/hx    to  bld/trap/lcl/dos/dosx/hx
   - ui_src/dos/*.c      to  bld/ui/dos/c
   - wv_src/*.c          to  bld/wv/c
   - wv_src/dsx/*.c      to  bld/wv/dsx/c
   - wv_src/dsx/*.h      to  bld/wv/dsx/h

  c) Change file bld\build\mif\cproj.mif:
  
   line "lflags_dos_386 = sys dos4g"  to "lflags_dos_386 = sys hxdos"
  
  d) (Re)build the Open Watcom project. Or, more specific:

    - run wmake in bld\dip\codeview\dos386
    - run wmake in bld\mad\dos386
    - run wmake in bld\trap\lcl\dos\dosr\dos.std
    - run wmake in bld\trap\lcl\dos\dosx\hx\dos.trp
    - run wmake in bld\ui\dos\dos386
    - run wmake in bld\wv\dsx\dos386

  e) Revert the change in file bld\build\mif\cproj.mif.


  japheth
