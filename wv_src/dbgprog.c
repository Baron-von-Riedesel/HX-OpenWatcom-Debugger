/****************************************************************************
*
*                            Open Watcom Project
*
*    Portions Copyright (c) 1983-2002 Sybase, Inc. All Rights Reserved.
*
*  ========================================================================
*
*    This file contains Original Code and/or Modifications of Original
*    Code as defined in and that are subject to the Sybase Open Watcom
*    Public License version 1.0 (the 'License'). You may not use this file
*    except in compliance with the License. BY USING THIS FILE YOU AGREE TO
*    ALL TERMS AND CONDITIONS OF THE LICENSE. A copy of the License is
*    provided with the Original Code and Modifications, and is also
*    available at www.sybase.com/developer/opensource.
*
*    The Original Code and all software distributed under the License are
*    distributed on an 'AS IS' basis, WITHOUT WARRANTY OF ANY KIND, EITHER
*    EXPRESS OR IMPLIED, AND SYBASE AND ALL CONTRIBUTORS HEREBY DISCLAIM
*    ALL SUCH WARRANTIES, INCLUDING WITHOUT LIMITATION, ANY WARRANTIES OF
*    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE, QUIET ENJOYMENT OR
*    NON-INFRINGEMENT. Please see the License for the specific language
*    governing rights and limitations under the License.
*
*  ========================================================================
*
* Description:  Processing of the NEW command, program and symbol loading.
*
****************************************************************************/


#include <string.h>
#include <stdio.h>
#include "spawn.h"
#include "dbgdefn.h"
#include "dbglit.h"
#include "dbgio.h"
#include "dbgtoggl.h"
#include "dbgtoken.h"
#include "dbgerr.h"
#include "dbginfo.h"
#include "dbgmem.h"
#include "dbgbreak.h"
#include "dbghook.h"
#include "trpcore.h"
#include "dbgreg.h"
#include "mad.h"
#include "dui.h"
#include "tistrail.h"
#include <limits.h>


search_result           LineCue( mod_handle, cue_file_id,
                          unsigned long line, unsigned column, cue_handle *ch );
extern cue_file_id      CueFileId( cue_handle * );
extern unsigned         CueFile( cue_handle *ch, char *file, unsigned max );
extern unsigned long    CueLine( cue_handle *ch );
extern void             StdInNew( void );
extern void             StdOutNew( void );
extern void             Warn( char * );
extern unsigned int     ScanCmd( char * );
extern void             Scan( void );
extern bool             ScanItem( bool, char **, unsigned int * );
extern void             ReqEOC( void );
extern bool             KillProgOvlay( void );
extern void             ReportTask( task_status, unsigned );
extern void             BPsDeac( void );
extern void             BPsUnHit( void );
extern unsigned         DoLoad( char *, unsigned long * );
extern void             ClearMachState( void );
extern void             SetupMachState( void );
extern unsigned long    RemoteGetLibName( unsigned long, void *, unsigned );
extern unsigned         RemoteStringToFullName( bool, char *, char *, unsigned );
extern char             *GetCmdArg( int );
extern void             SetCmdArgStart( int, char * );
extern void             RemoteSplitCmd( char *, char **, char ** );
extern void             SymInfoMvHdl( handle, handle );
extern handle           PathOpen( char *, unsigned, char * );
extern handle           FullPathOpen( char *name, char *ext, char *result, unsigned max_result );
extern void             ChkExpr( void );
extern bool             ScanEOC( void );
extern char             *ReScan( char * );
extern unsigned         ReqExpr( void );
extern char             *ScanPos( void );
extern void             ReqMemAddr( memory_expr, address * );
extern void             SetNoSectSeg( void );
extern char             *Format( char *buff, char *fmt, ... );
extern void             TraceKill( void );
extern void             ActPoint( brkp *, bool );
extern void             AddAliasInfo( unsigned, unsigned );
extern void             FreeAliasInfo( void );
extern void             CheckSegAlias( void );
extern char             *StrCopy( char *, char * );
extern void             SetCodeDot( address );
extern address          GetRegIP( void );
extern bool             DlgGivenAddr( char *title, address *value );
extern void             SetLastExe( char *name );
extern void             SetPointAddr( brkp *bp, address addr );
extern void             RemoteMapAddr( addr_ptr *, addr_off *, addr_off *, unsigned long handle );
extern void             AddrSection( address *, unsigned );
extern void             VarFreeScopes( void );
extern void             VarUnMapScopes( image_entry * );
extern void             VarReMapScopes( image_entry * );
extern char             *DIPMsgText( dip_status );
extern address          GetRegSP( void );
extern bool             FindNullSym( mod_handle, address * );
extern bool             SetWDPresent( mod_handle );
extern void             RecordStart( void );
extern char             *GetCmdName( int index );
extern char             *GetCmdEntry( char *tab, int index, char *buff );
extern void             RecordEvent( char * );
extern bool             HookPendingPush( void );
extern char             *CheckForPowerBuilder( char * );
extern char             *DupStr( char * );
extern mod_handle       LookupImageName( char *start, unsigned len );
extern mod_handle       LookupModName( mod_handle search, char *start, unsigned len );
extern bool             GetBPSymAddr( brkp *bp, address *addr );
extern void             DbgUpdate( update_list );
extern long             RemoteGetFileDate( char *name );
extern long             LocalGetFileDate( char *name );
extern bool             RemoteSetFileDate( char *name, long date );
extern unsigned         RemoteErase( char const * );
extern unsigned         MaxRemoteWriteSize( void );

extern void             InsertRing( char_ring **owner, char *start, unsigned len );
extern void             DeleteRing( char_ring **owner, char *start, unsigned len );
extern void             FreeRing( char_ring *p );
extern char_ring        **RingEnd( char_ring **owner );

extern void             WndSetCmdPmt(char *,char *,unsigned int ,void (*)());
static bool             CopyToRemote( char *local, char *remote, bool strip, void *cookie );
char                    *RealFName( char *name, open_access *loc );

extern void             DUIImageLoaded( image_entry*, bool, bool, bool* );
extern void             FClearOpenSourceCache( void );

extern tokens           CurrToken;
extern char             *TxtBuff;
extern system_config    SysConfig;
extern brkp             UserTmpBrk;
extern brkp             *BrkList;
extern mod_handle       CodeAddrMod;
extern mod_handle       ContextMod;
extern image_entry      *DbgImageList;
extern dip_status       DIPStatus;
extern machine_state    *DbgRegs;
extern bool             DownLoadTask;

static char             *SymFileName;
static char             *TaskCmd;
static process_info     *CurrProcess; //NYI: multiple processes


char_ring               *LocalDebugInfo;

#define SYM_FILE_IND ':'

#if 0
#define DBGMSG
extern void __stdcall OutputDebugStringA( char * );
char dbgbuff[256];
#define DebugMsg( x, ... ) sprintf( dbgbuff, x, __VA_ARGS__ ); OutputDebugStringA( dbgbuff )
#define DebugMsgC( x ) OutputDebugStringA( x )
#else
#define DebugMsg( x, ... )
#define DebugMsgC( x )
#endif


bool InitCmd( void )
{
    unsigned    argc;
    char        *curr;
    char        *ptr;
    char        *end;
    char        *parm;
    char        *last;
    unsigned    total;
    char        *start;

    curr = GetCmdArg( 0 );
    if( curr != NULL ) {
        while( *curr == ' ' || *curr == '\t' ) ++curr;
        if( *curr == SYM_FILE_IND ) {
            ++curr;
            while( *curr == ' ' || *curr == '\t' ) ++curr;
            start = curr;
            while( *curr != ' ' && *curr != '\t' && *curr != '\0' ) ++curr;
            _Alloc( parm, curr - start + 1 );
            if( parm == NULL ) return( FALSE );
            SymFileName = parm;
            while( start < curr ) {
                *parm++ = *start++;
            }
            *parm = NULLCHAR;
            while( *curr == ' ' || *curr == '\t' ) ++curr;
            argc = 0;
            if( *curr == NULLCHAR ) {
                curr = GetCmdArg( ++argc );
            }
            SetCmdArgStart( argc, curr );
        }
    }
    argc = 0;
    total = 0;
    for( ;; ) {
        curr = GetCmdArg( argc );
        if( curr == NULL ) break;
        while( *curr != NULLCHAR ) {
            ++total;
            ++curr;
        }
        ++total;
        ++argc;
    }
    _Alloc( TaskCmd, total + 2 );
    if( TaskCmd == NULL ) return( FALSE );
    ptr = TaskCmd;
    argc = 0;
    for( ;; ) {
        curr = GetCmdArg( argc );
        if( curr == NULL ) break;
        while( *curr != NULLCHAR ) {
            *ptr = *curr++;
            if( *ptr == ARG_TERMINATE ) *ptr = ' ';
            ++ptr;
        }
        *ptr++ = NULLCHAR;
        ++argc;
    }
    *ptr = ARG_TERMINATE;
    last = ptr;
    RemoteSplitCmd( TaskCmd, &end, &parm );
    for( ptr = TaskCmd; ptr < end; ++ptr ) {
        if( *ptr == NULLCHAR ) *ptr = ' ';
    }
    memmove( ptr + 1, parm, last - parm + 1 );
    *ptr = NULLCHAR;
    ptr = TaskCmd;
    // If the program name was quoted, strip off the quotes
    if( *ptr == '\"' ) {
        memmove( ptr, ptr + 1, end - ptr );
        memmove( end - 2, end, last - end + 1 );
    }
    return( TRUE );
}

void FindLocalDebugInfo( char *name )
{
    char    *buff, *symname, *fname;
    char    *ext;
    int     len = strlen( name );
    handle  local;

    _AllocA( buff, len + 1 + 4 + 2 );
    _AllocA( fname, len + 1 );
    _AllocA( symname, len + 1 + 4 );
    strcpy( buff, "@l" );
    // If a .sym file is present, use it in preference to the .exe
    StrCopy( name, fname );
    ext = ExtPointer( fname, OP_LOCAL );
    if( *ext != NULLCHAR )
        *ext = NULLCHAR;
    local = FullPathOpen( fname, "sym", symname, len + 4 );
    if( local != NIL_HANDLE ) {
        strcat( buff, symname );
        FileClose( local );
    } else {
        strcat( buff, name );
    }
    InsertRing( RingEnd( &LocalDebugInfo ), buff, len + 4 + 2 );
}

static void DoDownLoadCode( void )
/********************************/
{
    handle local;

    if( !DownLoadTask ) return;
    local = FullPathOpen( TaskCmd, "exe", TxtBuff, TXT_LEN );
    if( local == NIL_HANDLE ) {
        Error( ERR_NONE, LIT( ERR_FILE_NOT_OPEN ), TaskCmd );
    }
    FileClose( local );
    FindLocalDebugInfo( TxtBuff );
    CopyToRemote( TxtBuff, SkipPathInfo( TxtBuff, OP_LOCAL ), TRUE, NULL );
}


bool DownLoadCode( void )
/***********************/
{
    return( Spawn( DoDownLoadCode ) == 0 );
}

void FiniCmd()
{
    _Free( TaskCmd );
}

void InitLocalInfo()
{
    LocalDebugInfo = NULL;
}

void FiniLocalInfo( void )
{
    FreeRing( LocalDebugInfo );
    LocalDebugInfo = NULL;
}

image_entry *ImagePrimary( void )
{
    return( DbgImageList );
}

image_entry *ImageEntry( mod_handle mh )
{
    image_entry         **image_ptr;

    image_ptr = ImageExtra( mh );
    return( (image_ptr == NULL) ? NULL : *image_ptr );
}

address DefAddrSpaceForMod( mod_handle mh )
{
    image_entry *image;
    address     def_addr;

    image = ImageEntry( mh );
    if( image == NULL ) image = ImagePrimary();
    if( image != NULL ) {
        return( image->def_addr_space );
    }
    def_addr = GetRegSP();
    def_addr.mach.offset = 0;
    return( def_addr );
}

address DefAddrSpaceForAddr( address addr )
{
    mod_handle  mod;

    if( DeAliasAddrMod( addr, &mod ) == SR_NONE ) mod = NO_MOD;
    return( DefAddrSpaceForMod( mod ) );
}

OVL_EXTERN void MapAddrSystem( image_entry *image, addr_ptr *addr,
                        addr_off *lo_bound, addr_off *hi_bound )
{
    RemoteMapAddr( addr, lo_bound, hi_bound, image->system_handle );
}

static bool InMapEntry( map_entry *curr, addr_ptr *addr )
{
    if( addr->segment != curr->map_addr.segment ) return( FALSE );
    if( addr->offset < curr->map_valid_lo ) return( FALSE );
    if( addr->offset > curr->map_valid_hi ) return( FALSE );
    return( TRUE );
}

void MapAddrForImage( image_entry *image, addr_ptr *addr )
{
    map_entry           **owner;
    map_entry           *curr;
    addr_ptr            map_addr;
    addr_off            lo_bound;
    addr_off            hi_bound;

    owner = &image->map_list;
    for( ;; ) {
        curr = *owner;
        if( curr == NULL ) break;
        if( curr->pre_map || InMapEntry( curr, addr ) ) {
            curr->map_addr = *addr;
            curr->pre_map = FALSE;
            addr->segment = curr->real_addr.segment;
            addr->offset += curr->real_addr.offset;
            return;
        }
        owner = &curr->link;
    }
    map_addr = *addr;
    image->mapper( image, addr, &lo_bound, &hi_bound );
    _Alloc( curr, sizeof( *curr ) );
    if( curr != NULL ) {
        curr->link = NULL;
        *owner = curr;
        curr->map_valid_lo = lo_bound;
        curr->map_valid_hi = hi_bound;
        curr->map_addr = map_addr;
        curr->map_addr.offset = 0;
        curr->real_addr = *addr;
        curr->real_addr.offset -= map_addr.offset;
        curr->pre_map = FALSE;
    }
}


bool UnMapAddress( mappable_addr *loc, image_entry *image )
{
    map_entry           *map;
    mod_handle          himage;

    if( image == NULL ) {
        if( DeAliasAddrMod( loc->addr, &himage ) == SR_NONE ) return( FALSE );
        image = ImageEntry( himage );
    }
    if( image == NULL ) return( FALSE );
    DbgFree( loc->image_name );
    loc->image_name = DupStr( image->image_name );
    for( map = image->map_list; map != NULL; map = map->link ) {
        if( map->real_addr.segment == loc->addr.mach.segment ) {
            loc->addr.mach.segment = map->map_addr.segment;
            loc->addr.mach.offset = loc->addr.mach.offset - map->real_addr.offset;
            return( TRUE );
        }
    }
    return( FALSE );
}


static void UnMapOnePoint( brkp *bp, image_entry *image )
{
    mod_handle          himage;
    if( bp->status.b.unmapped ) return;
    if( image != NULL ) {
        if( DeAliasAddrMod( bp->loc.addr, &himage ) == SR_NONE ) return;
        if( image != ImageEntry( himage ) ) return;
    }
    if( bp->image_name == NULL || bp->mod_name == NULL ) {
        bp->status.b.unmapped = UnMapAddress( &bp->loc, image );
    } else {
        bp->status.b.unmapped = TRUE;
    }
}


void UnMapPoints( image_entry *image )
{
    brkp                *bp;

    for( bp = BrkList; bp != NULL; bp = bp->next ) {
        UnMapOnePoint( bp, image );
    }
    if( UserTmpBrk.status.b.has_address ) {
        UnMapOnePoint( &UserTmpBrk, image );
    }
}


void FreeImage( image_entry *image )
{
    image_entry         **owner;
    image_entry         *curr;
    map_entry           *head;
    map_entry           *next;

    owner = &DbgImageList;
    for( ;; ) {
        curr = *owner;
        if( curr == NULL ) return;
        if( curr == image ) break;
        owner = &curr->link;
    }
    if( curr == ImageEntry( ContextMod ) ) {
        ContextMod = NO_MOD;
    }
    if( curr == ImageEntry( CodeAddrMod ) ) {
        CodeAddrMod = NO_MOD;
    }
    VarUnMapScopes( curr );
    UnMapPoints( curr );
    *owner = curr->link;
    for( head = curr->map_list; head != NULL; head = next ) {
        next = head->link;
        _Free( head );
    }
    _Free( curr->sym_name );
    _Free( curr );
}


static image_entry *DoCreateImage( char *exe, char *sym )
{
    image_entry         *image;
    image_entry         **owner;
    unsigned            len;


    len = (exe==NULL) ? 0 : strlen( exe );
    _ChkAlloc( image, sizeof( *image ) + len, LIT( ERR_NO_MEMORY_FOR_DEBUG ) );
    if( image == NULL ) return( NULL );
    memset( image, 0, sizeof( *image ) );
    if( len != 0 ) memcpy( image->image_name, exe, len + 1 );
    if( sym != NULL ) {
        _Alloc( image->sym_name, strlen( sym ) + 1 );
        if( image->sym_name == NULL ) {
            _Free( image );
            Error( ERR_NONE, LIT( ERR_NO_MEMORY_FOR_DEBUG ) );
            return( NULL );
        }
        strcpy( image->sym_name, sym );
    }
    image->mapper = MapAddrSystem;
    for( owner = &DbgImageList; *owner != NULL; owner = &(*owner)->link )
        {}
    *owner = image;
    return( image );
}

char *GetLastImageName( void )
{
    image_entry         *image;

    for( image = DbgImageList; image->link != NULL; image = image->link ) ;
    return( image->image_name );
}

static image_entry *CreateImage( char *exe, char *sym )
{
    image_entry         *image;
    bool                local;
    char                *curr_name;
    char                *curr_ext;
    char                curr_extchar;
    char                *this_name;
    char                *this_ext;
    char                this_extchar;
    char_ring           *curr;
    open_access         ind;

    if( exe != NULL && sym == NULL ) {
        local = FALSE;
        this_name = SkipPathInfo( exe, OP_REMOTE );
        this_ext = ExtPointer( exe, OP_REMOTE );
        this_extchar = *this_ext;
        *this_ext = '\0';
        for( curr = LocalDebugInfo; curr != NULL; curr = curr->next ) {
            curr_name = SkipPathInfo( curr->name, OP_LOCAL );
            curr_name = RealFName( curr_name, &ind );
            if( curr_name[0] == '@' && curr_name[1] == 'l' ) curr_name += 2;
            curr_ext = ExtPointer( curr->name, OP_LOCAL );
            curr_extchar = *curr_ext;
            *curr_ext = '\0';
            local = stricmp( this_name, curr_name ) == 0;
            *curr_ext = curr_extchar;
            if( local ) {
                sym = curr->name;
                break;
            }
        }
        *this_ext = this_extchar;
    }

    _SwitchOn( SW_ERROR_RETURNS );
    image = DoCreateImage( exe, sym );
    _SwitchOff( SW_ERROR_RETURNS );
    return( image );
}

static bool CheckLoadDebugInfo( image_entry *image, handle h,
                        unsigned start, unsigned end )
{
    char        buff[TXT_LEN];
    char        *name;
    unsigned    prio;
    char        *endstr;

	DebugMsg("WD, CheckLoadDebugInfo enter, image=%s start=%u, end=%u\r\n", image->sym_name ? image->sym_name : image->image_name, start, end );
    prio = start;
    for( ;; ) {
        prio = DIPPriority( prio );
		DebugMsg("WD, CheckLoadDebugInfo: prio=%u\r\n", prio );
		if( prio == 0 ) {
			DebugMsgC("WD, CheckLoadDebugInfo exit FALSE, prio=0\r\n");
			return( FALSE );
		}
		if( prio > end ) {
			DebugMsgC("WD, CheckLoadDebugInfo exit FALSE, prio > end\r\n" );
			return( FALSE );
		}
        DIPStatus = DS_OK;
        image->dip_handle = DIPLoadInfo( h, sizeof( image_entry * ), prio );
        if( image->dip_handle != NO_MOD ) break;
        if( DIPStatus & DS_ERR ) {
            name = image->sym_name;
            if( name == NULL ) name = image->image_name;
            endstr = Format( buff, LIT( Sym_Info_Load_Failed ), name );
            *endstr++ = ' ';
            StrCopy( DIPMsgText( DIPStatus ), endstr );
            Warn( buff );
			DebugMsgC("WD, CheckLoadDebugInfo exit FALSE, DIPStatus DS_ERR\r\n");
            return( FALSE );
        }
    }
    DebugMsgC("WD, CheckLoadDebugInfo exit TRUE\r\n");
    *(image_entry **)ImageExtra( image->dip_handle ) = image;
    return( TRUE );
}


/*
 * ProcSymInfo -- initialize symbolic information
 *
 * Note: This function should try to open files locally first, for two
 * reasons:
 * 1) If a local file is open as remote, then local caching may interfere with
 *    file operations (notably seeks with DIO_SEEK_CUR)
 * 2) Remote access goes through extra layer of indirection; this overhead
 *    is completely unnecessary for local debugging.
 */
static bool ProcSymInfo( image_entry *image )
{
    handle      h;
    unsigned    last;
    char        buff[TXT_LEN];
    char        *sym_name;
    char        *nopath;
    unsigned    len;

	DebugMsg("WD, ProcSymInfo enter, image=%s\r\n", image->sym_name ? image->sym_name : image->image_name );
    image->deferred_symbols = FALSE;
    if( _IsOff( SW_LOAD_SYMS ) ) return( NO_MOD );
    if( image->sym_name != NULL ) {
        last = DP_MAX;
        h = PathOpen( image->sym_name, strlen( image->sym_name ), "sym" );
        if( h == NIL_HANDLE ) {
            nopath = SkipPathInfo( image->sym_name, OP_REMOTE );
            h = PathOpen( nopath, strlen( nopath ), "sym" );
            if( h == NIL_HANDLE ) {
                /* try the sym file without an added extension */
                h = FileOpen( image->sym_name, OP_READ );
            }
        }
    } else {
        last = DP_EXPORTS-1;
        h = FileOpen( image->image_name, OP_READ );
        if( h == NIL_HANDLE ) {
            h = FileOpen( image->image_name, OP_READ | OP_REMOTE );
        }
    }
    if( h != NIL_HANDLE ) {
        if( CheckLoadDebugInfo( image, h, DP_MIN, last ) ) {
            return( TRUE );
        }
        FileClose( h );
    }
    if( image->sym_name != NULL ) return( FALSE );
	DebugMsgC("WD, ProcSymInfo MS 1\r\n" );
    _AllocA( sym_name, strlen( image->image_name ) + 1 );
    strcpy( sym_name, image->image_name );
    *ExtPointer( sym_name, OP_REMOTE ) = '\0';
    len = MakeFileName( buff, sym_name, "sym", OP_REMOTE );
    _Alloc( image->sym_name, len + 1 );
    if( image->sym_name != NULL ) {
        memcpy( image->sym_name, buff, len + 1 );
        h = FileOpen( image->sym_name, OP_READ );
        if( h == NIL_HANDLE ) {
            h = FileOpen( image->sym_name, OP_READ | OP_REMOTE );
        }
        if( h == NIL_HANDLE ) {
            h = PathOpen( image->sym_name, strlen( image->sym_name ), "" );
        }
        if( h != NIL_HANDLE ) {
            if( CheckLoadDebugInfo( image, h, DP_MIN, DP_MAX ) ) {
                return( TRUE );
            }
            FileClose( h );
        }
        _Free( image->sym_name );
    }
	DebugMsgC("WD, ProcSymInfo MS 2\r\n" );
    image->sym_name = NULL;
    if( _IsOff( SW_NO_EXPORT_SYMS ) ) {
        if( _IsOn( SW_DEFER_SYM_LOAD ) ) {
            image->deferred_symbols = TRUE;
        } else {
            h = FileOpen( image->image_name, OP_READ | OP_REMOTE );
            if( h != NIL_HANDLE ) {
                if( CheckLoadDebugInfo( image, h, DP_EXPORTS-1, DP_MAX ) ) {
                    return( TRUE );
                }
                FileClose( h );
            }
        }
    }
    return( FALSE );
}


void UnLoadSymInfo( image_entry *image, bool nofree )
{
    if( image->dip_handle != NO_MOD ) {
        image->nofree = nofree;
        DIPUnloadInfo( image->dip_handle );
        if( nofree ) {
            image->dip_handle = NO_MOD;
            image->nofree = FALSE;
        }
        DbgUpdate( UP_SYMBOLS_LOST );
        FClearOpenSourceCache();
    }
}

bool ReLoadSymInfo( image_entry *image )
{
    if( ProcSymInfo( image ) ) {
        DIPMapInfo( image->dip_handle, image );
        DbgUpdate( UP_SYMBOLS_ADDED );
        return( TRUE );
    }
    return( FALSE );
}


remap_return ReMapImageAddress( mappable_addr *loc, image_entry *image )
{
    map_entry           *map;

    if( loc->image_name == NULL ) {
        return( REMAP_WRONG_IMAGE );
    }
    if( strcmp( image->image_name, loc->image_name ) != 0 ) {
        return( REMAP_WRONG_IMAGE );
    }
    for( map = image->map_list; map != NULL; map = map->link ) {
        if( map->map_addr.segment == loc->addr.mach.segment ) {
            loc->addr.mach.segment = map->real_addr.segment;
            loc->addr.mach.offset = loc->addr.mach.offset + map->real_addr.offset;
            AddrSection( &loc->addr, OVL_MAP_CURR );
            DbgFree( loc->image_name );
            loc->image_name = NULL;
            return( REMAP_REMAPPED );
        }
    }
    return( REMAP_ERROR );
}

bool ReMapAddress( mappable_addr *loc )
{
    image_entry         *image;
    for( image = DbgImageList; image != NULL; image = image->link ) {
        if( ReMapImageAddress( loc, image ) == REMAP_REMAPPED ) return( TRUE );
    }
    return( FALSE );
}

static remap_return ReMapOnePoint( brkp *bp, image_entry *image )
{
    mod_handle  himage,mod;
    bool        ok;
    address     addr;
    DIPHDL( cue, ch );
    DIPHDL( cue, ch2 );
    remap_return        rc = REMAP_REMAPPED;

    if( !bp->status.b.unmapped ) return( REMAP_WRONG_IMAGE );
    if( bp->image_name == NULL || bp->mod_name == NULL ) {
        if( image == NULL ) {
            if( ReMapAddress( &bp->loc ) ) {
                rc = REMAP_REMAPPED;
            } else {
                rc = REMAP_ERROR;
            }
        } else {
            rc = ReMapImageAddress( &bp->loc, image );
        }
    } else {
        himage = LookupImageName( bp->image_name, strlen( bp->image_name ) );
        if( himage == NO_MOD ) return( REMAP_ERROR );
        mod =  LookupModName( himage, bp->mod_name, strlen( bp->mod_name ) );
        if( mod == NO_MOD ) return( REMAP_ERROR );
        ok = GetBPSymAddr( bp, &addr );
        if( !ok ) return( REMAP_ERROR );
        if( bp->cue_diff != 0 ) {
            if( DeAliasAddrCue( mod, addr, ch ) != SR_EXACT ) return( REMAP_ERROR );

            if( LineCue( mod, CueFileId( ch ), CueLine( ch ) + bp->cue_diff,
                         0, ch2 ) != SR_EXACT ) return( REMAP_ERROR );
            addr = CueAddr( ch2 );
        }
        if( bp->addr_diff != 0 ) {
            addr.mach.offset += bp->addr_diff;
        }
        bp->loc.addr = addr;
        rc = REMAP_REMAPPED;
    }
    if( rc == REMAP_REMAPPED ) {
        bp->status.b.unmapped = FALSE;
    }
    SetPointAddr( bp, bp->loc.addr );
    if( bp->status.b.activate_on_remap ) {
        ActPoint( bp, TRUE );
    }
    return( rc );
}


void ReMapPoints( image_entry *image )
{
    brkp        *bp;

    for( bp = BrkList; bp != NULL; bp = bp->next ) {
        switch( ReMapOnePoint( bp, image ) ) {
        case REMAP_ERROR:
            ActPoint( bp, FALSE );
            bp->status.b.activate_on_remap = TRUE;
            break;
        case REMAP_REMAPPED:
            bp->countdown = bp->initial_countdown;
            bp->total_hits = 0;
        }
    }
    if( UserTmpBrk.status.b.has_address ) {
        switch( ReMapOnePoint( &UserTmpBrk, image ) ) {
        case REMAP_ERROR:
// nobody cares about this warning!!        Warn( LIT( WARN_Unable_To_Remap_Tmp ) );
            UserTmpBrk.status.b.active = FALSE;
            break;
        }
    }
}


static void InitImageInfo( image_entry *image )
{
    if( !FindNullSym( image->dip_handle, &image->def_addr_space ) ) {
        image->def_addr_space = GetRegSP();
        image->def_addr_space.mach.offset = 0;
    }
    SetWDPresent( image->dip_handle );
    VarReMapScopes( image );
    ReMapPoints( image );
}


bool LoadDeferredSymbols( void )
{
    image_entry *image;
    bool        rc = FALSE;
    bool        defer;

    defer = _IsOn( SW_DEFER_SYM_LOAD );
    _SwitchOff( SW_DEFER_SYM_LOAD );
    for( image = DbgImageList; image != NULL; image = image->link ) {
        if( image->deferred_symbols ) {
            if( ReLoadSymInfo( image ) ) {
                InitImageInfo( image );
                image->deferred_symbols = FALSE;
                rc = TRUE;
            }
        }
    }
    if( defer ) _SwitchOn( SW_DEFER_SYM_LOAD );
    return( rc );
}


bool AddLibInfo( bool already_stopping, bool *force_stop )
{
    unsigned long       module;
    bool                added;
    bool                deleted;
    image_entry         *image;

    added = FALSE;
    deleted = FALSE;
    module = 0;
    for( ;; ) {
        module = RemoteGetLibName( module, TxtBuff, TXT_LEN );
        if( module == 0 ) break;
        if( TxtBuff[0] == NULLCHAR ) {
            deleted = TRUE;
            for( image = DbgImageList; image != NULL; image = image->link ) {
                if( image->system_handle == module ) {
                    DUIImageLoaded( image, FALSE, already_stopping, force_stop );
                    UnLoadSymInfo( image, FALSE );
                    break;
                }
            }
        } else {
            added = TRUE;
            image = CreateImage( TxtBuff, NULL );
            if( image != NULL ) {
                image->system_handle = module;
                if( ReLoadSymInfo( image ) ) {
                    InitImageInfo( image );
                }
                DUIImageLoaded( image, TRUE, already_stopping, force_stop );
            }
        }
    }
    CheckSegAlias();
    if( deleted ) {
        HookNotify( TRUE, HOOK_DLL_END );
    }
    if( added ) {
        HookNotify( TRUE, HOOK_DLL_START );
    }
    return( added );
}

static bool ProgStartHook = TRUE;

bool SetProgStartHook( bool new )
{
    bool        old;

    old = ProgStartHook;
    ProgStartHook = new;
    return( old );
}

static void WndNewProg( void )
{
    DUIWndDebug();
    CodeAddrMod = NO_MOD;
    ContextMod = NO_MOD;
    SetCodeDot( GetRegIP() );
    DbgUpdate( UP_NEW_SRC | UP_CSIP_CHANGE |
               UP_SYM_CHANGE |
               UP_REG_CHANGE | UP_MEM_CHANGE |
               UP_THREAD_STATE | UP_NEW_PROGRAM );
    if( ProgStartHook ) {
        HookNotify( FALSE, HOOK_PROG_START );
    }
    HookNotify( FALSE, HOOK_NEW_MODULE );
}

static int DoLoadProg( char *task, char *sym, unsigned *error )
{
    open_access         loc;
    char                *name;
    unsigned            len;
    static char         fullname[2048];
    image_entry         *image;
    handle              local;
    unsigned long       system_handle;

    *error = 0;
    #ifdef __NT__
        task = CheckForPowerBuilder( task );
    #endif
    if( task[0] == NULLCHAR ) return( TASK_NONE );
    name = FileLoc( task, &loc );
    if( DownLoadTask ) {
        strcpy( fullname, name );
        local = FullPathOpen( TaskCmd, "exe", TxtBuff, TXT_LEN );
        if( local != NIL_HANDLE ) {
            strcpy( fullname, TxtBuff );
            FileClose( local );
        }
    } else {
        len = RemoteStringToFullName(TRUE, name, fullname, sizeof(fullname));
        fullname[len] = '\0';
    }
    image = CreateImage( fullname, sym );
    if( image == NULL ) return( TASK_NOT_LOADED );
    if( DownLoadTask ) {
        name = SkipPathInfo( name, OP_LOCAL );
    }
    *error = DoLoad( name, &system_handle );
    if( *error != 0 ) {
        FreeImage( image );
        return( TASK_NOT_LOADED );
    }
    CheckSegAlias();
    image->system_handle = system_handle;
	SetLastExe( fullname );
    DebugMsgC("WD, DoLoadProg, calling ProcSymInfo\r\n");
    ProcSymInfo( image );
    DebugMsgC("WD, DoLoadProg, calling DIPMapInfo\r\n");
    if( image->dip_handle != NO_MOD ) {
		DebugMsgC("WD, DoLoadProg, calling DIPMapInfo really\r\n");
        DIPMapInfo( image->dip_handle, image );
    }
    DebugMsgC("WD, DoLoadProg, calling InitImageInfo\r\n");
    InitImageInfo( image );
    DebugMsgC("WD, DoLoadProg, exit\r\n");
    return( TASK_NEW );
}

void LoadProg( void )
{
    unsigned            error;
    int                 ret;
    unsigned long       system_handle;
    static char         NullProg[] = { NULLCHAR, NULLCHAR, ARG_TERMINATE };
    bool                dummy;

    ClearMachState();
    CurrProcess = DIPCreateProcess(); //NYI: multiple processes
    if( !DownLoadCode() ) {
        ret =  TASK_NOT_LOADED;
    } else {
        ret = DoLoadProg( TaskCmd, SymFileName, &error );
    }
    if( ret != TASK_NEW ) {
        CreateImage( NULL, NULL );
        DoLoad( NullProg, &system_handle );
    }
    /* need to do all these because we might be the QNX low level debugger */
    AddLibInfo( TRUE, &dummy );
    SetupMachState();
    WndNewProg();
    RecordStart();
    ReportTask( ret, error );
}




/*
 * ReleaseProgOvlay -- release segment that was allocated for the user program
 */

void ReleaseProgOvlay( bool free_sym )
{
    if( _IsOn( SW_PROC_ALREADY_STARTED ) ) {
        /* detaching from a running proc just lets it go */
        DUIStop();
    }
    if( CurrProcess != NULL ) {
        DIPDestroyProcess( CurrProcess );
        CurrProcess = NULL;
    }
    if( !KillProgOvlay() ) {
        Error( ERR_NONE, LIT( ERR_CANT_KILL_PROGRAM ) );
    }
    if( free_sym ) {
        _Free( SymFileName );
        SymFileName = NULL;
    }
    FreeAliasInfo();
    WndNewProg();
    while( DbgImageList != NULL ) FreeImage( DbgImageList );
}



OVL_EXTERN void BadNew( void )
{
    Error( ERR_LOC, LIT( ERR_BAD_OPTION ), "new" );
}


void InitMappableAddr( mappable_addr *loc )
{
    loc->image_name = NULL;
}

void FiniMappableAddr( mappable_addr *loc )
{
    if( loc->image_name != NULL ) DbgFree( loc->image_name );
}


unsigned GetProgName( char *where, unsigned len )
{
    unsigned    l;

    /*
        Before, we did a:

            RemoteStringToFullName( TRUE, TaskCmd, where, len );

        but that screws up when the user specified something other than
        just an executable on the command line. E.g. a PID to connect
        to a running process, or a NID specifier for QNX.
    */
    l = strlen( TaskCmd );
    if( l >= len ) l = len - 1;
    memcpy( where, TaskCmd, l );
    where[l] = NULLCHAR;
    return( l );
}

static bool ArgNeedsQuotes( char *src )
{
    char        ch;

    if( *src == NULLCHAR ) return( TRUE );
    for( ;; ) {
        ch = *src;
        if( ch == NULLCHAR ) return( FALSE );
        if( ch == ' ' ) return( TRUE );
        if( ch == '\t' ) return( TRUE );
        ++src;
    }
}

static void AddString( char **dstp, unsigned *lenp, char *src )
{
    unsigned    len;

    len = strlen( src );
    if( len > *lenp ) len = *lenp;
    memcpy( *dstp, src, len );
    *dstp += len;
    *lenp -= len;
}

static unsigned PrepProgArgs( char *where, unsigned len )
{
    char        *src;
    char        *dst;

    --len;      /* leave room for NULLCHAR */
    src = TaskCmd + strlen( TaskCmd ) + 1;
    dst = where;
    for( ;; ) {
        if( *src == ARG_TERMINATE ) break;
        if( dst != where ) AddString( &dst, &len, " " );
        if( _IsOn( SW_TRUE_ARGV ) && ArgNeedsQuotes( src ) ) {
            AddString( &dst, &len, "\"" );
            AddString( &dst, &len, src );
            AddString( &dst, &len, "\"" );
        } else {
            AddString( &dst, &len, src );
        }
        src += strlen( src ) + 1;
    }
    *dst = NULLCHAR;
    return( dst - where );
}

unsigned GetProgArgs( char *where, unsigned len )
{
    len = PrepProgArgs( where, len );
    _SwitchOff( SW_TRUE_ARGV );
    return( len );
}

void SetSymName( char *file )
{
    if( SymFileName ) _Free( SymFileName );
    _Alloc( SymFileName, strlen( file ) + 1 );
    strcpy( SymFileName, file );
}


static void DoResNew( bool have_parms, char *cmd,
                     unsigned clen, char *parms, unsigned plen )
{
    char                *new;

    TraceKill();
    new = DbgMustAlloc( clen + plen + 2 );
    ReleaseProgOvlay( FALSE );
    BPsUnHit();
    memcpy( new, cmd, clen );
    new[ clen ] = NULLCHAR;
    memcpy( &new[ clen + 1 ], parms, plen );
    new[ clen + plen + 1 ] = ARG_TERMINATE;
    _Free( TaskCmd );
    TaskCmd = new;
    if( have_parms ) _SwitchOff( SW_TRUE_ARGV );
    LoadProg();
}


extern void LoadNewProg( char *cmd, char *parms )
{
    unsigned clen,plen;
    char        prog[FILENAME_MAX];

    clen = strlen( cmd );
    plen = strlen( parms );
    GetProgName( prog, sizeof( prog ) );
    if( stricmp( cmd, prog ) == 0 ) {
        DoResNew( plen != 0, cmd, clen, parms, plen+1 );
    } else {
        BPsDeac();
        DoResNew( plen != 0, cmd, clen, parms, plen+1 );
        VarFreeScopes();
    }
}


static long SizeMinusDebugInfo( handle floc, bool strip )
/*******************************************************/
{
    TISTrailer          trailer;
    long                copylen;

    copylen = SeekStream( floc, 0, DIO_SEEK_END );
    if( !strip ) return( copylen );
    SeekStream( floc, -sizeof( trailer ), DIO_SEEK_END );
    if( ReadStream( floc, &trailer, sizeof(trailer) ) != sizeof(trailer) ) return( copylen );
    if( trailer.signature != TIS_TRAILER_SIGNATURE ) return( copylen );
    return( copylen - trailer.size );
}


static bool CopyToRemote( char *local, char *remote, bool strip, void *cookie )
/*****************************************************************************/
{
    handle              floc;
    handle              frem;
    unsigned            len;
    char                *buff;
    unsigned            bsize;
    long                copylen;
    long                copied;
    long                remdate;
    long                lcldate;

    bsize = MaxRemoteWriteSize();
    _Alloc( buff, bsize );
    if( buff == NULL ) {
        bsize = 128;
        buff = DbgMustAlloc( bsize );
    }
    #ifdef __NT__
        lcldate = LocalGetFileDate( local );
    #else
        lcldate = -1;
    #endif
    remdate = RemoteGetFileDate( remote );
    if( remdate != -1 && lcldate != -1 && remdate == lcldate ) return( TRUE );
    strip = strip; // nyi - strip debug info here
    floc = FileOpen( local, OP_READ );
    if( floc == NIL_HANDLE ) {
        Error( ERR_NONE, LIT( ERR_FILE_NOT_OPEN ), local );
        return( FALSE );
    }
    frem = FileOpen( remote, OP_REMOTE | OP_WRITE | OP_CREATE | OP_TRUNC | OP_EXEC );
    if( frem == NIL_HANDLE ) {
        Error( ERR_NONE, LIT( ERR_FILE_NOT_OPEN ), remote );
        FileClose( floc );
        return( FALSE );
    }
    copylen = SizeMinusDebugInfo( floc, strip );
    DUICopySize( cookie, copylen );
    SeekStream( floc, 0, DIO_SEEK_ORG );
    copied = 0;
    while( ( len = ReadStream( floc, buff, bsize ) ) != 0 ) {
        WriteStream( frem, buff, len );
        DUICopyCopied( cookie, copied );
        copied += len;
        if( copied >= copylen ) break;
        if( DUICopyCancelled( cookie ) ) {
            FileClose( floc );
            FileClose( frem );
            RemoteErase( remote );
            return( FALSE );
        }
    }
    FileClose( floc );
    FileClose( frem );
    RemoteSetFileDate( remote, lcldate );
    return( TRUE );
}


static unsigned ArgLen( char *p )
{
    char    *start;

    start = p;
    while( *p != ARG_TERMINATE ) ++p;
    return( p - start );
}


static void DoReStart( bool have_parms, unsigned clen, char *start, unsigned len )
{
    DoResNew( have_parms, TaskCmd, clen, start, len );
}


static void ResNew( void )
{
    char                *start;
    unsigned            len;
    unsigned            clen;
    bool                have_parms;

    clen = strlen( TaskCmd );
    if( ScanItem( FALSE, &start, &len ) ) {
        memcpy( TxtBuff, start, len );
        TxtBuff[ len++ ] = NULLCHAR;
        have_parms = TRUE;
    } else {
        start = &TaskCmd[ clen + 1 ];
        len = ArgLen( start );
        have_parms = FALSE;
    }
    ReqEOC();
    if( _IsOff( SW_PROC_ALREADY_STARTED ) && _IsOff( SW_POWERBUILDER ) ) {
        DoReStart( have_parms, clen, start, len );
    } else {
        Error( ERR_NONE, LIT( ERR_CANT_RESTART ) );
    }
}

void ReStart( void )
{
    char                prog[FILENAME_MAX];
    char                args[UTIL_LEN];

    if( _IsOff( SW_PROC_ALREADY_STARTED ) && _IsOff( SW_POWERBUILDER ) ) {
        GetProgName( prog, sizeof( prog ) );
        GetProgArgs( args, sizeof( args ) );
        LoadNewProg( prog, args );
    } else {
        Error( ERR_NONE, LIT( ERR_CANT_RESTART ) );
    }
}

/*
 *
 */

#define SKIP_SPACES     while( *start == ' ' && len != 0 ) { ++start; --len; }

static char NogoTab[] = {
    "NOgo\0"
};



static void ProgNew( void )
{
    char        *start;
    char        *cmd;
    char        *parm;
    char        *end;
    char        *new;
    char        *sym;
    unsigned    len;
    unsigned    clen;
    unsigned    plen;
    bool        have_parms;
    bool        old;
    bool        progstarthook;

    progstarthook = TRUE;
    if( CurrToken == T_DIV ) {
        Scan();
        if( ScanCmd( NogoTab ) != 1 ) {
            Error( ERR_LOC, LIT( ERR_BAD_OPTION ), "new" );
        }
        progstarthook = FALSE;
    }
    old = SetProgStartHook( progstarthook );
    if( ScanItem( FALSE, &start, &len ) ) {
        _Free( SymFileName );
        SymFileName = NULL;
        SKIP_SPACES;
        if( len > 1 && *start == SYM_FILE_IND ) {
            ++start;
            --len;
            SKIP_SPACES;
            if( len != 0 ) {
                sym = start;
                while( len != 0 && *start != ' ' ) {
                    ++start;
                    --len;
                }
                new = DbgMustAlloc( start - sym + 1 );
                SymFileName = new;
                memcpy( new, sym, start - sym );
                new[ start - sym ] = NULLCHAR;
                SKIP_SPACES;
            }
        }
        cmd = TxtBuff;
        memcpy( cmd, start, len );
        cmd[ len++ ] = NULLCHAR;
        cmd[ len ] = ARG_TERMINATE;
        RemoteSplitCmd( cmd, &end, &parm );
        clen = end - cmd;
        plen = len - (parm - cmd);
        have_parms = TRUE;
    } else {
        cmd = TaskCmd;
        clen = strlen( TaskCmd );
        parm = &TaskCmd[ clen + 1 ];
        plen = ArgLen( parm );
        have_parms = FALSE;
    }
    BPsDeac();
    ReqEOC();
    DoResNew( have_parms, cmd, clen, parm, plen );
    VarFreeScopes();
    SetProgStartHook( old );
}

#define NO_SEG          0

static void PostProcMapExpr( address *addr )
{
    addr_off            off;

    off = addr->mach.offset;
    if( _IsOff( SW_HAVE_SEGMENTS ) ) {
        *addr = GetRegSP();
        addr->mach.offset = off;
    } else if( addr->mach.segment == NO_SEG ) {
        addr->mach.segment = (addr_seg) off;
        addr->mach.offset = 0;
    }
}

static void EvalMapExpr( address *addr )
{
    addr->mach.segment = NO_SEG;
    ReqMemAddr( EXPR_GIVEN, addr );
    PostProcMapExpr( addr );
}

/*
 * MapAddrUser - have the user supply address mapping information
 */

OVL_EXTERN void MapAddrUser( image_entry *image, addr_ptr *addr,
                        addr_off *lo_bound, addr_off *hi_bound )
{
    address     mapped;
    addr_off    offset   = addr->offset;

    //NYI: what about bounds under Netware?
    *lo_bound = 0;
    *hi_bound = ~(addr_off)0;
    if( image->map_list != NULL && !image->map_list->pre_map
      && MADAddrMap( addr, &image->map_list->map_addr,
                &image->map_list->real_addr, &DbgRegs->mr ) == MS_OK ) {
        return;
    }
    for( ;; ) {
        switch( addr->segment ) {
        case MAP_FLAT_CODE_SELECTOR:
            Format( TxtBuff, LIT( Map_Named_Selector ), "Flat Code", image->sym_name );
            break;
        case MAP_FLAT_DATA_SELECTOR:
            Format( TxtBuff, LIT( Map_Named_Selector ), "Flat Data", image->sym_name );
            break;
        default:
            Format( TxtBuff, LIT( Map_Selector ), addr->segment, image->sym_name );
        }
        mapped.mach.segment = NO_SEG;
        mapped.mach.offset = 0;
        if( DlgGivenAddr( TxtBuff, &mapped ) ) {
            PostProcMapExpr( &mapped );
            mapped.mach.offset += offset;   // add offset back!
            *addr = mapped.mach;
            break;
        }
    }
}


/*
 * SymFileNew - process a new symbolic file request
 */

OVL_EXTERN void SymFileNew( void )
{
    char        *fname;
    unsigned    fname_len;
    image_entry *image;
    address     addr;
    map_entry   **owner;
    map_entry   *curr;
    char        *temp;
    addr_off    dummy;

    if( ! ScanItem( TRUE, &fname, &fname_len ) ) {
        Error( ERR_NONE, LIT( ERR_WANT_FILENAME ) );
    }
    temp = ScanPos();
    while( !ScanEOC() ) {
        ChkExpr();
        if( CurrToken == T_COMMA ) {
            Scan();
        }
    }
    ReScan( temp );
    memcpy( TxtBuff, fname, fname_len );
    TxtBuff[ fname_len ] = '\0';
    image = DoCreateImage( NULL, TxtBuff );
    image->mapper = MapAddrUser;
    if( !ProcSymInfo( image ) ) {
        FreeImage( image );
        Error( ERR_NONE, LIT( ERR_FILE_NOT_OPEN ), TxtBuff );
    }
    owner = &image->map_list;
    while( !ScanEOC() ) {
        EvalMapExpr( &addr );
        _Alloc( curr, sizeof( *curr ) );
        if( curr == NULL ) {
            DIPUnloadInfo( image->dip_handle );
            Error( ERR_NONE, LIT( ERR_NO_MEMORY_FOR_DEBUG ) );
        }
        *owner = curr;
        owner = &curr->link;
        curr->link = NULL;
        curr->pre_map = TRUE;
        curr->real_addr = addr.mach;
        curr->map_valid_lo = 0;
        curr->map_valid_hi = ~(addr_off)0;
        curr->map_addr.offset  = 0;
        curr->map_addr.segment = 0;
        if( CurrToken == T_COMMA ) {
            Scan();
        }
    }
    DIPMapInfo( image->dip_handle, image );
    curr = image->map_list->link;
    if( _IsOn( SW_HAVE_SEGMENTS )
     && (MADAddrFlat( &DbgRegs->mr ) == MS_OK)
     && (curr == NULL || curr->pre_map) ) {
        /* FLAT model program */
        if( curr == NULL ) {
            MapAddrUser( image, &addr.mach, &dummy, &dummy );
        } else {
            addr.mach = curr->real_addr;
        }
        AddAliasInfo( image->map_list->real_addr.segment, addr.mach.segment );
    }
    DbgUpdate( UP_SYMBOLS_ADDED );
    InitImageInfo( image );
}


/*
 * MapAddrUsrMod - simple address mapping for user loaded modules
 */

OVL_EXTERN void MapAddrUsrMod( image_entry *image, addr_ptr *addr,
                        addr_off *lo_bound, addr_off *hi_bound )
{
    address     mapped;
    addr_off    offset   = addr->offset;

    *lo_bound = 0;
    *hi_bound = ~(addr_off)0;
    if( image->map_list != NULL && !image->map_list->pre_map
      && MADAddrMap( addr, &image->map_list->map_addr,
                &image->map_list->real_addr, &DbgRegs->mr ) == MS_OK ) {
        return;
    }
    mapped.mach.segment = NO_SEG;
    mapped.mach.offset  = 0;

    // Assumes flat model images with single base address
    if( image->map_list != NULL ) {
        mapped.mach = image->map_list->real_addr;

        PostProcMapExpr( &mapped );
        mapped.mach.offset += offset;   // add offset back!
        *addr = mapped.mach;
    }
}


/*
 * SymUserModLoad - process symbol information for user loaded module
 *                  NB: assumes flat model
 */

bool SymUserModLoad( char *fname, address *loadaddr )
{
    unsigned    fname_len;
    image_entry *image;
    map_entry   **owner;
    map_entry   *curr;

    if( !fname )
        return( TRUE );

    if( !( fname_len = strlen( fname ) ) )
        return( TRUE );

    image = DoCreateImage( fname, fname );
    image->mapper = MapAddrUsrMod;
    if( !ProcSymInfo( image ) ) {
        FreeImage( image );
        Error( ERR_NONE, LIT( ERR_FILE_NOT_OPEN ), fname );
    }
    owner = &image->map_list;

    _Alloc( curr, sizeof( *curr ) );
    if( curr == NULL ) {
        DIPUnloadInfo( image->dip_handle );
        Error( ERR_NONE, LIT( ERR_NO_MEMORY_FOR_DEBUG ) );
    }

    // Save off the load address in a map entry
    *owner = curr;
    owner = &curr->link;
    curr->link    = NULL;
    curr->pre_map = TRUE;
    curr->map_valid_lo = 0;
    curr->map_valid_hi = ~(addr_off)0;
    curr->real_addr    = loadaddr->mach;
    curr->map_addr.offset  = 0;
    curr->map_addr.segment = 0;

    DIPMapInfo( image->dip_handle, image );
    curr = image->map_list->link;
    DbgUpdate( UP_SYMBOLS_ADDED );
    InitImageInfo( image );
    return( FALSE );
}


/*
 * SymUserModUnload - unload symbol information for user loaded module
 */

bool SymUserModUnload( char *fname )
{
    image_entry *image;

    if( fname ) {
        for( image = ImagePrimary(); image != NULL; image = image->link ) {
            if( image->sym_name && ( strcmp( image->sym_name, fname ) == 0 ) ) {
                UnLoadSymInfo( image, FALSE );
                return( FALSE );
            }
        }
    }
    return( TRUE );
}

static char NewNameTab[] = {
    "Program\0"
    "Restart\0"
    "STDIn\0"
    "STDOut\0"
    "SYmbol\0"
};


static void (* const NewJmpTab[])( void ) = {
    &BadNew,
    &ProgNew,
    &ResNew,
    &StdInNew,
    &StdOutNew,
    &SymFileNew
};


/*
 *
 */

void ProcNew( void )
{
    if( CurrToken == T_DIV ) {
        Scan();
        (*NewJmpTab[ ScanCmd( &NewNameTab ) ])();
    } else {
        ResNew();
    }
    HookPendingPush();
}

void RecordNewProg( void )
{
    char        buff[40];
    char        *p;

    GetCmdEntry( NewNameTab, 1, buff );
    p = Format( TxtBuff, "%s/%s", GetCmdName( CMD_NEW ), buff );
    if( !ProgStartHook ) {
        GetCmdEntry( NogoTab, 1, buff );
        p = Format( p, "/%s", buff );
    }
    *p++ = ' ';
    p += GetProgName( p, TXT_LEN - (p-TxtBuff) );
    *p++ = ' ';
    PrepProgArgs( p, TXT_LEN - (p-TxtBuff) );
    RecordEvent( TxtBuff );
}
