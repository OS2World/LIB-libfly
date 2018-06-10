#define INCL_WIN
#define INCL_DOS
#define INCL_VIO
#define INCL_AVIO
//#define INCL_DOSPROCESS
//#define INCL_DOSSEMAPHORES
//#define INCL_DOSSESMGR
//#define INCL_DOSQUEUES
#include <os2.h>

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <unistd.h>

#include "menu.h"

#define MAX_DELTA 1

static int    tiled_size = 65535; /* 160*80*4 */
static char   *tiled1;
static USHORT *us1_16, *us2_16;

static HAB      hab, hab0;
static HMQ      hmq, hmq0;
static HWND     hwndFrame, hwndClient, hwnd0;
static QMSG     qmsg;
static SWCNTRL  swdata;
static HSWITCH  hsw;
static HDC      hdc = 0;
static PPIB     pib;
static PTIB     tib;

static HMTX   mtx_Queue, mtx_Video;
static HEV    hev_NewMessage, hev_VideoReady;

static HPS    hps  = 0;
static HVPS   hvps = 0;
static VIOCURSORINFO  *pci;

static USHORT cxChar = 0, cyChar = 0;

static int vio_rows = 25, vio_cols = 80;
static int item_status_change = 0;

MRESULT EXPENTRY FlyWndProc (HWND, ULONG, MPARAM, MPARAM);

void interface_thread (ULONG parm);
static int pmkey2asvkey (int k_virt, int k_scan, int k_char, int k_rep, int k_flags);
void fill_submenu (HWND h, struct _item *M);
void empty_submenu (HWND h, struct _item *M);
void change_displayed_size (int r, int c);

#define grab_video()    DosRequestMutexSem (mtx_Video, SEM_INDEFINITE_WAIT)
#define release_video() DosReleaseMutexSem (mtx_Video)

#define create_pm() {                                \
    hab0 = WinInitialize (0);                        \
    hmq0 = WinCreateMsgQueue (hab0, 0);              \
    DosGetInfoBlocks (&tib, &pib);                   \
    hsw = WinQuerySwitchHandle (0, pib->pib_ulpid);  \
    WinQuerySwitchEntry (hsw, &swdata);              \
    hwnd0 = swdata.hwnd;                             \
    }

#define destroy_pm() {               \
    WinDestroyMsgQueue (hmq0);       \
    WinTerminate (hab0);             \
    }

#define SHORTBASE             10000

#define WM_FLY_LOADMENU       WM_USER+1
#define WM_FLY_UNLOADMENU     WM_USER+2
#define WM_FLY_RESIZE         WM_USER+3
#define WM_FLY_MENU_CHSTATUS  WM_USER+4
#define WM_FLY_SET_FONT       WM_USER+5
#define WM_FLY_RESIZE_CANVAS  WM_USER+6
#define WM_FLY_MOVE_CANVAS    WM_USER+7
#define WM_FLY_MENU_CHSTATE   WM_USER+8

/* --------------------------------------------------------------
 small LIFO queue implementation
 ---------------------------------------------------------------- */
#define MSG_QUEUE_SIZE   8192
static int queue[MSG_QUEUE_SIZE], nq = 0;

int que_get (void);
int que_put (int msg);
int que_n (void);

/* --------------------------------------------------------------
 menuing system
 ---------------------------------------------------------------- */

void fly_init (int x0, int y0, int rows, int cols, char *font)
{
    TID       tid;
    int       rc;
    ULONG     reset;

    tiled1 = _tmalloc (tiled_size);
    if (tiled1 == NULL) fly_error ("cannot allocate tiled memory\n");
    us1_16 = _tmalloc (sizeof(USHORT));
    us2_16 = _tmalloc (sizeof(USHORT));
    pci = _tmalloc (sizeof(VIOCURSORINFO));

    rc = DosCreateMutexSem (NULL, &mtx_Queue, 0, FALSE);
    debug_tools ("rc = %d after DosCreateMutexSem\n", rc);

    rc = DosCreateMutexSem (NULL, &mtx_Video, 0, FALSE);
    debug_tools ("rc = %d after DosCreateMutexSem\n", rc);

    rc = DosCreateEventSem (NULL, &hev_NewMessage, 0, FALSE);
    debug_tools ("rc = %d after DosCreateEventSem\n", rc);

    rc = DosCreateEventSem (NULL, &hev_VideoReady, 0, FALSE);
    debug_tools ("rc = %d after DosCreateEventSem\n", rc);
    
    grab_video ();
    if (rows != -1 && cols != -1)
        video_init (rows, cols);
    else
        video_init (25, 80);
    release_video ();

    DosResetEventSem (hev_VideoReady, &reset);
    rc = DosCreateThread (&tid, interface_thread, 0, 0, 32786);
    debug_tools ("rc = %d after DosCreateThread\n", rc);

    DosWaitEventSem (hev_VideoReady, SEM_INDEFINITE_WAIT);
    fl_opt.initialized = TRUE;
    DosSleep (300);

    if (font != NULL)
        fly_set_font (font);

    //debug_tools ("rows = %d, cols = %d in fly_init\n", rows, cols);
    if (rows != -1 && cols != -1)
        video_set_window_size (rows, cols);
    
    debug_tools ("x0 = %d, y0 = %d in fly_init\n", x0, y0);
    if (x0 >= 0 && y0 >= 0)
        video_set_window_pos (x0, y0);
}

/* ------------------------------------------------------------- */

void fly_terminate (void)
{
    int  rc;
    
    WinPostMsg (hwndFrame, WM_QUIT, 0, 0);
    
    rc = DosCloseMutexSem (mtx_Queue);
    debug_tools ("rc = %d after DosCloseMutexSem1\n", rc);
    
    rc = DosCloseMutexSem (mtx_Video);
    
    debug_tools ("%d menu item changes\n", item_status_change);
    
    _tfree (tiled1);
    _tfree (us1_16);
    _tfree (us2_16);
    fl_opt.initialized = FALSE;
}

/* ------------------------------------------------------------- */

void screen_draw (char *s, int len, int row, int col, char *a)
{
    memcpy (tiled1, s, len);
    grab_video ();
    VioWrtCharStrAtt (tiled1, len, row, col, a, hvps);
    release_video ();
}

/* ------------------------------------------------------------- */

void video_update1 (int *forced)
{
}

/* ------------------------------------------------------------- */

void move_cursor (int row, int col)
{
    grab_video ();
    VioSetCurPos (row, col, hvps);
    release_video ();
}

/* ------------------------------------------------------------- */

char *get_clipboard (void)
{
    char   *clipdata = NULL;

    create_pm ();
    if (WinOpenClipbrd (hab0) == TRUE)
    {
        clipdata = (char *) WinQueryClipbrdData (hab0, CF_TEXT);
        if (clipdata != NULL)
        {
            // make a local copy
            clipdata = strdup (clipdata);
        }
    }
    
    WinCloseClipbrd (hab0);
    destroy_pm ();

    return clipdata;
}

/* ------------------------------------------------------------- */

void put_clipboard (char *s)
{
    int   len, rc;
    char  *mem;

    len = strlen (s);
    if (len == 0) return;

    create_pm ();
    if (WinOpenClipbrd (hab0) == TRUE)
    {
        WinEmptyClipbrd (hab0);
    
        rc = DosAllocSharedMem ((void **)&mem, NULL, len+1,
                                PAG_READ|PAG_WRITE|PAG_COMMIT|OBJ_GIVEABLE);
        if (rc == 0)
        {
            strcpy (mem, s);
            WinSetClipbrdData (hab0, (ULONG)mem, CF_TEXT, CFI_POINTER);
        }
    }
    
    WinCloseClipbrd (hab0);
    destroy_pm ();
}

/* ------------------------------------------------------------- */

void set_cursor (int flag)
{
    static int cursor_attrib = 0;
    
    VioGetCurType (pci, hvps);
    
    if (flag == 0)
    {
        if (pci->attr == 0xFFFF) return;
        cursor_attrib = pci->attr;
        pci->attr = 0xFFFF;
        VioSetCurType (pci, hvps);
    }
    else
    {
        if (pci->attr != 0xFFFF) return;
        pci->attr = cursor_attrib;
        VioSetCurType (pci, hvps);
    }
}

/* ------------------------------------------------------------- */

int getmessage (int interval)
{
    int             key = 0;
    ULONG           posted;

    if (que_n() > 0) return que_get ();
    
    // with interval == 0, we only check for keys in buffer
    if (interval == 0)
    {
        key = 0;
    }
    else if (interval > 0)
    {
        DosResetEventSem (hev_NewMessage, &posted);
        DosWaitEventSem (hev_NewMessage, interval);
        key = que_get ();
    }
    else if (interval < 0)
    {
        DosResetEventSem (hev_NewMessage, &posted);
        DosWaitEventSem (hev_NewMessage, SEM_INDEFINITE_WAIT);
        key = que_get ();
    }
    return key;
}

/* --------------------------------------------------------------------- */

int que_put (int msg)
{
    int    rc;
    
    DosRequestMutexSem (mtx_Queue, SEM_INDEFINITE_WAIT);
    if (nq == MSG_QUEUE_SIZE-1)
    {
        rc = -1;
    }
    else
    {
        queue[nq] = msg;
        nq++;
        rc = 0;
    }
    DosReleaseMutexSem (mtx_Queue);
    DosPostEventSem (hev_NewMessage);
    return rc;
}

/* --------------------------------------------------------------------- */

int que_get (void)
{
    int i, result;

    DosRequestMutexSem (mtx_Queue, SEM_INDEFINITE_WAIT);
    if (nq == 0)
    {
        result = 0;
    }
    else
    {
        result = queue[0];
        for (i=1; i<nq; i++)
            queue[i-1] = queue[i];
        nq--;
    }
    
    if (IS_SYSTEM (result))
    {
        switch (SYS_TYPE(result))
        {
        case SYSTEM_RESIZE:
            grab_video ();
            vio_cols = SYS_INT1 (result);
            vio_rows = SYS_INT2 (result);
            debug_tools ("changin' size to %d, %d\n", vio_rows, vio_cols);
            video_init (vio_rows, vio_cols);
            release_video ();
            break;

        case SYSTEM_DIE:
            exit (1); // ugh, this one is dirty...
            break;
        }
    }
    
    DosReleaseMutexSem (mtx_Queue);
    return result;
}

/* --------------------------------------------------------------------- */

int que_n (void)
{
    int result;

    DosRequestMutexSem (mtx_Queue, SEM_INDEFINITE_WAIT);
    result = nq;
    DosReleaseMutexSem (mtx_Queue);
    return result;
}

/* ------------------------------------------------------------- */

#define KB_SHIFT   0x00010000
#define KB_ALT     0x00020000
#define KB_CTRL    0x00040000

struct _transkey
{
    int asv_key;
    int vk_key;
};

struct _transkey TransTable2[] = {
    {_CtrlA, KB_CTRL|'a'},    {_CtrlB, KB_CTRL|'b'},    {_CtrlC, KB_CTRL|'c'},    {_CtrlD, KB_CTRL|'d'},
    {_CtrlE, KB_CTRL|'e'},    {_CtrlF, KB_CTRL|'f'},    {_CtrlG, KB_CTRL|'g'},    {_CtrlH, KB_CTRL|'h'},
    {_CtrlI, KB_CTRL|'i'},    {_CtrlJ, KB_CTRL|'j'},    {_CtrlK, KB_CTRL|'k'},    {_CtrlL, KB_CTRL|'l'},
    {_CtrlM, KB_CTRL|'m'},    {_CtrlN, KB_CTRL|'n'},    {_CtrlO, KB_CTRL|'o'},    {_CtrlP, KB_CTRL|'p'},
    {_CtrlQ, KB_CTRL|'q'},    {_CtrlR, KB_CTRL|'r'},    {_CtrlS, KB_CTRL|'s'},    {_CtrlT, KB_CTRL|'t'},
    {_CtrlU, KB_CTRL|'u'},    {_CtrlV, KB_CTRL|'v'},    {_CtrlW, KB_CTRL|'w'},    {_CtrlX, KB_CTRL|'x'},
    {_CtrlY, KB_CTRL|'y'},    {_CtrlZ, KB_CTRL|'z'},
    
    {_AltA, KB_ALT|'a'},    {_AltB, KB_ALT|'b'},    {_AltC, KB_ALT|'c'},    {_AltD, KB_ALT|'d'},
    {_AltE, KB_ALT|'e'},    {_AltF, KB_ALT|'f'},    {_AltG, KB_ALT|'g'},    {_AltH, KB_ALT|'h'},
    {_AltI, KB_ALT|'i'},    {_AltJ, KB_ALT|'j'},    {_AltK, KB_ALT|'k'},    {_AltL, KB_ALT|'l'},
    {_AltM, KB_ALT|'m'},    {_AltN, KB_ALT|'n'},    {_AltO, KB_ALT|'o'},    {_AltP, KB_ALT|'p'},
    {_AltQ, KB_ALT|'q'},    {_AltR, KB_ALT|'r'},    {_AltS, KB_ALT|'s'},    {_AltT, KB_ALT|'t'},
    {_AltU, KB_ALT|'u'},    {_AltV, KB_ALT|'v'},    {_AltW, KB_ALT|'w'},    {_AltX, KB_ALT|'x'},
    {_AltY, KB_ALT|'y'},    {_AltZ, KB_ALT|'z'},
    
    {_Alt1, KB_ALT|'1'},    {_Alt2, KB_ALT|'2'},    {_Alt3, KB_ALT|'3'},    {_Alt4, KB_ALT|'4'},
    {_Alt5, KB_ALT|'5'},    {_Alt6, KB_ALT|'6'},    {_Alt7, KB_ALT|'7'},    {_Alt8, KB_ALT|'8'},
    {_Alt9, KB_ALT|'9'},    {_Alt0, KB_ALT|'0'},

    {_CtrlTab, KB_CTRL|VK_TAB},
    {_CtrlBackSlash, KB_CTRL|'\\'},
    
    {_CtrlBackSpace, KB_CTRL|VK_BACKSPACE},
    {_AltBackSpace, KB_ALT|VK_BACKSPACE},

    {_CtrlLeftRectBrac, KB_CTRL|'['},
    {_CtrlRightRectBrac, KB_CTRL|']'},
    {_AltLeftRectBrac, KB_ALT|'['},
    {_AltRightRectBrac, KB_ALT|']'},

    {_AltSemicolon, KB_ALT|';'},
    {_AltApostrophe, KB_ALT|'\''},
    {_AltBackApostrophe, KB_ALT|'`'},
    {_AltBackSlash, KB_ALT|'\\'},
    {_AltComma, KB_ALT|','},
    {_AltPeriod, KB_ALT|'.'},
    
    /* numeric keypad */
    {_CtrlNumSlash, KB_CTRL|'/'},
    {_AltSlash, KB_ALT|'/'},
    {_AltNumSlash, KB_ALT|'/'},
    
    {_CtrlNumAsterisk, KB_CTRL|'*'},
    {_AltNumAsterisk, KB_ALT|'*'},
    
    {_CtrlMinus, KB_CTRL|'-'},
    {_AltMinus, KB_ALT|'-'},
    {_AltNumMinus, KB_ALT|'-'},
    
    {_CtrlNumPlus, KB_CTRL|'+'},
    {_AltPlus, KB_ALT|'='},
    {_AltNumPlus, KB_ALT|'+'},
    
    {_Ctrl2, KB_CTRL|'2'},    {_Ctrl6, KB_CTRL|'6'},
};

struct _transkey TransTable1[] = {

    {_Esc, VK_ESC},
    
    {_F1, VK_F1},    {_F2, VK_F2},    {_F3, VK_F3},    {_F4, VK_F4},    {_F5, VK_F5},
    {_F6, VK_F6},    {_F7, VK_F7},    {_F8, VK_F8},    {_F9, VK_F9},    {_F10, VK_F10},
    {_F11, VK_F11},  {_F12, VK_F12},
    
    {_ShiftF1, KB_SHIFT|VK_F1},    {_ShiftF2, KB_SHIFT|VK_F2},    {_ShiftF3, KB_SHIFT|VK_F3},
    {_ShiftF4, KB_SHIFT|VK_F4},    {_ShiftF5, KB_SHIFT|VK_F5},    {_ShiftF6, KB_SHIFT|VK_F6},
    {_ShiftF7, KB_SHIFT|VK_F7},    {_ShiftF8, KB_SHIFT|VK_F8},    {_ShiftF9, KB_SHIFT|VK_F9},
    {_ShiftF10, KB_SHIFT|VK_F10},  {_ShiftF11, KB_SHIFT|VK_F11},  {_ShiftF12, KB_SHIFT|VK_F12},
    
    {_CtrlF1, KB_CTRL|VK_F1},    {_CtrlF2, KB_CTRL|VK_F2},    {_CtrlF3, KB_CTRL|VK_F3},
    {_CtrlF4, KB_CTRL|VK_F4},    {_CtrlF5, KB_CTRL|VK_F5},    {_CtrlF6, KB_CTRL|VK_F6},
    {_CtrlF7, KB_CTRL|VK_F7},    {_CtrlF8, KB_CTRL|VK_F8},    {_CtrlF9, KB_CTRL|VK_F9},
    {_CtrlF10, KB_CTRL|VK_F10},  {_CtrlF11, KB_CTRL|VK_F11},  {_CtrlF12, KB_CTRL|VK_F12},
    
    {_AltF1, KB_ALT|VK_F1},    {_AltF2, KB_ALT|VK_F2},    {_AltF3, KB_ALT|VK_F3},
    {_AltF4, KB_ALT|VK_F4},    {_AltF5, KB_ALT|VK_F5},    {_AltF6, KB_ALT|VK_F6},
    {_AltF7, KB_ALT|VK_F7},    {_AltF8, KB_ALT|VK_F8},    {_AltF9, KB_ALT|VK_F9},
    {_AltF10, KB_ALT|VK_F10},  {_AltF11, KB_ALT|VK_F11},  {_AltF12, KB_ALT|VK_F12},
    
    {_Home, VK_HOME},  {_CtrlHome, KB_CTRL|VK_HOME},  {_AltHome, KB_ALT|VK_HOME},
    {_End, VK_END},    {_CtrlEnd, KB_CTRL|VK_END},    {_AltEnd, KB_ALT|VK_END},
    
    {_Up, VK_UP},    {_CtrlUp, KB_CTRL|VK_UP},    {_AltUp, KB_ALT|VK_UP},
    {_Down, VK_DOWN},    {_CtrlDown, KB_CTRL|VK_DOWN},    {_AltDown, KB_ALT|VK_DOWN},

    {_Left, VK_LEFT},    {_CtrlLeft, KB_CTRL|VK_LEFT},    {_AltLeft, KB_ALT|VK_LEFT},
    {_Right, VK_RIGHT},  {_CtrlRight, KB_CTRL|VK_RIGHT},  {_AltRight, KB_ALT|VK_RIGHT},
    
    {_PgUp, VK_PAGEUP},    {_CtrlPgUp, KB_CTRL|VK_PAGEUP},    {_AltPageUp, KB_ALT|VK_PAGEUP},
    {_PgDn, VK_PAGEDOWN},  {_CtrlPgDn, KB_CTRL|VK_PAGEDOWN},  {_AltPageDn, KB_ALT|VK_PAGEDOWN},
    
    {_Gold, VK_CLEAR},    {_CtrlGold, KB_CTRL|VK_CLEAR},
    
    {_Insert, VK_INSERT}, {_CtrlInsert, KB_CTRL|VK_INSERT},   {_AltInsert, KB_ALT|VK_INSERT},
    {_Delete, VK_DELETE}, {_CtrlDelete, KB_CTRL|VK_DELETE},   {_AltDelete, KB_ALT|VK_DELETE},

    {_CtrlNumIns, KB_CTRL|VK_INSERT},
    {_CtrlNumDel, KB_CTRL|VK_DELETE}, 
    {_AltTab, KB_ALT|VK_TAB},
    {_CtrlEnter, KB_CTRL|VK_ENTER},

    {_ShiftInsert, KB_SHIFT|VK_INSERT}
};

static int pmkey2asvkey (int k_virt, int k_scan, int k_char, int k_rep, int k_flags)
{
    int i, key = -1, flags = 0;

    key = -1;

    // ignore key up
    if (k_flags & KC_KEYUP) return key;

    // find the shifts state
    if (k_flags & KC_ALT) flags |= KB_ALT;
    if (k_flags & KC_CTRL) flags |= KB_CTRL;
    if (k_flags & KC_SHIFT) flags |= KB_SHIFT;
    
    // very special cases
    if (k_char == 9 && flags == KB_SHIFT) return _ShiftTab;

    if (k_flags & KC_CHAR)
    {
        return (unsigned char) k_char;
    }

    // process virtual keys
    if (k_flags & KC_VIRTUALKEY)
    {
        for (i = 0; i < sizeof(TransTable1)/sizeof(struct _transkey); i++)
            if (TransTable1[i].vk_key == (flags|k_virt))
            {
                key = TransTable1[i].asv_key;
                break;
            }
        return key;
    }

    // check for Ctrl-something and Alt-something
    if (flags & (KB_ALT|KB_CTRL))
    {
        for (i = 0; i < sizeof(TransTable2)/sizeof(struct _transkey); i++)
            if (TransTable2[i].vk_key == (k_char|flags))
            {
                key = TransTable2[i].asv_key;
                break;
            }
        return key;
    }
    
    return key;
}

/* ------------------------------------------------------------- */

void set_window_name (char *format, ...)
{
    va_list  args;
    char     buffer[8192];
    
    /* set up new title */
    va_start (args, format);
    vsnprintf1 (buffer, sizeof(buffer), format, args);
    va_end (args);
    buffer[MAXNAMEL] = '\0';

    // change title
    create_pm ();
    WinSetWindowText (hwnd0, buffer);
    strncpy (swdata.szSwtitle, buffer, MAXNAMEL);
    swdata.szSwtitle[MAXNAMEL] = '\0';
    WinChangeSwitchEntry (hsw, &swdata);
    destroy_pm ();
}

/* ------------------------------------------------------------- */

char *get_window_name (void)
{
    char     buffer[1024];
    int      n;

    buffer[0] = '\0';
    create_pm ();
    n = WinQueryWindowText (hwnd0, sizeof (buffer), buffer);
    destroy_pm ();
    
    if (n <= 0 || buffer[0] == '\0') return strdup (" ");
    return strdup (buffer);
}

/* ------------------------------------------------------------- */

void interface_thread (ULONG parm)
{
    static CHAR  szClientClass [] = "FlyWindow";
    static ULONG flFrameFlags =
        FCF_TITLEBAR | FCF_SYSMENU | FCF_SIZEBORDER | FCF_MINMAX |
        FCF_SHELLPOSITION | FCF_TASKLIST | FCF_MENU;
    int          rc;

    hab = WinInitialize (0);
    hmq = WinCreateMsgQueue (hab, 0);
    rc = WinRegisterClass (hab, szClientClass, FlyWndProc, CS_SIZEREDRAW, 0);
    hwndFrame = WinCreateStdWindow (HWND_DESKTOP, WS_VISIBLE,
                                    &flFrameFlags, szClientClass, NULL,
                                    0L, 0, FLY_APP_FRAMEWND, &hwndClient);
    
    while (WinGetMsg (hab, &qmsg, 0, 0, 0))
        WinDispatchMsg (hab, &qmsg);
    
    WinDestroyWindow (hwndFrame);
    WinDestroyMsgQueue (hmq);
    WinTerminate (hab);

    // post semaphore that is has died
}

/* ------------------------------------------------------------- */
  
MRESULT EXPENTRY FlyWndProc (HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
    SIZEL        sizl;
    int          rc, key, mou_r, mou_c, mou_ev, new_vio_rows, new_vio_cols;
    int          deltaX, deltaY, pix_rows, pix_cols, new_x0, new_y0;
    static int   mou_c1=-1, mou_r1=-1;
    struct _item *L;
    HWND         hwndMenu;
    SWP          swp;
    QMSG         *qmsg;
    USHORT       vk, fl;
    
    switch (msg)
    {
    case WM_CREATE:
        hdc = WinOpenWindowDC (hwnd);
        sizl.cx = sizl.cy = 0;
        grab_video ();
        hps = GpiCreatePS (hab, hdc, &sizl, PU_PELS | GPIF_DEFAULT |
                           GPIT_MICRO | GPIA_ASSOC);
        rc = VioCreatePS (&hvps, 80, 25, 0, 1, 0);
        VioGetDeviceCellSize (&cyChar, &cxChar, hvps);
        set_cursor (0);
        VioSetCurType (pci, hvps);
        release_video ();
        DosPostEventSem (hev_VideoReady);
        return 0;
        
    case WM_MOUSEMOVE:          mou_ev = MOUEV_MOVE; goto MOUSE;
    
    case WM_BUTTON1CLICK:       mou_ev = MOUEV_B1SC; goto MOUSE;
    case WM_BUTTON1DBLCLK:      mou_ev = MOUEV_B1DC; goto MOUSE;
    case WM_BUTTON1MOTIONSTART: mou_ev = MOUEV_B1MS; goto MOUSE;
    case WM_BUTTON1MOTIONEND:   mou_ev = MOUEV_B1ME; goto MOUSE;
    case WM_BUTTON1DOWN:        mou_ev = MOUEV_B1DN; goto MOUSE;
    case WM_BUTTON1UP:          mou_ev = MOUEV_B1UP; goto MOUSE;
    
    case WM_BUTTON2CLICK:       mou_ev = MOUEV_B2SC; goto MOUSE;
    case WM_BUTTON2DBLCLK:      mou_ev = MOUEV_B2DC; goto MOUSE;
    case WM_BUTTON2MOTIONSTART: mou_ev = MOUEV_B2MS; goto MOUSE;
    case WM_BUTTON2MOTIONEND:   mou_ev = MOUEV_B2ME; goto MOUSE;
    case WM_BUTTON2DOWN:        mou_ev = MOUEV_B2DN; goto MOUSE;
    case WM_BUTTON2UP:          mou_ev = MOUEV_B2UP; goto MOUSE;
    
    case WM_BUTTON3DBLCLK:      mou_ev = MOUEV_B3DC; goto MOUSE;
    case WM_BUTTON3CLICK:       mou_ev = MOUEV_B3SC; goto MOUSE;
    case WM_BUTTON3MOTIONSTART: mou_ev = MOUEV_B3MS; goto MOUSE;
    case WM_BUTTON3MOTIONEND:   mou_ev = MOUEV_B3ME; goto MOUSE;
    case WM_BUTTON3DOWN:        mou_ev = MOUEV_B3DN; goto MOUSE;
    case WM_BUTTON3UP:          mou_ev = MOUEV_B3UP; goto MOUSE;
    
    MOUSE:
        if (fl_opt.mouse_active != TRUE) break;
        mou_r = vio_rows - 1 - (SHORT2FROMMP (mp1)/cyChar);
        mou_c = SHORT1FROMMP (mp1)/cxChar;
        if (mou_r < 0 || mou_c < 0) break;
        // prevent MOUEV_MOVE message with same coordinates
        if (mou_ev == MOUEV_MOVE && mou_r == mou_r1 && mou_c == mou_c1) break;
        mou_r1 = mou_r, mou_c1 = mou_c;
        que_put (FMSG_BASE_MOUSE + FMSG_BASE_MOUSE_EVTYPE*mou_ev +
                 FMSG_BASE_MOUSE_X*mou_c + FMSG_BASE_MOUSE_Y*mou_r);
        break;
        
    case WM_PAINT:
        WinBeginPaint (hwnd, hps, NULL);
        grab_video ();
        VioShowBuf (0, 2 * vio_rows * vio_cols, hvps);
        release_video ();
        WinEndPaint (hps);
        return 0;

    case WM_CHAR:
        if (SHORT1FROMMP (mp1) & KC_KEYUP) return 0;
        if (SHORT2FROMMP (mp2) == VK_SHIFT ||
            SHORT2FROMMP (mp2) == VK_CTRL ||
            SHORT2FROMMP (mp2) == VK_ALT) return 0;
        key = pmkey2asvkey (SHORT2FROMMP(mp2), CHAR4FROMMP(mp1), SHORT1FROMMP(mp2),
                            CHAR3FROMMP(mp1), SHORT1FROMMP(mp1));
        if (key != -1) que_put (key);
        return 0;

    case WM_TRANSLATEACCEL:
        qmsg = (QMSG *)mp1;
        vk = SHORT2FROMMP (qmsg->mp2);
        fl = SHORT1FROMMP (qmsg->mp1) & (KC_ALT | KC_SHIFT | KC_CTRL | KC_KEYUP);
        if (vk == VK_MENU || vk == VK_F1) return FALSE;
        //if ((fl & KC_ALT) && vk >= VK_F1 && vk <= VK_F24) return FALSE;
        break;
        
    case WM_CLOSE:
        que_put (FMSG_BASE_SYSTEM +
                 FMSG_BASE_SYSTEM_TYPE*SYSTEM_QUIT);
        return 0;
        
    case WM_SIZE:
        if (cxChar != 0 && cyChar != 0)
        {
            pix_rows = SHORT2FROMMP (mp2);
            pix_cols = SHORT1FROMMP (mp2);
            new_vio_rows = pix_rows / cyChar;
            new_vio_cols = pix_cols / cxChar;
            if (new_vio_rows != vio_rows || new_vio_cols != vio_cols)
            {
                grab_video ();
                VioAssociate (0, hvps);
                VioDestroyPS (hvps);
                rc = VioCreatePS (&hvps, new_vio_rows, new_vio_cols, 0, 1, 0);
                VioSetDeviceCellSize (cyChar, cxChar, hvps);
                VioGetDeviceCellSize (&cyChar, &cxChar, hvps);
                rc = VioAssociate (hdc, hvps);
                VioSetCurType (pci, hvps);
                release_video ();
                que_put (FMSG_BASE_SYSTEM + FMSG_BASE_SYSTEM_TYPE*SYSTEM_RESIZE +
                         FMSG_BASE_SYSTEM_INT2*new_vio_rows + FMSG_BASE_SYSTEM_INT1*new_vio_cols);
            }
            deltaX = new_vio_cols*cxChar - pix_cols;
            deltaY = new_vio_rows*cyChar - pix_rows;
            //if (deltaX != 0 || deltaY != 0)
            if (abs(deltaX) > MAX_DELTA || abs(deltaY) > MAX_DELTA)
            {
                WinPostMsg (hwndFrame, WM_FLY_RESIZE,
                            MPFROM2SHORT (SHORTBASE+deltaX, SHORTBASE+deltaY), NULL);
            }
        }
        WinDefAVioWindowProc (hwnd, msg, (ULONG)mp1, (ULONG)mp2);
        return 0;

    case WM_COMMAND:
        que_put (FMSG_BASE_MENU + LOUSHORT (mp1));
        break;

    case WM_FLY_LOADMENU:
        L = PVOIDFROMMP (mp1);
        // obtain handle for window menu
        hwndMenu = WinWindowFromID (WinQueryWindow (hwnd, QW_PARENT), FID_MENU);
        fill_submenu (hwndMenu, L);
        fly_active_menu = L;
        break;

    case WM_FLY_UNLOADMENU:
        L = PVOIDFROMMP (mp1);
        // obtain handle for window menu
        hwndMenu = WinWindowFromID (WinQueryWindow (hwnd, QW_PARENT), FID_MENU);
        empty_submenu (hwndMenu, L);
        fly_active_menu = NULL;
        break;
        
    case WM_FLY_RESIZE:
        deltaX = SHORT1FROMMP (mp1) - SHORTBASE;
        deltaY = SHORT2FROMMP (mp1) - SHORTBASE;
        rc = WinQueryWindowPos (hwndFrame, &swp);
        rc = WinSetWindowPos (hwndFrame, 0, swp.x, swp.y-deltaY, swp.cx+deltaX, swp.cy+deltaY, SWP_SIZE|SWP_MOVE);
        break;
        
    case WM_FLY_MOVE_CANVAS:
        new_x0 = SHORT1FROMMP (mp1) - SHORTBASE;
        new_y0 = SHORT2FROMMP (mp1) - SHORTBASE;
        rc = WinQueryWindowPos (hwndFrame, &swp);
        WinSetWindowPos (hwndFrame, 0, new_x0, new_y0-swp.cy, 0, 0, SWP_MOVE);
        //DosPostEventSem (hev_VideoReady);
        break;
        
    case WM_FLY_MENU_CHSTATUS:
        hwndMenu = WinWindowFromID (WinQueryWindow (hwnd, QW_PARENT), FID_MENU);
        WinEnableMenuItem (hwndMenu, SHORT1FROMMP(mp1), SHORT2FROMMP(mp1));
        item_status_change++;
        break;
        
    case WM_FLY_MENU_CHSTATE:
        hwndMenu = WinWindowFromID (WinQueryWindow (hwnd, QW_PARENT), FID_MENU);
        WinSendMsg (hwndMenu, MM_SETITEMATTR, MPFROM2SHORT (SHORT1FROMMP(mp1), TRUE),
                    MPFROM2SHORT (MIA_CHECKED, SHORT2FROMMP(mp1) ? MIA_CHECKED : 0));
        break;
        
    case WM_DESTROY:
        grab_video ();
        VioAssociate (0, hvps);
        VioDestroyPS (hvps);
        GpiDestroyPS (hps);
        release_video ();
        que_put (FMSG_BASE_SYSTEM +
                 FMSG_BASE_SYSTEM_TYPE*SYSTEM_DIE);
        return 0;
    }
    
    return WinDefWindowProc (hwnd, msg, mp1, mp2);
}

/* ------------------------------------------------------------- */

void fly_mouse (int enabled)
{
    fl_opt.mouse_active = enabled;
}

/* -------------------------------------------------------------  */

void ifly_error_gui (char *message)
{
    create_pm ();
    WinMessageBox (HWND_DESKTOP, HWND_DESKTOP, message, "Error", 0, MB_INFORMATION|MB_OK);
    destroy_pm ();
}

/* ------------------------------------------------------------- */

void fly_launch (char *command, int wait, int pause)
{
    char    failbuf[512], *cmd, *p, que_name[64], *w;
    int     rc;
    PID     pid;
    ULONG   sess_id, datalen;
    void    *data;
    
    STARTDATA    sdata;
    HQUEUE       hq;
    REQUESTDATA  rd;
    BYTE         prty;

    if (wait)
    {
        debug_tools ("entered fly_launch(%s)\n", command);
        w = get_window_name ();
        set_window_name (command);
        snprintf1 (que_name, sizeof(que_name), "\\QUEUES\\FLY\\%u\\LAUNCH", getpid ());
        rc = DosCreateQueue (&hq, QUE_FIFO, que_name);
        debug_tools ("rc = %d after DosCreateQueue\n", rc);
        if (rc != 0)
        {
            set_window_name (w);
            return;
        }
        
        cmd = strdup (command);
        p = strchr (cmd, ' ');
        if (p != NULL) *p = '\0';

        sdata.Length = sizeof (STARTDATA);
        sdata.Related = SSF_RELATED_CHILD;
        sdata.FgBg = SSF_FGBG_FORE;
        sdata.TraceOpt = SSF_TRACEOPT_NONE;
        sdata.PgmTitle = NULL;
        sdata.PgmName = cmd;
        sdata.PgmInputs = (p == NULL) ? NULL : p+1;
        sdata.TermQ = que_name;
        sdata.Environment = NULL;
        sdata.InheritOpt = SSF_INHERTOPT_SHELL;
        sdata.SessionType = SSF_TYPE_DEFAULT;
        sdata.IconFile = NULL;
        sdata.PgmHandle = 0;
        sdata.PgmControl = 0;
        sdata.InitXPos = 0;
        sdata.InitYPos = 0;
        sdata.InitXSize = 100;
        sdata.InitYSize = 100;
        sdata.Reserved = 0;
        sdata.ObjectBuffer = failbuf;
        sdata.ObjectBuffLen = sizeof (failbuf);

        debug_tools ("going for DosStartSession()\n");
        rc = DosStartSession (&sdata, &sess_id, &pid);
        
        debug_tools ("rc = %d, failbuf: [%s]\n", rc, failbuf);

        if (rc == 0)
        {
            datalen = sizeof (rd);
            prty = 0;
            DosReadQueue (hq, &rd, &datalen, &data, 0, DCWW_WAIT, &prty, 0);
        }
        DosCloseQueue (hq);
        
        free (cmd);
        set_window_name (w);
    }
    else
    {
        cmd = malloc (strlen (command)+32);
        snprintf1 (cmd, strlen (command)+32, "detach %s >nul 2>&1", command);
        str_translate (cmd, '/', '\\');
        debug_tools ("detaching [%s]\n", cmd);
        system (cmd);
        free (cmd);
    }
}

/* ------------------------------------------------------------- */

#define MAX_ACT 512

void menu_chstatus (int actions[], int n, int newstatus)
{
    int         i;
    static int  cached[MAX_ACT], now_cached = 0;

    if (!menu_enabled) return;
    
    if (now_cached == 0)
    {
        for (i=0; i<MAX_ACT; i++)
            cached[i] = -1;
        now_cached = 1;
    }
    
    for (i=0; i<n; i++)
    {
        if (actions[i] < MAX_ACT && cached[actions[i]] == newstatus) continue;
        WinPostMsg (hwndFrame, WM_FLY_MENU_CHSTATUS,
                    MPFROM2SHORT (actions[i], newstatus), NULL);
        if (actions[i] < MAX_ACT) cached[actions[i]] = newstatus;
    }
}

/* ------------------------------------------------------------- */

void menu_activate (struct _item *L)
{
    int   rc;
    
    rc = WinPostMsg (hwndFrame, WM_FLY_LOADMENU, MPFROMP(L), NULL);
    debug_tools ("rc = %d from WinPostMsg in menu_activate; error = %d\n",
                 rc, WinGetLastError (hab));
}

/* ------------------------------------------------------------- */

void menu_deactivate (struct _item *L)
{
    int   rc;
    
    rc = WinPostMsg (hwndFrame, WM_FLY_UNLOADMENU, MPFROMP(L), NULL);
    debug_tools ("rc = %d from WinPostMsg in menu_deactivate; error = %d\n",
                 rc, WinGetLastError (hab));
}

/* ------------------------------------------------------------- */

int menu_process_line (struct _item *L, int init, int opendrop)
{
    return MENU_RC_CANCELLED;
}

/* ------------------------------------------------------------- */

void fill_submenu (HWND h, struct _item *M)
{
    int       ni, i;
    HWND      hwndMenu;
    char      *p;
    MENUITEM  mi;
    
    // count submenus
    ni = 0;
    while (M[ni].type != ITEM_TYPE_END) ni++;
    
    // fill every submenu
    for (i=0; i<ni; i++)
    {
        switch (M[i].type)
        {
        case ITEM_TYPE_ACTION:
        case ITEM_TYPE_SWITCH:
            mi.iPosition = MIT_END;
            mi.afStyle = MIS_TEXT;
            mi.afAttribute = 0;
            mi.hwndSubMenu = 0;
            mi.hItem = 0;
            mi.id = M[i].body.action;
            p = str_strdup1 (M[i].text, 64);
            if (M[i].hotkey != 0)
                str_insert (p, '~', M[i].hkpos);
            if (M[i].keydesc[0] != '\0')
            {
                strcat (p, "\t");
                strcat (p, M[i].keydesc);
            }
            WinSendMsg (h, MM_INSERTITEM, MPFROMP (&mi), p);
            free (p);
            break;
            
        case ITEM_TYPE_SUBMENU:
            hwndMenu = WinCreateMenu (HWND_OBJECT, NULL);
            WinSetWindowUShort (hwndMenu, QWS_ID, i);
            M[i].submenu_id = i;
            fill_submenu (hwndMenu, M[i].body.submenu);
            mi.iPosition = MIT_END;
            mi.afStyle = MIS_TEXT | MIS_SUBMENU;
            mi.afAttribute = 0;
            mi.hwndSubMenu = hwndMenu;
            mi.id = i;
            p = str_strdup1 (M[i].text, 64);
            if (M[i].hotkey != 0)
                str_insert (p, '~', M[i].hkpos);
            if (M[i].keydesc[0] != '\0')
            {
                strcat (p, "\t");
                strcat (p, M[i].keydesc);
            }
            WinSendMsg (h, MM_INSERTITEM, MPFROMP (&mi), p);
            free (p);
            break;
            
        case ITEM_TYPE_SEP:
            mi.iPosition = MIT_END;
            mi.afStyle = MIS_SEPARATOR;
            mi.afAttribute = 0;
            mi.hwndSubMenu = 0;
            mi.hItem = 0;
            mi.id = 0;
            WinSendMsg (h, MM_INSERTITEM, MPFROMP (&mi), NULL);
            break;
        }
    }
}

/* ------------------------------------------------------------- */

void empty_submenu (HWND h, struct _item *M)
{
    int       ni, i;
    
    // count submenus
    ni = 0;
    while (M[ni].type != ITEM_TYPE_END) ni++;
    
    // empty every submenu
    for (i=0; i<ni; i++)
    {
        switch (M[i].type)
        {
        case ITEM_TYPE_ACTION:
        case ITEM_TYPE_SWITCH:
        case ITEM_TYPE_SEP:
            WinSendMsg (h, MM_DELETEITEM, MPFROM2SHORT (M[i].body.action, 1), NULL);
            break;
            
        case ITEM_TYPE_SUBMENU:
            empty_submenu (h/* ? */, M[i].body.submenu);
            WinSendMsg (h, MM_DELETEITEM, MPFROM2SHORT (M[i].submenu_id, 1), NULL);
            break;
        }
    }
}

/* --------------------------------------------------------------------- */

void menu_enable (void)
{
    int items[64], ni;

    if (fly_active_menu == NULL) return;
    ni = 0;
    while (fly_active_menu[ni].type != ITEM_TYPE_END)
    {
        items[ni] = fly_active_menu[ni].submenu_id;
        ni++;
    }

    menu_enabled = TRUE;
    menu_chstatus (items, ni, 1);
}

/* --------------------------------------------------------------------- */

void menu_disable (void)
{
    int items[64], ni;
    
    if (fly_active_menu == NULL) return;
    ni = 0;
    while (fly_active_menu[ni].type != ITEM_TYPE_END)
    {
        debug_tools ("disabling %s (%d)\n",
                     fly_active_menu[ni].text, fly_active_menu[ni].submenu_id);
        items[ni] = fly_active_menu[ni].submenu_id;
        ni++;
    }

    menu_chstatus (items, ni, 0);
    menu_enabled = FALSE;
}

/* ------------------------------------------------------------- */

int  menu_check_key (struct _item *L, int key)
{
    return 0;
}

/* -------------------------------------------------------------
returns: 0 - esc hit; 1...n - choice is selected
*/

int ifly_ask_gui (int flags, char *questions, char *answers)
{
    char          *p, *p1;
    int           i, choice, numchoices;
    int           ch_ind[16], ch_len[16];
    MB2INFO       *pmbInfo;
    ULONG         uls;

    // parse 'answers' string
    p = answers; numchoices = 0; p1 = p;
    while (1)
    {
        if (*p=='\n' || *p==0)
        {
            ch_ind[numchoices] = p1 - answers;
            ch_len[numchoices] = p - p1;
            p1 = p + 1;
            numchoices++;
        }
        if (*p++ == 0) break;
    }

    uls = sizeof(MB2INFO) + sizeof(MB2D) * (numchoices-1);
    pmbInfo = malloc (uls);

    pmbInfo->cb         = uls;
    pmbInfo->hIcon      = NULLHANDLE;
    pmbInfo->cButtons   = numchoices;
    pmbInfo->flStyle    = MB_APPLMODAL | MB_NOICON | MB_MOVEABLE;
    pmbInfo->hwndNotify = NULLHANDLE;

    /* Set information for each button in the MB2INFO structure */
    for (i=0; i<numchoices; i++)
    {
        str_scopy (pmbInfo->mb2d[i].achText, answers+ch_ind[i]);
        pmbInfo->mb2d[i].achText[ch_len[i]] = '\0';
        pmbInfo->mb2d[i].idButton = 10000+i;
        pmbInfo->mb2d[i].flStyle = BS_PUSHBUTTON;
    }
    pmbInfo->mb2d[0].flStyle |= BS_DEFAULT;

    create_pm ();
    //choice = WinMessageBox2 (hwndFrame, hwndFrame, questions, fl_opt.appname, 1234L, pmbInfo);
    choice = WinMessageBox2 (HWND_DESKTOP, hwndFrame, questions, fl_opt.appname, 1234L, pmbInfo);
    destroy_pm ();
    free (pmbInfo);
    
    if (choice == MBID_ERROR) choice = 0;
    else                      choice = choice-10000+1;
    if ((flags & ASK_YN) && choice == numchoices) choice = 0;
    return choice;
}

/* ------------------------------------------------------------- */

void video_set_window_pos (int x0, int y0)
{
    //ULONG    reset;

    //DosResetEventSem (hev_VideoReady, &reset);
    WinPostMsg (hwndFrame, WM_FLY_MOVE_CANVAS,
                MPFROM2SHORT (SHORTBASE+x0, SHORTBASE+y0), NULL);
    //DosWaitEventSem (hev_VideoReady, SEM_INDEFINITE_WAIT);

    //video_get_window_pos (&x0, &y0);
    //debug_tools ("result: x = %d, y = %d\n", x0, y0);
}

/* ------------------------------------------------------------- */

int video_get_window_pos (int *x0, int *y0)
{
    SWP   swp;
    int   rc;
    
    create_pm ();
    rc = WinQueryWindowPos (hwnd0, &swp);
    destroy_pm ();
    if (!rc) return 1;
    debug_tools ("video_get_window_pos: x,y = %d %d;  cx,cy = %d %d\n",
                 swp.x, swp.y, swp.cx, swp.cy);
    
    *x0 = swp.x;
    *y0 = swp.y+swp.cy;
    return 0;
}

/* ------------------------------------------------------------- */

int video_set_window_size (int rows, int cols)
{
    int deltaX, deltaY;
    //ULONG    reset;

    //return 0;
    
    deltaX = (cols - video_hsize()) * cxChar;
    deltaY = (rows - video_vsize()) * cyChar;
    debug_tools ("video_set_window_size (%d -> %d, %d -> %d): dx = %d, dy = %d\n",
                 video_hsize(), cols, video_vsize(), rows, deltaX, deltaY);
    if (deltaX != 0  || deltaY != 0)
    {
        //DosResetEventSem (hev_VideoReady, &reset);
        WinPostMsg (hwndFrame, WM_FLY_RESIZE, MPFROM2SHORT (SHORTBASE+deltaX, SHORTBASE+deltaY), NULL);
        //DosWaitEventSem (hev_VideoReady, SEM_INDEFINITE_WAIT);
        //debug_tools ("result: x = %d, y = %d\n", video_hsize(), video_vsize());
    }

    return 0;
}

/* ------------------------------------------------------------- */

char *fly_get_font (void)
{
    char buffer[64];
    
    snprintf1 (buffer, sizeof(buffer), "%s %dx%d", "System VIO", cxChar, cyChar);
    return strdup (buffer);
}

/* ------------------------------------------------------------- */

void change_displayed_size (int r, int c)
{
    int rc;
    
    grab_video ();
    
    video_init (r, c);
    
    VioAssociate (0, hvps);
    VioDestroyPS (hvps);
    
    rc = VioCreatePS (&hvps, r, c, 0, 1, 0);
    rc = VioAssociate (hdc, hvps);
    rc = VioSetDeviceCellSize (cyChar, cxChar, hvps);
    VioSetCurType (pci, hvps);
    
    vio_rows = r;
    vio_cols = c;
    
    release_video ();
}

/* ------------------------------------------------------------- */

int fly_get_fontlist (flyfont **F, int restrict_by_language)
{
    ULONG             remfonts, nf, nff;
    FONTMETRICS       *fm;
    int               rc, i;
    char              buffer[64];

    // find how many fonts are available
    nf = 0;
    VioQueryFonts (&remfonts, NULL, sizeof(FONTMETRICS), &nf, NULL, VQF_PUBLIC, hvps);
    nf = remfonts;
    // retrieve information about them
    fm = malloc (sizeof (FONTMETRICS) * nf);
    *F = malloc (sizeof (flyfont) * nf);
    memset (fm, 0, sizeof (fm));
    rc = VioQueryFonts (&remfonts, fm, sizeof(FONTMETRICS), &nf, NULL, VQF_PUBLIC, hvps);
    // filter out what we need
    nff = 0;
    for (i=0; i<nf; i++)
    {
        if (strcmp (fm[i].szFamilyname, "System VIO")) continue;
        //if (!(fm[i].fsType & FM_TYPE_FIXED)) continue;
        
        debug_tools ("Font # %d\nfamily: %s, face: %s, width = %d, height = %d\n",
                     i, fm[i].szFamilyname, fm[i].szFacename, fm[i].lAveCharWidth, fm[i].lMaxBaselineExt);
        
        (*F)[nff].cx = fm[i].lAveCharWidth;
        (*F)[nff].cy = fm[i].lMaxBaselineExt;
        snprintf1 (buffer, sizeof(buffer), "%s %ldx%ld", fm[i].szFacename, fm[i].lAveCharWidth, fm[i].lMaxBaselineExt);
        (*F)[nff].name = strdup (buffer);
        (*F)[nff].signature = (void *)fm[i].lMatch;
        nff++;
    }

    free (fm);
    return nff;
}

/* ------------------------------------------------------------- */

void beep2 (int frequency, int duration)
{
    DosBeep (frequency, duration);
}

/* ------------------------------------------------------------- */

int fly_set_font (char *fontname)
{
    int      rc, deltaX, deltaY, cx, cy, cx0, cy0;
    //ULONG    reset;
    char     *p, *p1, *fn;

    fn = strdup (fontname);
    p = strstr (fn, "VIO ");
    if (p == NULL) {free (fn); return 1;}
    p1 = strchr (p+4, 'x');
    if (p1 == NULL) {free (fn); return 1;}
    *p1 = '\0';
    cx = atoi (p+4);
    cy = atoi (p1+1);
    free (fn);

    if (cx == cxChar && cy == cyChar) return 0;
    debug_tools ("entering set_font_size: cx = %d, cy = %d; x = %d, y = %d\n",
                 cxChar, cyChar, video_hsize(), video_vsize());

    cx0 = cxChar; cy0 = cyChar;
    grab_video ();
    rc = VioSetDeviceCellSize (cy, cx, hvps);
    rc = VioGetDeviceCellSize (&cyChar, &cxChar, hvps);
    release_video ();
    
    deltaX = vio_cols*(cx - cx0);
    deltaY = vio_rows*(cy - cy0);
    WinPostMsg (hwndFrame, WM_FLY_RESIZE,
                MPFROM2SHORT (SHORTBASE+deltaX, SHORTBASE+deltaY), NULL);
    return 0;
}

/* ------------------------------------------------------------- */

void menu_set_switch (int action, int newstate)
{
    WinPostMsg (hwndFrame, WM_FLY_MENU_CHSTATE,
                MPFROM2SHORT (action, newstate), NULL);
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

void set_icon (char *filename)
{
}

