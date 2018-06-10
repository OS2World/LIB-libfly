#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <stdarg.h>

#define INCL_SUB
#define INCL_DOS
#define INCL_WIN
#define INCL_AVIO
#define INCL_WINSWITCHLIST
#include <os2.h>
#include <os2thunk.h>

#include <stdarg.h>
#include <string.h>

#define CMD_GET  1
#define CMD_PUT  2

static HAB         hab;
static HMQ         hmq;
static SWCNTRL     swdata;
static HSWITCH     hsw;
static HWND        hwndFrame;
static PPIB        pib;

static int         have_pm, windowed;

static PFN pWinInitialize, pWinTerminate, pWinCreateMsgQueue, pWinDestroyMsgQueue;
static PFN pWinQueryWindowText, pWinSetWindowText;
static PFN pWinChangeSwitchEntry, pWinQuerySwitchEntry, pWinQuerySwitchHandle;
static PFN pWinCloseClipbrd, pWinOpenClipbrd, pWinQueryClipbrdData, pWinSetClipbrdData, pWinEmptyClipbrd;
static PFN pWinSetWindowPos, pWinQueryWindowPos;
static _far16ptr pmshapi_97_addr = 0;

static int          tiled_size = 65535; /* 160*80*4 */
static char         *tiled1;
static USHORT       *us1_16, *us2_16;
static VIOMODEINFO  *saved_mode;
static VIOCURSORINFO  *pci;

static char  *saved_screen;
static int   saved_row, saved_col;

static int convert_to_asv_key (KBDKEYINFO  k);
static int check_mouse (void);
static void load_pm_pointers (void);

static HMOU          MouHandle;

/* ------------------------------------------------------------- */

void fly_init (int x0, int y0, int rows, int cols, char *font)
{
    KBDINFO      stat1, stat2, *pstat;
    int          r, offset;
    int          rc;
    PTIB         tib;
    
    have_pm  = FALSE;
    windowed = FALSE;
    
    load_pm_pointers ();
    if (!have_pm) goto no_pm;

    // morph into PM program
    rc = DosGetInfoBlocks (&tib, &pib);
    if (rc) {have_pm = FALSE; goto no_pm;}
    if (pib->pib_ultype == 2) windowed = TRUE;
    pib->pib_ultype = 3;

    // create message queue
    hab = pWinInitialize (0);
    if (hab == NULLHANDLE) {have_pm = FALSE; goto no_pm;}
    
    hmq = pWinCreateMsgQueue (hab, 0);
    if (hmq == 0)
    {
        pWinTerminate (hab);
        have_pm = FALSE;
        goto no_pm;
    }

    // init switch entry
    hsw = pWinQuerySwitchHandle (0, pib->pib_ulpid);
    if (hsw == NULLHANDLE) {have_pm = FALSE; goto no_pm;}
    
    rc = pWinQuerySwitchEntry (hsw, &swdata);
    if (rc) {have_pm = FALSE; goto no_pm;}
    hwndFrame = swdata.hwnd;

no_pm:
    tiled1 = _tmalloc (tiled_size);
    if (tiled1 == NULL) fly_error ("cannot allocate tiled memory\n");
    us1_16 = _tmalloc (sizeof(USHORT));
    us2_16 = _tmalloc (sizeof(USHORT));
    pstat = _THUNK_PTR_STRUCT_OK (&stat1) ? &stat1 : &stat2;
    pci = _tmalloc (sizeof(VIOCURSORINFO));

    // switch keyboard into binary mode
    pstat->cb = sizeof (stat1);
    KbdGetStatus (pstat, 0);
    pstat->fsMask |= KEYBOARD_BINARY_MODE;
    pstat->fsMask &= ~KEYBOARD_ASCII_MODE;
    KbdSetStatus (pstat, 0);

    /* determine terminal dimensions */
    saved_mode = _tmalloc (sizeof(VIOMODEINFO));
    saved_mode->cb = sizeof (VIOMODEINFO);
    VioGetMode (saved_mode, 0);

    // save screen contents and initial cursor position
    saved_screen = _tmalloc (saved_mode->row * saved_mode->col * 2);
    for (r=0; r<saved_mode->row; r++)
    {
        offset = r*saved_mode->col*2;
        *us1_16 = saved_mode->col*2;
        VioReadCellStr (saved_screen+offset, us1_16, r, 0, 0);
    }
    VioGetCurPos (us1_16, us2_16, 0);
    saved_row = *us1_16;
    saved_col = *us2_16;
    
    video_init (saved_mode->row, saved_mode->col);
    
    fly_mouse (FALSE);
    fl_opt.initialized = TRUE;

    if (rows != -1 && cols != -1)
    {
        video_set_window_size (rows, cols);
    }
    if (x0 != -1 && y0 != -1)
    {
        video_set_window_pos (x0, y0);
    }
}

/* ------------------------------------------------------------- */

void fly_terminate (void)
{
    KBDINFO      stat1, stat2, *pstat;
    //VIOMODEINFO  vi1, vi2, *pvi;
    int          r, offset;

    // switch keyboard into ascii mode
    pstat = _THUNK_PTR_STRUCT_OK (&stat1) ? &stat1 : &stat2;
    pstat->cb = sizeof (stat1);
    KbdGetStatus (pstat, 0);
    pstat->fsMask &= ~KEYBOARD_BINARY_MODE;
    pstat->fsMask |=  KEYBOARD_ASCII_MODE;
    KbdSetStatus (pstat, 0);
    
    video_terminate ();

    /* set terminal dimensions back */
    VioSetMode (saved_mode, 0);
    
    // restore screen contents and initial cursor position
    for (r=0; r<saved_mode->row; r++)
    {
        offset = r*saved_mode->col*2;
        VioWrtCellStr (saved_screen+offset, saved_mode->col*2, r, 0, 0);
    }
    _tfree (saved_screen);
    _tfree (saved_mode);
    VioSetCurPos (saved_row, saved_col, 0);
    
    fly_mouse (FALSE);
    
    // show cursor
    set_cursor (1);
    
    _tfree (tiled1);
    _tfree (us1_16);
    _tfree (us2_16);
    
    fl_opt.initialized = FALSE;
}

/* ------------------------------------------------------------- */

void fly_mouse (int enabled)
{
    USHORT  eventmask, nb;
    
    switch (enabled)
    {
    case TRUE:
        if (fl_opt.mouse_active == TRUE) break;
        MouOpen (NULL, &MouHandle);
        MouGetNumButtons (&nb, MouHandle);
        fl_opt.mouse_nb = nb;
        MouGetEventMask (&eventmask, MouHandle);
        eventmask = (MOUSE_MOTION |
                     MOUSE_BN1_DOWN |
                     MOUSE_BN2_DOWN |
                     MOUSE_BN3_DOWN |
                     MOUSE_MOTION_WITH_BN1_DOWN |
                     MOUSE_MOTION_WITH_BN2_DOWN |
                     MOUSE_MOTION_WITH_BN3_DOWN);
        MouSetEventMask (&eventmask, MouHandle);
        MouDrawPtr (MouHandle);
        break;
        
    case FALSE:
        if (fl_opt.mouse_active == FALSE) break;
        MouClose (MouHandle);
        break;
    }
    
    fl_opt.mouse_active = enabled;
}

/* ------------------------------------------------------------- */

int getmessage (int interval)
{
    KBDKEYINFO     k1, k2, *pk;
    double         timergoal;
    int            key;

    pk = _THUNK_PTR_STRUCT_OK (&k1) ? &k1 : &k2;
    
    timergoal = clock1 () + (double)interval/1000.;
    do
    {
        key = -1;
        KbdCharIn (pk, IO_NOWAIT, 0);
        if (pk->fbStatus != 0)
        {
            key = convert_to_asv_key (*pk);
        }
        else if (fl_opt.mouse_active == TRUE)
        {
            key = check_mouse ();
        }
        
        if (key != -1) return key;
        if (interval == 0) return 0;

        DosSleep (1);
    }
    while (interval == -1 || timergoal > clock1() );

    return 0;
}

/* -------------------------------------------------------------
 state diagrams (# means event is reported):
 1) single click
 . B1 down, others up #
 2) double click
 . B1 down, others up (SC is reported) t1
 . B1 up
 . B1 down, t2-t1 is small enough,     t2
 .    coordinates of all events are the same #
 3) drag
 . B1 down, others up (SC is reported)
 . coordinates change (MS is reported, then ) #
 ------------------------------------------------------------- */

static int check_mouse (void)
{
    MOUEVENTINFO     m1, m2, *pm;
    USHORT           wait = MOU_NOWAIT;
    ULONG            now;
    int              rc, b1, b2, b3;

    while (1)
    {
        pm = _THUNK_PTR_STRUCT_OK (&m1) ? &m1 : &m2;
        rc = MouReadEventQue (pm, &wait, MouHandle);
        if (rc != 0 || pm->time == 0) break;
        b1 = (pm->fs & MOUSE_BN1_DOWN) || (pm->fs & MOUSE_MOTION_WITH_BN1_DOWN) ? 1 : 0;
        b2 = (pm->fs & MOUSE_BN2_DOWN) || (pm->fs & MOUSE_MOTION_WITH_BN2_DOWN) ? 1 : 0;
        b3 = (pm->fs & MOUSE_BN3_DOWN) || (pm->fs & MOUSE_MOTION_WITH_BN3_DOWN) ? 1 : 0;
        mouse_received (b1, b2, b3, pm->time, pm->row, pm->col);
    }
    
    DosQuerySysInfo (QSV_MS_COUNT, QSV_MS_COUNT, &now, sizeof (now));

    return mouse_check (now);
}

/* ------------------------------------------------------------- */

static int convert_to_asv_key (KBDKEYINFO  k)
{
    // no keypress?
    if (k.fbStatus == 0) return -1;

    // handle russian 'p'
    if (k.chChar == 0xE0 && k.chScan == 0) return 0xE0;
    
    // catch shift-insert
    if ((k.fsState & (KBDSTF_RIGHTSHIFT | KBDSTF_LEFTSHIFT)) &&
        k.chScan == 82)
        return _ShiftInsert;
    
    return ( (k.chChar==0) || (k.chChar==0xE0) ) ?
        __EXT_KEY_BASE__+k.chScan : k.chChar;
}

/* ------------------------------------------------------------- */

void set_cursor (int flag)
{
    static int       visibility = 0;

    VioGetCurType (pci, 0);
    
    if (flag == 0)
    {
        if (pci->attr == 0xFFFF) return;
        visibility = pci->attr;
        pci->attr = 0xFFFF;
        VioSetCurType (pci, 0);
        return;
    }
    else
    {
        if (pci->attr != 0xFFFF) return;
        pci->attr = visibility;
        VioSetCurType (pci, 0);
        return;
    }
}

/* ------------------------------------------------------------- */

void move_cursor (int row, int col)
{
    VioSetCurPos (row, col, 0);
}

/* ------------------------------------------------------------- */

void screen_draw (char *s, int len, int row, int col, char *a)
{
    NOPTRRECT  remove;

    remove.row = 0;
    remove.col = 0;
    remove.cRow = video_vsize ()-1;
    remove.cCol = video_hsize ()-1;
    
    memcpy (tiled1, s, len);
    
    if (fl_opt.mouse_active)
        MouRemovePtr (&remove, MouHandle);
    
    VioWrtCharStrAtt (tiled1, len, row, col, a, 0);
    
    if (fl_opt.mouse_active)
        MouDrawPtr (MouHandle);
}

/* ------------------------------------------------------------- */

void video_update1 (int *forced)
{
}

/* ------------------------------------------------------------- */

void fly_launch (char *command, int wait, int pause)
{
    KBDINFO       stat1, stat2, *pstat;
    int           mouse_state = fl_opt.mouse_active;
    char          *cmd, *p;
    
    if (wait)
    {
        // switch keyboard into ascii mode
        pstat = _THUNK_PTR_STRUCT_OK (&stat1) ? &stat1 : &stat2;
        pstat->cb = sizeof (stat1);
        KbdGetStatus (pstat, 0);
        pstat->fsMask &= ~KEYBOARD_BINARY_MODE;
        pstat->fsMask |=  KEYBOARD_ASCII_MODE;
        KbdSetStatus (pstat, 0);
        VioSetCurPos (0, 0, 0);
        fly_mouse (FALSE);

        p = get_window_name ();
        set_window_name ("running %s...", command);
        set_cursor (1);
        system ("cls");
        system (command);
        if (pause)
        {
            fprintf (stderr, "\nPress ENTER...");
            fgetc (stdin);
        }
        set_cursor (video_cursor_state (-1));
        set_window_name (p);
        free (p);

        // switch keyboard into binary mode
        pstat->cb = sizeof (stat1);
        KbdGetStatus (pstat, 0);
        pstat->fsMask |= KEYBOARD_BINARY_MODE;
        pstat->fsMask &= ~KEYBOARD_ASCII_MODE;
        KbdSetStatus (pstat, 0);
        if (mouse_state) fly_mouse (TRUE);
        set_cursor (0);
        
        video_update (1);
    }
    else
    {
        cmd = malloc (strlen (command)+32);
        snprintf1 (cmd, strlen (command)+32, "detach %s >nul 2>&1", command);
        system (cmd);
        free (cmd);
        
        video_update (1);
    }
}

/* ------------------------------------------------------------- */

void video_set_window_pos (int x0, int y0)
{
    SWP   swp;
    int   rc;
    
    if (!windowed) return;
    rc = pWinQueryWindowPos (hwndFrame, &swp);
    if (!rc) return;
    pWinSetWindowPos (hwndFrame, HWND_TOP, x0, y0, swp.cx, swp.cy, SWP_MOVE);
}

/* ------------------------------------------------------------- */

int video_set_window_size (int rows, int cols)
{
    VIOMODEINFO mode;
    int         rc;

    if (rows == video_vsize() && cols == video_hsize()) return 0;
    mode.cb = sizeof (mode);
    rc = VioGetMode (&mode, 0);
    debug_tools ("rc = %d after VioGetMode\n", rc);
    if (rc) return 1;
    mode.row = rows;
    mode.col = cols;
    rc = VioSetMode (&mode, 0);
    if (rc) return 1;
    debug_tools ("rc = %d after VioSetMode (%d, %d)\n", rc, rows, cols);
    video_init (rows, cols);
    return 0;
}

/* ------------------------------------------------------------- */

int video_get_window_pos (int *x0, int *y0)
{
    SWP   swp;
    int   rc;
    
    *x0 = 0;
    *y0 = 0;
    
    if (!have_pm) return 1;
    if (!windowed) return 1;
    rc = pWinQueryWindowPos (hwndFrame, &swp);
    if (!rc) return 1;
    
    *x0 = swp.x;
    *y0 = swp.y;
    return 0;
}

/* ------------------------------------------------------------- */

void set_window_name (char *format, ...)
{
    va_list  args;
    char     buffer[8192];

    if (!have_pm) return;

    /* set up new title */
    va_start (args, format);
    vsnprintf1 (buffer, sizeof(buffer), format, args);
    va_end (args);
    buffer[MAXNAMEL] = '\0';

    /* WinSetWindowText */
    pWinSetWindowText (hwndFrame, buffer);
    
    /* switch entry */
    strncpy (swdata.szSwtitle, buffer, MAXNAMEL);
    swdata.szSwtitle[MAXNAMEL] = '\0';
    pWinChangeSwitchEntry (hsw, &swdata);
}

/* ------------------------------------------------------------- */

int WinSetTitleAndIcon (char *title, char *icon)
{
  return ((short)(_THUNK_PROLOG (4+4);
                  _THUNK_FLAT (title);
                  _THUNK_FLAT (icon);
                  _THUNK_CALLI (pmshapi_97_addr)));
}

/* ------------------------------------------------------------- */

void set_icon (char *filename)
{
    char     buffer[1024], *b1, *b2;
    int      n, rc;
    PTIB     tib;
    PPIB     pib1;
    
    if (pmshapi_97_addr == 0) return;

    buffer[0] = '\0';
    n = pWinQueryWindowText (hwndFrame, sizeof (buffer), buffer);
    if (n <= 0 || buffer[0] == '\0') return;

    str_translate (filename, '/', '\\');
    b1 = _tmalloc (strlen (buffer)+1);
    strcpy (b1, buffer);
    b2 = _tmalloc (strlen (filename)+1);
    strcpy (b2, filename);
    
    // morph into VIO program
    rc = DosGetInfoBlocks (&tib, &pib1);
    if (rc) return;
    pib->pib_ultype = 2;
    
    WinSetTitleAndIcon (b1, b2);
    
    // morph back into PM program
    pib->pib_ultype = 3;
    
    _tfree (b1);
    _tfree (b2);
    
}

/* ------------------------------------------------------------- */

char *get_window_name (void)
{
    char     buffer[1024];
    int      n;
    
    if (!have_pm) return strdup (" ");

    buffer[0] = '\0';
    n = pWinQueryWindowText (hwndFrame, sizeof (buffer), buffer);
    
    if (n <= 0 || buffer[0] == '\0') return strdup (" ");
    return strdup (buffer);
}

/* ------------------------------------------------------------- */

static void load_pm_pointers (void)
{
    HMODULE   hm = 0;
    int       rc;
    char      failbuf[512];
    PFN       pfn;

    have_pm = FALSE;
    
    // first of all we check for presence of PMWIN; if absent,
    // then running textmode-only (TSHELL?)
    
    rc = DosLoadModule (failbuf, sizeof(failbuf), "PMWIN", &hm);
    if (rc) return;

    rc = 0;
    rc += DosQueryProcAddr (hm, 763, 0, (PFN*)&pWinInitialize);
    rc += DosQueryProcAddr (hm, 888, 0, (PFN*)&pWinTerminate);
    rc += DosQueryProcAddr (hm, 716, 0, (PFN*)&pWinCreateMsgQueue);
    rc += DosQueryProcAddr (hm, 726, 0, (PFN*)&pWinDestroyMsgQueue);
    rc += DosQueryProcAddr (hm, 841, 0, (PFN*)&pWinQueryWindowText);
    rc += DosQueryProcAddr (hm, 877, 0, (PFN*)&pWinSetWindowText);
    rc += DosQueryProcAddr (hm, 793, 0, (PFN*)&pWinOpenClipbrd);
    rc += DosQueryProcAddr (hm, 707, 0, (PFN*)&pWinCloseClipbrd);
    rc += DosQueryProcAddr (hm, 806, 0, (PFN*)&pWinQueryClipbrdData);
    rc += DosQueryProcAddr (hm, 854, 0, (PFN*)&pWinSetClipbrdData);
    rc += DosQueryProcAddr (hm, 733, 0, (PFN*)&pWinEmptyClipbrd);
    rc += DosQueryProcAddr (hm, 837, 0, (PFN*)&pWinQueryWindowPos);
    rc += DosQueryProcAddr (hm, 875, 0, (PFN*)&pWinSetWindowPos);

    if (rc) return;

    rc = DosLoadModule (failbuf, sizeof(failbuf), "PMSHAPI", &hm);
    if (rc) return;

    rc = 0;
    rc += DosQueryProcAddr(hm, 123, 0, (PFN*)&pWinChangeSwitchEntry);
    rc += DosQueryProcAddr(hm, 124, 0, (PFN*)&pWinQuerySwitchEntry);
    rc += DosQueryProcAddr(hm, 125, 0, (PFN*)&pWinQuerySwitchHandle);
    if (rc) return;
    
    rc = DosQueryProcAddr(hm, 97,  0, (PFN*)&pfn);
    if (rc == 0)
        pmshapi_97_addr = _emx_32to16 (pfn);

    have_pm = TRUE;
}

/* ------------------------------------------------------------- */

char *get_clipboard (void)
{
    char *text;
    
    if (!have_pm) return NULL;

    // to make sure...
    pWinCloseClipbrd (hab);
    
    if (pWinOpenClipbrd (hab) != TRUE) return NULL;
    
    text = (char *) pWinQueryClipbrdData (hab, CF_TEXT);
    
    // copy and fix if necessary
    if (text != NULL)
    {
        text = strdup (text);
    }
    
    pWinCloseClipbrd (hab);

    return text;
}

/* ------------------------------------------------------------- */

void put_clipboard (char *s)
{
    char  *text;
    int   len;
    
    if (!have_pm) return;

    // to make sure...
    pWinCloseClipbrd (hab);
    
    if (pWinOpenClipbrd (hab) != TRUE) return;
    
    pWinEmptyClipbrd (hab);
    
    len = strlen (s);
    
    if (len)
    {
        DosAllocSharedMem ((void **)&text, 0, len + 1,
                           PAG_WRITE | PAG_COMMIT | OBJ_GIVEABLE);
        strcpy (text, s);
        pWinSetClipbrdData (hab, (ULONG) text, CF_TEXT, CFI_POINTER);
    }
    
    pWinCloseClipbrd (hab);
}

/* ------------------------------------------------------------- */

char *fly_get_font (void)
{
    return strdup ("default font");
}

/* ------------------------------------------------------------- */

int fly_get_fontlist (flyfont **F, int restrict_by_language)
{
    return -1;
}

/* ------------------------------------------------------------- */

void beep2 (int frequency, int duration)
{
    DosBeep (frequency, duration);
}

/* ------------------------------------------------------------- */

int fly_set_font (char *fontname)
{
    return 1;
}

/* ------------------------------------------------------------- */

void fly_process_args (int *argc, char **argv[], char ***envp)
{
}

/* ------------------------------------------------------------- */

void video_flush (void)
{
}

/* ------------------------------------------------------------- */

int ifly_ask_gui (int flags, char *questions, char *answers)
{
    return ifly_ask_txt (flags, questions, answers);
}

/* ------------------------------------------------------------- */

void ifly_error_gui (char *message)
{
    ifly_error_txt (message);
}
