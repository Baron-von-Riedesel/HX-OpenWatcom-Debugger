
  1. About
  
  This version of OpenWatcom's WD can debug programs written in C, C++ or assembly language. It has improved support for the CodeView debug format and hence is not restricted to Open Watcom, but should also work with MS C(++) compilers, both 16-bit and 32-bit.


  2. Modifications

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


  japheth
