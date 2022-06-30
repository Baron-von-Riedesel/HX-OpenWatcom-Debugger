
    Files modified for binary codeview.dll/codeview.dip, branch \watcom\bld\dip:

    - crash in wd/wdw if C module compiled with -hc is included and debug format is codeview
      dip\codeview\c\cvmisc.c    (just the "Confused()" proc - additional params )
      dip\codeview\c\cvsym.c     ( added case for S_BLOCK32 )
      dip\codeview\c\cvtype.c    (just the "Confused()" calls )
      dip\codeview\h\cvinfo.h    (just prototype for "Confused()")


