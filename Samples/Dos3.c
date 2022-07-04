
#include <stdio.h>

extern __cdecl _setdosstdfiles(); /* ensures files 0-4 are DOS alike */
int dummy = (int)_setdosstdfiles;

char *pTest = "Hello, world!\n";

int main( int argc, char *argv[] )
{
    printf( pTest );
    return( 0 );
}
