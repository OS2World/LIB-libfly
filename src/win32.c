#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>

#include <windows.h>

#ifndef FROM_LEFT_1ST_BUTTON_PRESSED
#define FROM_LEFT_1ST_BUTTON_PRESSED	(1)
#define RIGHTMOST_BUTTON_PRESSED	(2)
#define FROM_LEFT_2ND_BUTTON_PRESSED	(4)
#endif

#define CELL_WIDTH (sizeof(CHAR_INFO))

/* row, column start at 0 in NT, just as in OS/2  */

static HANDLE   h_input, h_output;
static DWORD    prev_console_mode;
static int      win95;

/* data about screen and working storage */
static char    *saved_screen;
static int     saved_row, saved_col, saved_X, saved_Y;

static int  convert_to_asv_key (INPUT_RECORD in);

/* ------------------------------------------------------------- */

void fly_init (int x0, int y0, int rows, int cols, char *font)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD          xy, size;
    SMALL_RECT     smr;
    OSVERSIONINFO   oi;

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

    // get handle for screen
    h_output = GetStdHandle (STD_OUTPUT_HANDLE);
    if (h_output == INVALID_HANDLE_VALUE)
    {
        fly_error ("cannot obtain console output handle (%ld)\n", GetLastError());
    }

    // get handle for keyboard
    h_input  = GetStdHandle (STD_INPUT_HANDLE);
    if (h_input == INVALID_HANDLE_VALUE)
    {
        fly_error ("cannot obtain input handle (%ld)\n", GetLastError());
    }
    
    // switch keyboard into raw mode saving previous state
    GetConsoleMode (h_input, &prev_console_mode);
    if (SetConsoleMode (h_input, ENABLE_WINDOW_INPUT) == 0)
    {
        fly_error ("error setting keyboard mode (%ld)\n", GetLastError());
    }
    SetFileApisToOEM ();

    // save screen contents and initial cursor position
    GetConsoleScreenBufferInfo (h_output, &csbi);
    saved_screen = malloc (csbi.dwSize.X*csbi.dwSize.Y*2*2);
    saved_X = csbi.dwSize.X;
    saved_Y = csbi.dwSize.Y;
    xy.X = 0;   size.X = csbi.dwSize.X;
    xy.Y = 0;   size.Y = csbi.dwSize.Y;   
    smr.Left = 0;   smr.Right  = csbi.dwSize.X-1;
    smr.Top  = 0;   smr.Bottom = csbi.dwSize.Y-1;
    ReadConsoleOutput (h_output, (CHAR_INFO *)saved_screen, size, xy, &smr);
    saved_col = csbi.dwCursorPosition.X;
    saved_row = csbi.dwCursorPosition.Y;

    video_init (csbi.dwSize.Y, csbi.dwSize.X);
    fl_opt.initialized = TRUE;
    if (rows != -1 && cols != -1)
        video_set_window_size (rows, cols);

    if (video_vsize() == 300 && win95 == 0)
    {
        video_set_window_size (25, video_hsize());
    }
}

/* ------------------------------------------------------------- */

void fly_terminate (void)
{
    COORD          xy, size;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    SMALL_RECT     smr;
    
    SetConsoleMode (h_input, prev_console_mode);

    // restore screen contents and initial cursor position
    GetConsoleScreenBufferInfo (h_output, &csbi);
    xy.X = 0;   size.X = saved_X;
    xy.Y = 0;   size.Y = saved_Y;
    smr.Left = 0;   smr.Right  = saved_X-1;
    smr.Top  = 0;   smr.Bottom = saved_Y-1;
    WriteConsoleOutput (h_output, (CHAR_INFO *)saved_screen, size, xy, &smr);
    free (saved_screen);
    
    xy.X = saved_col;   xy.Y = saved_row;
    SetConsoleCursorPosition (h_output, xy);
    
    // show cursor
    set_cursor (1);
    fl_opt.initialized = FALSE;
}

/* ------------------------------------------------------------- */

int getmessage (int interval)
{
    INPUT_RECORD    inp;
    DWORD           nr;
    double          timergoal;
    int             key, mou_r, mou_c, b1, b2, b3;

    timergoal = clock1 () + (double)interval/1000.;
    do
    {
        if (fl_opt.mouse_active)
        {
            key = mouse_check (clock()*1000/CLOCKS_PER_SEC);
            if (key != -1) return key;
        }
        key = -1;
        nr = 1;
        PeekConsoleInput (h_input, &inp, 1, &nr);
        if (nr == 1)
            ReadConsoleInput (h_input, &inp, 1, &nr);
        if (nr == 1 && inp.EventType == MOUSE_EVENT && fl_opt.mouse_active)
        {
            mou_c = inp.Event.MouseEvent.dwMousePosition.X;
            mou_r = inp.Event.MouseEvent.dwMousePosition.Y;
            if (mou_r < 0 || mou_c < 0) continue;
            b1 = inp.Event.MouseEvent.dwButtonState & FROM_LEFT_1ST_BUTTON_PRESSED ? 1 : 0;
            b2 = inp.Event.MouseEvent.dwButtonState & RIGHTMOST_BUTTON_PRESSED ? 1 : 0;
            b3 = inp.Event.MouseEvent.dwButtonState & FROM_LEFT_2ND_BUTTON_PRESSED ? 1 : 0;
            mouse_received (b1, b2, b3, clock()*1000/CLOCKS_PER_SEC, mou_r, mou_c);
        }
        else if (nr == 1)
        {
            key = convert_to_asv_key (inp);
        }
        
        if (key != -1) return key;
        if (interval == 0) return 0;

        Sleep (30);
    }
    while (interval == -1 || timergoal > clock1());

    return 0;
}

/* ------------------------------------------------------------- */

void set_cursor (int flag)
{
    CONSOLE_CURSOR_INFO   cci;

    GetConsoleCursorInfo (h_output, &cci);
    cci.bVisible = flag ? TRUE : FALSE;
    SetConsoleCursorInfo (h_output, &cci);
}

/* ------------------------------------------------------------- */

void move_cursor (int row, int col)
{
    COORD  xy;
    
    xy.X = col;   xy.Y = row;
    SetConsoleCursorPosition (h_output, xy);
}

/* ------------------------------------------------------------- */

void screen_draw (char *s, int len, int row, int col, char *a)
{
    COORD dwWriteCoord;
    DWORD written;
    
    dwWriteCoord.X = col;
    dwWriteCoord.Y = row;
    WriteConsoleOutputCharacter (h_output, s, len, dwWriteCoord, &written);
    FillConsoleOutputAttribute (h_output, *a, len, dwWriteCoord, &written);
}

/* ------------------------------------------------------------- */

void video_update1 (int *forced)
{
}

/* ------------------------------------------------------------- */

void beep2 (int frequency, int duration)
{
    // on Win95, it's too ugly
    //if (!win95)
    Beep (frequency, duration);
}

/* ------------------------------------------------------------- */

void set_window_name (char *format, ...)
{
    char     buffer[8192];
    va_list  args;
    
    va_start (args, format);
    vsnprintf1 (buffer, sizeof(buffer), format, args);
    va_end (args);
    
    SetConsoleTitle (buffer);
}

/* ------------------------------------------------------------- */

char *get_window_name (void)
{
    char     buffer[1024];

    buffer[0] = '\0';
    GetConsoleTitle (buffer, sizeof (buffer));
    
    if (strlen (buffer) == 0) return strdup (" ");
    return strdup (buffer);
}

/* ------------------------------------------------------------- */

void fly_mouse (int enabled)
{
    SetConsoleMode (h_input, ENABLE_WINDOW_INPUT |
                    (enabled ? ENABLE_MOUSE_INPUT : 0));
    fl_opt.mouse_active = enabled;
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
    {_CtrlA, KB_CTRL|1},     {_CtrlB, KB_CTRL|2},     {_CtrlC, KB_CTRL|3},     {_CtrlD, KB_CTRL|4},
    {_CtrlE, KB_CTRL|5},     {_CtrlF, KB_CTRL|6},     {_CtrlG, KB_CTRL|7},     {_CtrlH, KB_CTRL|8},
    {_CtrlI, KB_CTRL|9},     {_CtrlJ, KB_CTRL|10},    {_CtrlK, KB_CTRL|11},    {_CtrlL, KB_CTRL|12},
    {_CtrlM, KB_CTRL|13},    {_CtrlN, KB_CTRL|14},    {_CtrlO, KB_CTRL|15},    {_CtrlP, KB_CTRL|16},
    {_CtrlQ, KB_CTRL|17},    {_CtrlR, KB_CTRL|18},    {_CtrlS, KB_CTRL|19},    {_CtrlT, KB_CTRL|20},
    {_CtrlU, KB_CTRL|21},    {_CtrlV, KB_CTRL|22},    {_CtrlW, KB_CTRL|23},    {_CtrlX, KB_CTRL|24},
    {_CtrlY, KB_CTRL|25},    {_CtrlZ, KB_CTRL|26},
    
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


    {_CtrlBackSpace, KB_CTRL|0x7f},
    {_CtrlLeftRectBrac, KB_CTRL|0x1b},
    {_CtrlRightRectBrac, KB_CTRL|0x1d},
    {_CtrlBackSlash, KB_CTRL|0x1c},
    
    {_AltBackSpace, KB_ALT|VK_BACK},
    {_AltLeftRectBrac, KB_ALT|'['},
    {_AltRightRectBrac, KB_ALT|']'},
    {_AltSemicolon, KB_ALT|';'},
    {_AltApostrophe, KB_ALT|'\''},
    {_AltBackApostrophe, KB_ALT|'`'},
    {_AltBackSlash, KB_ALT|'\\'},
    {_AltComma, KB_ALT|','},
    {_AltPeriod, KB_ALT|'.'},
    {_AltSlash, KB_ALT|'/'},
    {_AltNumAsterisk, KB_ALT|'*'},
    
    {_AltPlus, KB_ALT|'='},
    {_AltNumPlus, KB_ALT|'+'},
    
    {_AltMinus, KB_ALT|'-'},
    {_AltNumMinus, KB_ALT|'-'},
    
    {'\\', KB_ALT|KB_CTRL|'\\'}
};

struct _transkey TransTable1[] = {
    {_Esc, VK_ESCAPE},
    
    {_F1, VK_F1},    {_F2, VK_F2},    {_F3, VK_F3},    {_F4, VK_F4},    {_F5, VK_F5},
    {_F6, VK_F6},    {_F7, VK_F7},    {_F8, VK_F8},    {_F9, VK_F9},    {_F10, VK_F10},
    {_F11, VK_F11},    {_F12, VK_F12},
    
    {_ShiftF1, KB_SHIFT|VK_F1},    {_ShiftF2, KB_SHIFT|VK_F2},    {_ShiftF3, KB_SHIFT|VK_F3},
    {_ShiftF4, KB_SHIFT|VK_F4},    {_ShiftF5, KB_SHIFT|VK_F5},    {_ShiftF6, KB_SHIFT|VK_F6},
    {_ShiftF7, KB_SHIFT|VK_F7},    {_ShiftF8, KB_SHIFT|VK_F8},    {_ShiftF9, KB_SHIFT|VK_F9},
    {_ShiftF10, KB_SHIFT|VK_F10},    {_ShiftF11, KB_SHIFT|VK_F11},    {_ShiftF12, KB_SHIFT|VK_F12},
    
    {_CtrlF1, KB_CTRL|VK_F1},    {_CtrlF2, KB_CTRL|VK_F2},    {_CtrlF3, KB_CTRL|VK_F3},
    {_CtrlF4, KB_CTRL|VK_F4},    {_CtrlF5, KB_CTRL|VK_F5},    {_CtrlF6, KB_CTRL|VK_F6},
    {_CtrlF7, KB_CTRL|VK_F7},    {_CtrlF8, KB_CTRL|VK_F8},    {_CtrlF9, KB_CTRL|VK_F9},
    {_CtrlF10, KB_CTRL|VK_F10},    {_CtrlF11, KB_CTRL|VK_F11},    {_CtrlF12, KB_CTRL|VK_F12},
    
    {_AltF1, KB_ALT|VK_F1},    {_AltF2, KB_ALT|VK_F2},    {_AltF3, KB_ALT|VK_F3},
    {_AltF4, KB_ALT|VK_F4},    {_AltF5, KB_ALT|VK_F5},    {_AltF6, KB_ALT|VK_F6},
    {_AltF7, KB_ALT|VK_F7},    {_AltF8, KB_ALT|VK_F8},    {_AltF9, KB_ALT|VK_F9},
    {_AltF10, KB_ALT|VK_F10},    {_AltF11, KB_ALT|VK_F11},    {_AltF12, KB_ALT|VK_F12},
    
    {_Home, VK_HOME},    {_CtrlHome, KB_CTRL|VK_HOME},    {_AltHome, KB_ALT|VK_HOME},
    {_End, VK_END},    {_CtrlEnd, KB_CTRL|VK_END},    {_AltEnd, KB_ALT|VK_END},
    
    {_Up, VK_UP},    {_CtrlUp, KB_CTRL|VK_UP},    {_AltUp, KB_ALT|VK_UP},
    {_Down, VK_DOWN},    {_CtrlDown, KB_CTRL|VK_DOWN},    {_AltDown, KB_ALT|VK_DOWN},

    {_Left, VK_LEFT},    {_CtrlLeft, KB_CTRL|VK_LEFT},    {_AltLeft, KB_ALT|VK_LEFT},
    {_Right, VK_RIGHT},    {_CtrlRight, KB_CTRL|VK_RIGHT},    {_AltRight, KB_ALT|VK_RIGHT},
    
    {_PgUp, VK_PRIOR},    {_CtrlPgUp, KB_CTRL|VK_PRIOR},    {_AltPageUp, KB_ALT|VK_PRIOR},
    {_PgDn, VK_NEXT},    {_CtrlPgDn, KB_CTRL|VK_NEXT},    {_AltPageDn, KB_ALT|VK_NEXT},
    
    {_Gold, VK_CLEAR},    {_CtrlGold, KB_CTRL|VK_CLEAR},
    
    {_Insert, VK_INSERT},    {_CtrlInsert, KB_CTRL|VK_INSERT},    {_AltInsert, KB_ALT|VK_INSERT},
    {_Delete, VK_DELETE},    {_CtrlDelete, KB_CTRL|VK_DELETE},    {_AltDelete, KB_ALT|VK_DELETE},

    {_CtrlTab, KB_CTRL|VK_TAB},    {_CtrlNumAsterisk, KB_CTRL|VK_MULTIPLY},
    {_CtrlNumSlash, KB_CTRL|VK_DIVIDE},    {_CtrlNumIns, KB_CTRL|VK_INSERT},
    {_CtrlNumDel, KB_CTRL|VK_DELETE},    {_AltNumSlash, KB_ALT|VK_DIVIDE},
    {_AltTab, KB_ALT|VK_TAB},

    {_CtrlNumPlus, KB_CTRL|VK_ADD},      {_CtrlNumMinus, KB_CTRL|VK_SUBTRACT},
    {_Ctrl2, KB_CTRL|0x32},    {_Ctrl6, KB_CTRL|0x36},    {_CtrlMinus, KB_CTRL|0xbd},

    {_ShiftInsert, KB_SHIFT|VK_INSERT}
};

static int convert_to_asv_key (INPUT_RECORD in)
{
    int           i, key = 0, flags = 0, code = 0;
    //int           mou_c, mou_r, mou_ev, new_buttons;
    //static int    mou_c1=-1, mou_r1=-1, buttons = 0;
    
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    //printf ("console key: KeyCode = %d, ScanCode = %d, State = %ld, "
    //        "char = %d\n",
    //        in.Event.KeyEvent.wVirtualKeyCode,
    //        in.Event.KeyEvent.wVirtualScanCode,
    //        in.Event.KeyEvent.dwControlKeyState,
    //        in.Event.KeyEvent.uChar.AsciiChar);

    //debug_tools ("got event; type = %d\n", in.EventType);

    switch (in.EventType)
    {
    case WINDOW_BUFFER_SIZE_EVENT:
        GetConsoleScreenBufferInfo (h_output, &csbi);
        video_init (csbi.dwSize.Y, csbi.dwSize.X);
        key = FMSG_BASE_SYSTEM +
            FMSG_BASE_SYSTEM_TYPE*SYSTEM_RESIZE +
            FMSG_BASE_SYSTEM_INT2*csbi.dwSize.Y +
            FMSG_BASE_SYSTEM_INT1*csbi.dwSize.X;
        break;

    case KEY_EVENT:

        // ignore key releases
        if (in.Event.KeyEvent.bKeyDown == 0) return -1;

        // ignore Shift/ctrl/Alt/etc.
        if (in.Event.KeyEvent.wVirtualKeyCode == VK_SHIFT ||
            in.Event.KeyEvent.wVirtualKeyCode == VK_CONTROL ||
            in.Event.KeyEvent.wVirtualKeyCode == VK_MENU ||
            in.Event.KeyEvent.wVirtualKeyCode == VK_PAUSE ||
            in.Event.KeyEvent.wVirtualKeyCode == VK_CAPITAL ||
            //in.Event.KeyEvent.wVirtualKeyCode == VK_LWIN ||
            //in.Event.KeyEvent.wVirtualKeyCode == VK_RWIN ||
            //in.Event.KeyEvent.wVirtualKeyCode == VK_APPS ||
            0
           )
            return -1;

        if (in.Event.KeyEvent.dwControlKeyState &
            (LEFT_ALT_PRESSED|RIGHT_ALT_PRESSED))
            flags |= KB_ALT;
        if (in.Event.KeyEvent.dwControlKeyState &
            (LEFT_CTRL_PRESSED|RIGHT_CTRL_PRESSED))
            flags |= KB_CTRL;
        if (in.Event.KeyEvent.dwControlKeyState & SHIFT_PRESSED)
            flags |= KB_SHIFT;

        key = -1;

        // very special cases
        if (in.Event.KeyEvent.wVirtualKeyCode == 9 &&
            in.Event.KeyEvent.wVirtualScanCode == 15 &&
            flags == KB_SHIFT)
            return _ShiftTab;

        // Russian 'p' !
        if (in.Event.KeyEvent.uChar.AsciiChar == (CHAR)0xE0 &&
            (in.Event.KeyEvent.dwControlKeyState & ENHANCED_KEY))
            in.Event.KeyEvent.uChar.AsciiChar = 0;

        // check if it is a super-special key (not a char with ALT/CTRL/SHIFT)
        if (in.Event.KeyEvent.uChar.AsciiChar == 0)
        {
            code = in.Event.KeyEvent.wVirtualKeyCode | flags;
            for (i = 0; i < sizeof(TransTable1)/sizeof(struct _transkey); i++)
                if (TransTable1[i].vk_key == code)
                {
                    key = TransTable1[i].asv_key;
                    break;
                }
        }
        else
        {
            if (flags & (KB_ALT|KB_CTRL))
            {
                code = in.Event.KeyEvent.uChar.AsciiChar | flags;
                for (i = 0; i < sizeof(TransTable2)/sizeof(struct _transkey); i++)
                    if (TransTable2[i].vk_key == code)
                    {
                        key = TransTable2[i].asv_key;
                        break;
                    }
            }
            else
            {
                key = (unsigned char)in.Event.KeyEvent.uChar.AsciiChar;
            }
        }
        break;

    default: // other types of events??
        key = -1;
    }

    return key;
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
    return strdup ("default font");
}

/* ------------------------------------------------------------- */

int fly_get_fontlist (flyfont **F, int restrict_by_language)
{
    return 0;
}

/* ------------------------------------------------------------- */

int video_set_window_size (int rows, int cols)
{
    COORD          size;
    int            rc;

    if (win95) return 1;
    
    size.X = cols;
    size.Y = rows;
    rc = SetConsoleScreenBufferSize (h_output, size);
    if (rc == 0) return 1;
    
    video_init (rows, cols);
    
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

/* ------------------------------------------------------------- */

void set_icon (char *filename)
{
}

