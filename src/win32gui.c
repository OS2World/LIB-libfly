#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <math.h>

#include <windows.h>

#ifndef VK_0
#define VK_0	(48)
#define VK_1	(49)
#define VK_2	(50)
#define VK_3	(51)
#define VK_4	(52)
#define VK_5	(53)
#define VK_6	(54)
#define VK_7	(55)
#define VK_8	(56)
#define VK_9	(57)
#define VK_A	(65)
#define VK_B	(66)
#define VK_C	(67)
#define VK_D	(68)
#define VK_E	(69)
#define VK_F	(70)
#define VK_G	(71)
#define VK_H	(72)
#define VK_I	(73)
#define VK_J	(74)
#define VK_K	(75)
#define VK_L	(76)
#define VK_M	(77)
#define VK_N	(78)
#define VK_O	(79)
#define VK_P	(80)
#define VK_Q	(81)
#define VK_R	(82)
#define VK_S	(83)
#define VK_T	(84)
#define VK_U	(85)
#define VK_V	(86)
#define VK_W	(87)
#define VK_X	(88)
#define VK_Y	(89)
#define VK_Z	(90)
#endif


#include "menu.h"
#include "pc2rgb.h"

#define WM_FLY_FONT WM_USER+1
#define WM_FLY_SIZE WM_USER+2

#define MAXROWS 120
#define MAXCOLS 160

/* --------------------------------------------------------------
 internal variables
 ---------------------------------------------------------------- */
HINSTANCE     s_hInst;
int           s_nShow;
static int    win95;
static HANDLE NewMessage, interface_ready;
static HWND   hwndMain;
static HMENU  hMenu;
static int    current_size = 12;

/*static FILE *d_wnd, *d_fly;*/

/* --------------------------------------------------------------
 internal functions
 ---------------------------------------------------------------- */
LRESULT CALLBACK FlyWndProc (HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam);
static HFONT EzCreateFont (HDC hdc, char *szFaceName, int height);
DWORD WINAPI interface_thread (LPVOID parm);
static void SetColour (HDC hdc, char a);
void fill_submenu (HMENU h, struct _item *M);
static void repaint (HDC hdc, RECT rect);

/* --------------------------------------------------------------
 internal screen buffer
 ---------------------------------------------------------------- */
static int    vrows=-1, vcols=-1;
static char   *buffer_char=NULL, *buffer_attr=NULL;
static int    cxChar, cyChar, cxClient, cyClient;
static int    cursor_visible = TRUE, cursor_X=0, cursor_Y=0;

/* --------------------------------------------------------------
 small LIFO queue implementation
 ---------------------------------------------------------------- */
#define MSG_QUEUE_SIZE   8192
static int queue[MSG_QUEUE_SIZE], nq = 0;
static CRITICAL_SECTION cs1;
int que_get (void);
int que_put (int msg);
int que_n (void);

/* --------------------------------------------------------------------- */

int que_put (int msg)
{
    int    rc;

    EnterCriticalSection (&cs1);
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
    LeaveCriticalSection (&cs1);
    ReleaseSemaphore (NewMessage, 1, NULL);
    return rc;
}

/* --------------------------------------------------------------------- */

int que_get (void)
{
    int i, result;

    EnterCriticalSection (&cs1);
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
    
    LeaveCriticalSection (&cs1);

    if (IS_SYSTEM (result))
    {
        switch (SYS_TYPE(result))
        {
        case SYSTEM_RESIZE:
            //EnterCriticalSection (&cs2);
            vcols = min1 (SYS_INT1 (result), MAXCOLS);
            vrows = min1 (SYS_INT2 (result), MAXROWS);
            //LeaveCriticalSection (&cs2);
            video_init (vrows, vcols);
            break;

        case SYSTEM_DIE:
            exit (1); // ugh, this one is dirty...
            break;
        }
    }
    
    return result;
}

/* --------------------------------------------------------------------- */

int que_n (void)
{
    int result;

    EnterCriticalSection (&cs1);
    result = nq;
    LeaveCriticalSection (&cs1);
    return result;
}


/* ------------------------------------------------------------- */

void fly_init (int x0, int y0, int rows, int cols, char *font)
{
    OSVERSIONINFO   oi;
    DWORD           tid;

    // check whether we're running under buggy Win95 or NT
    oi.dwOSVersionInfoSize = sizeof (oi);
    GetVersionEx ((LPOSVERSIONINFO) &oi);
    switch (oi.dwPlatformId)
    {
    case VER_PLATFORM_WIN32_WINDOWS: 
        win95 = 1; break;
    case VER_PLATFORM_WIN32_NT:      
        win95 = 0; break;
    default:
        win95 = 0;
        fprintf (stderr, "Unknown operating system (neither NT nor Win95)\n");
    }
    
    SetFileApisToOEM ();

    /*d_wnd = fopen ("0_wnd.log", "a");
    setvbuf(d_wnd, NULL, _IONBF, 0);
    d_fly = fopen ("0_fly.log", "a");
    setvbuf(d_fly, NULL, _IONBF, 0);*/

    buffer_char = malloc (MAXROWS*MAXCOLS);
    memset (buffer_char, ' ', MAXROWS*MAXCOLS);
    buffer_attr = malloc (MAXROWS*MAXCOLS);
    memset (buffer_attr, fl_clr.background, MAXROWS*MAXCOLS);
            
    InitializeCriticalSection (&cs1);
    //InitializeCriticalSection (&cs2);
    NewMessage      = CreateSemaphore (NULL, 0, 8, NULL);
    interface_ready = CreateSemaphore (NULL, 0, 8, NULL);
    video_init (25, 80);

    ResetEvent (interface_ready);
    CreateThread (NULL, 0, interface_thread, NULL, 0, &tid);
    WaitForSingleObject (interface_ready, INFINITE);

    fl_opt.initialized = TRUE;
    debug_tools ("init done\n");

    if (font != NULL)
        fly_set_font (font);
}

/* ------------------------------------------------------------- */

void fly_terminate (void)
{
}

/* ------------------------------------------------------------- */

DWORD WINAPI interface_thread (LPVOID parm)
{
    MSG             msg;
    WNDCLASSEX      wndclass;
    char            *szFlyWndClass = "WinFlyWin";
    
    memset (&wndclass, 0, sizeof(WNDCLASSEX));
    wndclass.lpszClassName = szFlyWndClass;
    wndclass.cbSize = sizeof (WNDCLASSEX);
    wndclass.style = CS_HREDRAW | CS_VREDRAW;
    wndclass.lpfnWndProc = FlyWndProc;
    wndclass.hInstance = s_hInst;
    wndclass.hIcon = LoadIcon (s_hInst, "FLY");
    wndclass.hIconSm = LoadIcon (s_hInst, "FLY");
    wndclass.hCursor = LoadCursor (NULL, IDC_ARROW);
    wndclass.hbrBackground = (HBRUSH) GetStockObject (BLACK_BRUSH);
    RegisterClassEx (&wndclass);

    hwndMain = CreateWindow (szFlyWndClass,       /* Class name */
                             " ",                 /* Caption */
                             WS_OVERLAPPEDWINDOW, /* Style */
                             CW_USEDEFAULT,	  /* Initial x (use default) */
                             CW_USEDEFAULT,	  /* Initial y (use default) */
                             CW_USEDEFAULT,	  /* Initial x size (use default) */
                             CW_USEDEFAULT,	  /* Initial y size (use default) */
                             NULL,		  /* No parent window */
                             NULL,		  /* No menu */
                             s_hInst,		  /* This program instance */
                             NULL		  /* Creation parameters */
                            );
	
    ShowWindow (hwndMain, s_nShow);
    UpdateWindow (hwndMain);
    ReleaseSemaphore (interface_ready, 1, NULL);

    while (GetMessage (&msg, hwndMain, 0, 0))
    {
        TranslateMessage (&msg);
        DispatchMessage (&msg);
    }

    return 0;
}

/* ------------------------------------------------------------- */

int getmessage (int interval)
{
    int             key = 0;

    if (que_n() > 0) return que_get ();
    
    // with interval == 0, we only check for keys in buffer
    if (interval == 0)
    {
        key = 0;
    }
    else if (interval > 0)
    {
        WaitForSingleObject (NewMessage, interval);
        key = que_get ();
    }
    else if (interval < 0)
    {
        WaitForSingleObject (NewMessage, INFINITE);
        key = que_get ();
    }
    return key;
}

/* ------------------------------------------------------------- */

void set_cursor (int flag)
{
    cursor_visible = flag;
}

/* ------------------------------------------------------------- */

void move_cursor (int row, int col)
{
    cursor_X = col;
    cursor_Y = row;
}

/* ------------------------------------------------------------- */

void screen_draw (char *s, int len, int row, int col, char *a)
{
    if (buffer_char == NULL || buffer_attr == NULL) return;
    //EnterCriticalSection (&cs2);
    memcpy (buffer_char+col+row*MAXCOLS, s, len);
    memcpy (buffer_attr+col+row*MAXCOLS, a, len);
    //LeaveCriticalSection (&cs2);
}

/* ------------------------------------------------------------- */

void video_flush (void)
{
    InvalidateRect (hwndMain, NULL, FALSE);
    UpdateWindow (hwndMain);
}

/* ------------------------------------------------------------- */

void video_update1 (int *forced)
{
}

/* ------------------------------------------------------------- */

void beep2 (int frequency, int duration)
{
    //Beep (frequency, duration);
}

/* ------------------------------------------------------------- */

void set_window_name (char *format, ...)
{
    char     buffer[1024];
    va_list  args;
    
    va_start (args, format);
    vsnprintf1 (buffer, sizeof(buffer), format, args);
    va_end (args);
    
    SetWindowText (hwndMain, buffer);
}

/* ------------------------------------------------------------- */

char *get_window_name (void)
{
    char     buffer[1024];

    if (GetWindowText (hwndMain, buffer, sizeof(buffer)) == 0)
        return strdup (" ");
    
    return strdup (buffer);
}

/* ------------------------------------------------------------- */

void fly_mouse (int enabled)
{
    // always enabled!
}

/* ------------------------------------------------------------- */

char *get_clipboard (void)
{
    char        *p, *text;
    HANDLE      hmem;

    if (OpenClipboard (NULL) == 0) return NULL;
    if ((hmem = GetClipboardData (CF_TEXT)) == NULL ||
        (p = (char *)GlobalLock (hmem)) == NULL)
    {
        CloseClipboard ();
        return NULL;
    }
    
    // copy clipboard contents to buffer and preprocess it
    text = strdup (p);

    GlobalUnlock (hmem);
    CloseClipboard ();
    
    return text;
}

/* ------------------------------------------------------------- */

void put_clipboard (char *s)
{
    char    *p;
    HGLOBAL hmem;

    if (OpenClipboard (NULL) == 0 || EmptyClipboard () == 0) return;

    if ((hmem = GlobalAlloc (GMEM_MOVEABLE, strlen (s) + 1)) == 0 ||
        (p = (char *)GlobalLock (hmem)) == NULL)
    {
        CloseClipboard();
        return;
    }
        
    strcpy (p, s);

    GlobalUnlock (hmem);
    SetClipboardData (CF_TEXT, hmem);
    CloseClipboard ();
    
    return;
}

/* ------------------------------------------------------------- */

void video_set_window_pos (int x0, int y0)
{
}

/* ------------------------------------------------------------- */

int video_get_window_pos (int *x0, int *y0)
{
    *x0 = 0;
    *y0 = 0;
    return 1;
}

/* ------------------------------------------------------------- */

char *fly_get_font (void)
{
    char  buffer[64];
    
    snprintf1 (buffer, sizeof(buffer), "Lucida Console, %d", current_size);
    return strdup (buffer);
}

/* ------------------------------------------------------------- */

static int fontsizes[] =
{5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24};

int fly_get_fontlist (flyfont **F, int restrict_by_language)
{
    int   i, nfonts = sizeof(fontsizes)/sizeof(fontsizes[0]);
    char  buffer[64];
    
    *F = malloc (sizeof (flyfont) * nfonts);
    for (i=0; i<nfonts; i++)
    {
        (*F)[i].cx = 0;
        (*F)[i].cy = fontsizes[i];
        snprintf1 (buffer, sizeof(buffer), "Lucida Console, %d", fontsizes[i]);
        (*F)[i].name = strdup (buffer);
        (*F)[i].signature = 0;
    }

    return nfonts;
}

/* ------------------------------------------------------------- */

int video_set_window_size (int rows, int cols)
{
    return 0;
}

/* ------------------------------------------------------------- */

void fly_launch (char *command, int wait, int pause)
{
    STARTUPINFO          si;
    PROCESS_INFORMATION  pi;
    char                 *cmd, *p;
    
    if (wait)
    {
        p = get_window_name ();
        set_window_name ("running %s...", command);

        si.cb = sizeof (STARTUPINFO);
        memset (&si, 0, sizeof (STARTUPINFO));
        CreateProcess (NULL, command, NULL, NULL, FALSE, 0, NULL, NULL,
                       &si, &pi);
        WaitForSingleObject (pi.hProcess, INFINITE);
        CloseHandle (pi.hProcess);
        CloseHandle (pi.hThread);
        
        set_window_name (p);
        free (p);
        video_update (1);
    }
    else
    {
        cmd = malloc (strlen (command)+32);
        snprintf1 (cmd, strlen (command)+32, "start %s", command);
        system (cmd);
        free (cmd);
    }
}

/* -------------------------------------------------------------
 names are in the form: "Lucida console: 20x10"
 */

int fly_set_font (char *fontname)
{
    int      cy;
    char     *p, *fn;

    fn = strdup (fontname);
    p = strstr (fn, ", ");
    if (p == NULL) {free (fn); return 1;}
    *p = '\0';
    cy = atoi (p+2);
    if (cy == 0) {free (fn); return 1;}

    PostMessage (hwndMain, WM_FLY_FONT, cy, 0);

    free (fn);
    return 0;
}


/* ------------------------------------------------------------- */

void fly_process_args (int *argc, char **argv[], char ***envp)
{
}

/* ------------------------------------------------------------- */

int menu_process_line (struct _item *L, int init, int opendrop)
{
    return MENU_RC_CANCELLED;
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
        EnableMenuItem (hMenu, actions[i], newstatus ? MF_ENABLED : MF_GRAYED);
        if (actions[i] < MAX_ACT) cached[actions[i]] = newstatus;
    }
}

/* ------------------------------------------------------------- */

void menu_activate (struct _item *L)
{
    hMenu = CreateMenu ();
    fill_submenu (hMenu, L);
    SetMenu (hwndMain, hMenu);
    fly_active_menu = L;
}

/* ------------------------------------------------------------- */

void fill_submenu (HMENU h, struct _item *M)
{
    int       i;
    HMENU     hm;
    char      *p;

    // fill every submenu
    for (i=0; M[i].type != ITEM_TYPE_END; i++)
    {
        switch (M[i].type)
        {
        case ITEM_TYPE_ACTION:
        case ITEM_TYPE_SWITCH:
            p = str_strdup1 (M[i].text, 64);
            if (M[i].hotkey != 0)
                str_insert (p, '&', M[i].hkpos);
            if (M[i].keydesc[0] != '\0')
            {
                strcat (p, "\t");
                strcat (p, M[i].keydesc);
            }
            AppendMenu (h, MF_STRING, M[i].body.action, p);
            free (p);
            break;
            
        case ITEM_TYPE_SUBMENU:
            hm = CreateMenu ();
            fill_submenu (hm, M[i].body.submenu);
            p = str_strdup1 (M[i].text, 64);
            if (M[i].hotkey != 0)
                str_insert (p, '&', M[i].hkpos);
            if (M[i].keydesc[0] != '\0')
            {
                strcat (p, "\t");
                strcat (p, M[i].keydesc);
            }
            AppendMenu (h, MF_POPUP, (UINT)hm, p);
            free (p);
            break;
            
        case ITEM_TYPE_SEP:
            AppendMenu (h, MF_SEPARATOR, 0, NULL);
            break;
        }
    }
}

/* ------------------------------------------------------------- */

void menu_deactivate (struct _item *L)
{
}

/* --------------------------------------------------------------------- */

void menu_enable (void)
{
}

/* --------------------------------------------------------------------- */

void menu_disable (void)
{
}

/* ------------------------------------------------------------- */

int  menu_check_key (struct _item *L, int key)
{
    return 0;
}

/* ------------------------------------------------------------- */

void menu_set_switch (int action, int newstate)
{
    CheckMenuItem (hMenu, action, newstate ? MF_CHECKED : MF_UNCHECKED);
}

/* -------------------------------------------------------------  */

void ifly_error_gui (char *message)
{
    MessageBox (NULL, message, "Error", MB_OK|MB_ICONERROR|MB_SETFOREGROUND);
}

/* ------------------------------------------------------------- */

int ifly_ask_gui (int flags, char *questions, char *answers)
{
    int choice;

    if (flags & ASK_YN)
    {
        choice = MessageBox (NULL, questions, " ", MB_YESNO|MB_SETFOREGROUND);
        if (choice == IDYES) return 1;
        if (choice == IDNO) return 0;
        return 0;
    }
    else if (strchr (answers, '\n') != NULL)
    {
        return ifly_ask_txt (flags, questions, answers);
    }
    else
    {
        MessageBox (NULL, questions, fl_opt.appname, MB_OK|MB_SETFOREGROUND);
        return 0;
    }
}

/* ------------------------------------------------------------- */

#include "winkeytables.c"

int win2fly (WPARAM wParam, LPARAM lParam)
{
    int i, is_shift = 0, is_alt = 0, is_ctrl = 0;
    
    if (GetKeyState (VK_MENU) < 0)     is_alt   = 1;
    if (GetKeyState (VK_CONTROL) < 0)  is_ctrl  = 1;
    if (GetKeyState (VK_SHIFT) < 0)    is_shift = 1;

    // ignore keys with multiple modificators
    if (is_alt + is_ctrl + is_shift > 1) return -1;

    // lookup codes without ctrl/alt and having AsciiChar
    //if (!is_alt && !is_ctrl &&
    
    // lookup virtual code in the table
    for (i = 0; i < sizeof(winkeytable)/sizeof(struct _transkey); i++)
    {
        if (winkeytable[i].vk_key == wParam)
        {
            if (is_shift) return winkeytable[i].shift;
            if (is_ctrl)  return winkeytable[i].ctrl;
            if (is_alt)   return winkeytable[i].alt;
            return               winkeytable[i].plain;
        }
    }
    return -1;
}

/* -------------------------------------------------------------- */

static HFONT EzCreateFont (HDC hdc, char *szFaceName, int height)
{
    HFONT      hFont;

    SaveDC (hdc);

    SetGraphicsMode (hdc, GM_ADVANCED);
    ModifyWorldTransform (hdc, NULL, MWT_IDENTITY);
    SetViewportOrgEx (hdc, 0, 0, NULL);
    SetWindowOrgEx   (hdc, 0, 0, NULL);

    hFont = CreateFont (height, 0, 0, 0,
                        FW_DONTCARE, FALSE, FALSE, FALSE, OEM_CHARSET,
                        OUT_CHARACTER_PRECIS, CLIP_DEFAULT_PRECIS,
                        DEFAULT_QUALITY, FF_DONTCARE|FIXED_PITCH, szFaceName);

    RestoreDC (hdc, -1);

    return hFont;
}

/* -------------------------------------------------------------- */

static void SetColour (HDC hdc, char a)
{
    unsigned char  a1;
    int            rgb1;
    
    a1 = (a & 0xf0)/16;
    rgb1 = RGB(pc2rgb[a1].r, pc2rgb[a1].g, pc2rgb[a1].b);
    SetBkColor (hdc, rgb1);
    a1 = a & 0x0f;
    rgb1 = RGB(pc2rgb[a1].r, pc2rgb[a1].g, pc2rgb[a1].b);
    SetTextColor (hdc, rgb1);
}

/* -------------------------------------------------------------- */

static void repaint (HDC hdc, RECT rect)
{
    int              rgb1;
    unsigned char    a, a0, a1;
    int              r, c, r1, c1, r2, c2, s, e;

    // compute invalid area
    r1 = max1 (rect.top/cyChar - 1, 0);
    r2 = min1 (rect.bottom/cyChar + 1, vrows-1);
    c1 = max1 (rect.left/cxChar - 1, 0);
    c2 = min1 (rect.right/cxChar + 1, vcols-1);
    
    for (r=r1; r<=r2; r++)
    {
        for (c=c1; c<=c2; )
        {
            // look for place where attributes are the same
            a = buffer_attr[c+r*MAXCOLS];
            s = c; e = c+1;
            while (buffer_attr[e+r*MAXCOLS] == a && e<=c2) e++;
            SetColour (hdc, a);
            TextOut (hdc, s*cxChar, r*cyChar, buffer_char+s+r*MAXCOLS, e-s);
            c = e;
        }
    }
    
    if (cursor_visible && cursor_X >= c1 && cursor_X <= c2 &&
        cursor_Y >= r1 && cursor_Y <= r2)
    {
        a0 = buffer_attr[cursor_X+cursor_Y*MAXCOLS];
        a1 = a0 & 0x0f;
        rgb1 = RGB(pc2rgb[a1].r, pc2rgb[a1].g, pc2rgb[a1].b);
        SetTextColor (hdc, rgb1);
        a0 = 0x70 - (a0 & 0xF0) + (a0 & 0x0F);
        a1 = (a0 & 0xf0)/16;
        rgb1 = RGB(pc2rgb[a1].r, pc2rgb[a1].g, pc2rgb[a1].b);
        SetBkColor (hdc, rgb1);
        TextOut (hdc, cursor_X*cxChar, cursor_Y*cyChar,
                 buffer_char+cursor_X+cursor_Y*MAXCOLS, 1);
    }
}

/* --------------------------------------------------------------------- */

LRESULT CALLBACK FlyWndProc (HWND hwnd, UINT nMsg, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    int         key, new_rows, new_cols;
    int         cxClient1, cyClient1, deltax, deltay;
    PAINTSTRUCT ps;
    TEXTMETRIC  tm;
    HFONT       Font;
    RECT        rect;

    switch (nMsg)
    {
    case WM_CREATE:
        hdc = GetDC (hwnd);
        Font = EzCreateFont (hdc, "Lucida Console", current_size);
        if (Font != NULL)
            SelectObject (hdc, Font);
        else
            SelectObject (hdc, GetStockObject (OEM_FIXED_FONT));
        GetTextMetrics (hdc, &tm);
        cxChar = tm.tmAveCharWidth;
        cyChar = tm.tmHeight + tm.tmExternalLeading;
        SelectObject (hdc, GetStockObject (SYSTEM_FONT));
        if (Font != NULL) DeleteObject (Font);
        ReleaseDC (hwnd, hdc);
        return 0;

    case WM_KEYDOWN:
        key = win2fly (wParam, lParam);
        if (key != -1) que_put (key);
        break;
        
    case WM_SIZE:
        PostMessage (hwnd, WM_FLY_SIZE, wParam, lParam);
        cxClient1 = LOWORD (lParam);
        cyClient1 = HIWORD (lParam);
        if (cxClient1 % cxChar != 0 || cyClient1 % cyChar != 0)
        {
            deltax = cxClient1 % cxChar;
            deltay = cyClient1 % cyChar;
            GetWindowRect (hwnd, &rect);
            SetWindowPos (hwnd, HWND_TOP, 0, 0,
                          rect.right-rect.left-deltax,
                          rect.bottom-rect.top-deltay,
                          SWP_NOMOVE|SWP_NOZORDER);
        }
        return 0;
        
    case WM_PAINT:
        hdc = BeginPaint (hwnd, &ps);
        Font = EzCreateFont (hdc, "Lucida Console", current_size);
        if (Font != NULL)
            SelectObject (hdc, Font);
        else
            SelectObject (hdc, GetStockObject (OEM_FIXED_FONT));
        repaint (hdc, ps.rcPaint);
        SelectObject (hdc, GetStockObject (SYSTEM_FONT));
        if (Font != NULL) DeleteObject (Font);
        EndPaint (hwnd, &ps);
        return 0;

    case WM_COMMAND:
        que_put (FMSG_BASE_MENU + LOWORD(wParam));
        return 0;

    case WM_FLY_FONT:
        hdc = GetDC (hwnd);
        current_size = wParam;
        Font = EzCreateFont (hdc, "Lucida Console", current_size);
        if (Font != NULL)
            SelectObject (hdc, Font);
        else
            SelectObject (hdc, GetStockObject (OEM_FIXED_FONT));
        GetTextMetrics (hdc, &tm);
        cxChar = tm.tmAveCharWidth;
        cyChar = tm.tmHeight + tm.tmExternalLeading;
        SelectObject (hdc, GetStockObject (SYSTEM_FONT));
        if (Font != NULL) DeleteObject (Font);
        que_put (FMSG_BASE_SYSTEM +
                 FMSG_BASE_SYSTEM_TYPE*SYSTEM_RESIZE +
                 FMSG_BASE_SYSTEM_INT2*(cyClient/cyChar) +
                 FMSG_BASE_SYSTEM_INT1*(cxClient/cxChar));
        ReleaseDC (hwnd, hdc);
        return 0;
        
    case WM_FLY_SIZE:
        cxClient = LOWORD (lParam);
        cyClient = HIWORD (lParam);
        new_cols = cxClient/cxChar;
        new_rows = cyClient/cyChar;
        if (new_rows != vrows || new_cols != vcols)
        {
            que_put (FMSG_BASE_SYSTEM +
                     FMSG_BASE_SYSTEM_TYPE*SYSTEM_RESIZE +
                     FMSG_BASE_SYSTEM_INT2*(cyClient/cyChar) +
                     FMSG_BASE_SYSTEM_INT1*(cxClient/cxChar));
        }
        return 0;
        
    case WM_DESTROY:
        que_put (FMSG_BASE_SYSTEM + FMSG_BASE_SYSTEM_TYPE*SYSTEM_DIE);
        PostQuitMessage (0);
        return 0;
    }

    return DefWindowProc (hwnd, nMsg, wParam, lParam);
}

/* ------------------------------------------------------------- */

void set_icon (char *filename)
{
}

