#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#ifndef __sun__
#include <termios.h>
#include <termcap.h>
#endif

#ifdef __sun__
#include <curses.h>
#include <term.h>
#endif

#ifdef __EMX__
#include <sys/select.h>
#endif

#define USE_DIRECT_WRITE  0
#define STDIN_FD      0
#define STDOUT_FD     1

#ifndef VDSUSP
        #define VDSUSP		11
#endif

static char            *clipboard_text;
static int             mono=0, can_print_rd_corner=0;
static char            *cmd_hide_cursor=NULL, *cmd_show_cursor=NULL, *cmd_ceol=NULL;
static int             count_plain=0, count_ctrl=0;
static int             current_row=-1, current_col=-1, current_attribute=-1;

static struct termios  saved_tio, tio;

static int pc2ansi[] = {0, 4, 2, 6, 1, 5, 3, 7};

static void ansi_move_cursor (int row, int col);
static void ansi_set_attributes (int a);
static void ansi_fullclear (void);
static void ansi_out (char *s);
static void ansi_cel (void);
static void ansi_mouse (int flag);

static int  convert_to_asv_key (unsigned long long ch);
static int  getct (int interval);
static void setup_termcap (void);
static unsigned long long string2key (char *str);

static void sig_interrupt (int sig);
static void sig_winch (int sig);
static void sig_stopped (int sig);
static void sig_resumed (int sig);

typedef void (*sighandler_t2)(int);
sighandler_t2 signal2 (int signum, sighandler_t2 handler);

/*
 Mouse-in-the-xterm-hack, inspired by wonderful program Pine
 message format:  \033[M\x23\x68\x86
 \033[M   prologue
 0x21     button code; "&" with 3 (0 - button 1, 1 - button 3, 2 - button 2)
 0x68     column + 0x21
 0x86     row + 0x21
 mouse click generates two messages:
 1) where button was down, with corresponding button marked active
 2) where button was released, button code = 0x23
 correspondingly, the code for such "key" would be (each group is a byte)
 row+0x21  column+0x21 button-code 'M' '[' Esc
 00        00          00          4D   5B 1b
 */

/* ------------------------------------------------------------- */

void fly_init (int x0, int y0, int rows, int cols, char *font)
{
    struct winsize   ws;
    char             *p;

    setup_termcap ();
    clipboard_text = NULL;
    
    // we need unbuffered input for select() to work
    fflush (stdin);
    setvbuf (stdin, NULL, _IONBF, 0);

    // ignore some signals, process some others
    signal  (SIGHUP, SIG_IGN);
    signal  (SIGINT, sig_interrupt);
    signal2 (SIGWINCH, sig_winch);
#ifndef __bsdi__    
    signal2 (SIGTSTP, sig_stopped);
    signal2 (SIGCONT, sig_resumed);
#endif

    if (fl_opt.platform == PLATFORM_BEOS_TERM)
       signal (SIGTSTP, SIG_IGN);

    // get terminal mode and save it
    tcgetattr (STDIN_FD, &tio);
    saved_tio = tio;

    // set terminal parameters
    /* stty sane - input:
     -ignbrk brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr
     icrnl ixon -ixoff -iuclc -ixany imaxbel */
    tio.c_iflag &= ~ISTRIP;
    tio.c_iflag &= ~(ISTRIP|ICRNL|INLCR|IGNCR);
        
    /* stty sane - output:
     opost -olcuc -ocrnl onlcr -onocr -onlret -ofill -ofdel
     nl0 cr0 tab0 bs0 vt0 ff0 */
    tio.c_oflag &= ~(ONLCR);
    
    /* stty sane - control:
     -parenb -parodd cs8 hupcl -cstopb cread -clocal -crtscts */
    tio.c_cflag &= ~(CSIZE | HUPCL);
    tio.c_cflag |=  CS8;
    
    /* stty sane - local:
     isig icanon iexten echo echoe echok -echonl -noflsh
     -xcase -tostop -echoprt echoctl echoke */
    //tio.c_lflag &= ~ISIG;
    tio.c_lflag &= ~(ICANON | ECHO | ECHOCTL);

    tio.c_cc[VMIN]  = 1;
    tio.c_cc[VTIME] = 0;
    tio.c_cc[VQUIT] = _POSIX_VDISABLE;
    tio.c_cc[VDSUSP] = _POSIX_VDISABLE;
    tio.c_cc[VSTART] = _POSIX_VDISABLE;
    tio.c_cc[VSTOP]  = _POSIX_VDISABLE;

    tcsetattr (STDIN_FD, TCSADRAIN, &tio);

    /* determine terminal dimensions */
    rows = 0;
    cols = 0;
    
    if (ioctl (STDIN_FD, TIOCGWINSZ, &ws) == 0)
    {
        rows = ws.ws_row;
        cols = ws.ws_col;
    }
    else
    {
        p = getenv ("LINES");
        if (p != NULL) rows = atoi (p);
        p = getenv ("COLUMNS");
        if (p != NULL) cols = atoi (p);
    }
    
    // fallback values
    if (rows == 0 || cols == 0)
    {
        fprintf (stderr, "warning: cannot determine terminal window dimensions; "
                 "assuming 80 x 24\n");
        rows = 24;
        cols = 80;
    }

    video_init (rows, cols);
    fl_opt.initialized = TRUE;
}

/* ------------------------------------------------------------- */

void fly_mouse (int enabled)
{
    // enable mouse in xterm if requested
    switch (enabled)
    {
    case TRUE:
        if (fl_opt.mouse_active == TRUE) break;
        fl_opt.mouse_nb = 3;
        if (getenv ("DISPLAY") != NULL || fl_opt.platform == PLATFORM_BEOS_TERM)
        {
            ansi_mouse (TRUE);
            current_row = -1, current_col = -1;
            fl_opt.mouse_active = TRUE;
        }
        break;
        
    case FALSE:
        if (fl_opt.mouse_active == FALSE) break;
        if (getenv ("DISPLAY") != NULL || fl_opt.platform == PLATFORM_BEOS_TERM)
        {
            ansi_mouse (FALSE);
        }
        current_row = -1, current_col = -1;
        fl_opt.mouse_active = FALSE;
        break;
    }
}

/* ------------------------------------------------------------- */

void fly_terminate (void)
{
    if (!backgrounded)
    {
        set_cursor (1);
        tcsetattr (STDIN_FD, TCSADRAIN, &saved_tio);
        ansi_move_cursor (1, 1);
        ansi_fullclear ();
        if (!USE_DIRECT_WRITE) fflush (stdout);
    }
    
    video_terminate ();

    debug_tools ("bytes written to terminal: strings: %d, control: %d\n",
                 count_plain, count_ctrl);
    
    fly_mouse (FALSE);
    fl_opt.initialized = FALSE;
}

/* ------------------------------------------------------------- */

struct _keytable
{
    int                  asv_key;
    unsigned long long   curses_key;
};

/* defaults, defaults.... */

struct _keytable TransTable1[] = {

    /* Linux xterm */
    
    {_Insert, 0x7e325b1bLL},        /*{_Delete, 0x7f},*/
    {_Home, 0x485b1b},              {_End, 0x465b1b},
    {_PgUp, 0x7e355b1bLL},          {_PgDn, 0x7e365b1bLL},
    {_Up, 0x415b1b},                {_Down, 0x425b1b},
    {_Left, 0x445b1b},              {_Right, 0x435b1b},
    {_Gold, 0x455b1b},              {_Delete, 0x7e335b1bLL},
    
    {_F1, 0x504f1b},                {_F2, 0x514f1b},
    {_F3, 0x524f1b},                {_F4, 0x534f1b},
    {_F5, 0x7e35315b1bLL},          {_F6, 0x7e37315b1bLL},
    {_F7, 0x7e38315b1bLL},          {_F8, 0x7e39315b1bLL},
    {_F9, 0x7e30325b1bLL},          {_F10, 0x7e31325b1bLL},
    
    /* Linux console */
    
    {_F1, 0x415b5b1bL},              {_F2, 0x425b5b1bL},
    {_F3, 0x435b5b1bL},              {_F4, 0x445b5b1bL},
    {_F5, 0x455b5b1bL},              
    {_ShiftF1, 0x7e33325b1bLL},      {_ShiftF2, 0x7e34325b1bLL},
    {_ShiftF3, 0x7e35325b1bLL},      {_ShiftF4, 0x7e36325b1bLL},
    {_ShiftF5, 0x7e38325b1bLL},      {_ShiftF6, 0x7e39325b1bLL},
    {_ShiftF7, 0x7e31335b1bLL},      {_ShiftF8, 0x7e32335b1bLL},
    {_ShiftF9, 0x7e33335b1bLL},      {_ShiftF10, 0x7e34335b1bLL},
    
    {_Home, 0x7e315b1bLL},          {_End, 0x7e345b1bLL},
    
    /* ANSI driver by Maloff */
    /*
    {_F5, 0x544f1b},       {_F6, 0x554f1b},       {_F7, 0x564f1b},
    {_F8, 0x574f1b},       {_F9, 0x584f1b},       {_F10, 0x594f1b},
    {_ShiftF1, 0x704f1b},  {_ShiftF2, 0x714f1b},  {_ShiftF3, 0x724f1b},
    {_ShiftF4, 0x734f1b},  {_ShiftF5, 0x744f1b},  {_ShiftF6, 0x754f1b},
    {_ShiftF7, 0x764f1b},  {_ShiftF8, 0x774f1b},  {_ShiftF9, 0x784f1b},
    {_ShiftF10, 0x794f1b},
    */
    
    /* FreeBSD console */
    
    {_F1, 0x4d5b1bL},                {_F2,  0x4e5b1bL},
    {_F3, 0x4f5b1bL},                {_F4,  0x505b1bL},
    {_F5, 0x515b1bL},                {_F6,  0x525b1bL},
    {_F7, 0x535b1bL},                {_F8,  0x545b1bL},
    {_F9, 0x555b1bL},                {_F10, 0x565b1bL},
    {_F11, 0x575b1bL},               {_F12, 0x585b1bL},
                  
    {_ShiftF1,  0x595b1bLL},         {_ShiftF2,  0x5a5b1bLL},
    {_ShiftF3,  0x615b1bLL},         {_ShiftF4,  0x625b1bLL},
    {_ShiftF5,  0x635b1bLL},         {_ShiftF6,  0x645b1bLL},
    {_ShiftF7,  0x655b1bLL},         {_ShiftF8,  0x665b1bLL},
    {_ShiftF9,  0x675b1bLL},         {_ShiftF10, 0x685b1bLL},
    {_ShiftF11, 0x695b1bLL},         {_ShiftF12, 0x6a5b1bLL},
    
    {_BackSpace, 0x7f},
    {_PgUp, 0x495b1bLL},             {_PgDn, 0x475b1bLL},
    {_Insert, 0x4c5b1bLL},
    
    {_ShiftTab, 0x5a5b1bL},

    /* Sun -- xterm */

    {_F1, 0x7e31315b1bLL},  {_F2, 0x7e32315b1bLL},
    {_F3, 0x7e33315b1bLL},  {_F4, 0x7e34315b1bLL},

    /* Sun -- shelltool */

    {_F1, 0x7a3432325b1bLL},  {_F2, 0x7a3532325b1bLL},
    {_F3, 0x7a3632325b1bLL},  {_F4, 0x7a3732325b1bLL},
    {_F5, 0x7a3832325b1bLL},  {_F6, 0x7a3932325b1bLL},
    {_F7, 0x7a3033325b1bLL},  {_F8, 0x7a3133325b1bLL},
    {_F9, 0x7a3233325b1bLL},  {_F10, 0x7a3333325b1bLL},
    {_F11, 0x7a3433325b1bLL}, {_F12, 0x7a3533325b1bLL}
};

/* table which is actually formed from /etc/termcap in run time */

struct _keytable TransTable2[] =
{
    {_Insert, 0}, {_Delete, 0},  {_Home, 0}, {_End, 0},
    {_Up, 0},     {_Down, 0},    {_Left, 0}, {_Right, 0},
    {_PgUp, 0},   {_PgDn, 0},

    {_F1, 0}, {_F2, 0},  {_F3, 0},  {_F4, 0},
    {_F5, 0}, {_F6, 0},  {_F7, 0},  {_F8, 0},
    {_F9, 0}, {_F10, 0}, {_F11, 0}, {_F12, 0}
};

/* things which never rust */

struct _keytable TransTable3[] =
{
    {_Esc, 0x1b1bLL},

    {_AltA, 0x611b}, {_AltB, 0x621b}, {_AltC, 0x631b}, {_AltD, 0x641b},
    {_AltE, 0x651b}, {_AltF, 0x661b}, {_AltG, 0x671b}, {_AltH, 0x681b},
    {_AltI, 0x691b}, {_AltJ, 0x6a1b}, {_AltK, 0x6b1b}, {_AltL, 0x6c1b},
    {_AltM, 0x6d1b}, {_AltN, 0x6e1b}, {_AltO, 0x6f1b}, {_AltP, 0x701b},
    {_AltQ, 0x711b}, {_AltR, 0x721b}, {_AltS, 0x731b}, {_AltT, 0x741b},
    {_AltU, 0x751b}, {_AltV, 0x761b}, {_AltW, 0x771b}, {_AltX, 0x781b},
    {_AltY, 0x791b}, {_AltZ, 0x7a1b},
    {_Alt0, 0x301b}, {_Alt1, 0x311b}, {_Alt2, 0x321b}, {_Alt3, 0x331b},
    {_Alt4, 0x341b}, {_Alt5, 0x351b}, {_Alt6, 0x361b}, {_Alt7, 0x371b},
    {_Alt8, 0x381b}, {_Alt9, 0x391b},
    
    {_AltBackApostrophe, 0x601b},         {_AltMinus, 0x2d1b},
    {_AltPlus, 0x3d1b},                   {_AltBackSlash, 0x5c1b},
    {_AltBackSpace, 0x7f1b},              {_AltTab, 0x091b},
    {_AltEnter, 0x0d1b},                  {_AltLeftRectBrac, 0x5b1b},
    {_AltRightRectBrac, 0x5d1b},          {_AltSemicolon, 0x3b1b},
    {_AltApostrophe, 0x271b},             {_AltComma, 0x2c1b},
    {_AltPeriod, 0x2e1b},                 {_AltSlash, 0x2f1b},
    {_AltSpace, 0x201b},                  

};

/* ------------------------------------------------------------- */

static int b1state = 0, b2state = 0, b3state = 0;

static int convert_to_asv_key (unsigned long long ch)
{
    int i, key, mou_r, mou_c, buttns, mou_ev;
    struct timeval  tv;

    key = 0;
    mou_ev = -1;
    //debug_tools ("entered convert_to_asv_key; key = %Lx\n", ch);

    if (ch == 0x7f) return _BackSpace;
    if (ch <= 255)  return  (int) ch;

    // check things which are common for every terminal
    for (i = 0; i < sizeof(TransTable3)/sizeof(TransTable3[0]); i++)
        if (TransTable3[i].curses_key == ch)
        {
            key = TransTable3[i].asv_key;
            break;
        }
    //debug_tools ("table3 giveth %d\n", key);
    if (key != 0) return key;
    
    // check table from termcap
    if (fl_opt.use_termcap)
    {
        for (i = 0; i < sizeof(TransTable2)/sizeof(TransTable2[0]); i++)
            if (TransTable2[i].curses_key == ch)
            {
                key = TransTable2[i].asv_key;
                break;
            }
        //debug_tools ("table2 giveth %d\n", key);
        if (key != 0) return key;
    }

    // check default table
    for (i = 0; i < sizeof(TransTable1)/sizeof(TransTable1[0]); i++)
        if (TransTable1[i].curses_key == ch)
        {
            key = TransTable1[i].asv_key;
            break;
        }
    //debug_tools ("table1 giveth %d\n", key);

    // check mouse-in-xterm hack
    if ((ch & 0xFFFFFF) == 0x4D5B1b)
    {
        ch = (ch >> 24);
        mou_r = ((ch >> 16) & 0xFF) - 0x21;
        mou_c = ((ch >>  8) & 0xFF) - 0x21;
        
        buttns = 0;
        switch (ch & 0x03)
        {
        case 0:  buttns = 1; break;
        case 1:  buttns = 3; break;
        case 2:  buttns = 2; break;
        case 3:  buttns = 0;
        }
        
        switch (buttns)
        {
        case 1: b1state = 1; break;
        case 2: b2state = 1; break;
        case 3: b3state = 1; break;
        case 0: if (b1state) { b1state = 0; break; }
                if (b2state) { b2state = 0; break; }
                if (b3state) { b3state = 0; break; }
        }
        
        gettimeofday (&tv, NULL);
        mouse_received (b1state, b2state, b3state, tv.tv_sec*1000+tv.tv_usec/1000, mou_r, mou_c);
        
        if (mou_ev != -1)
            key = FMSG_BASE_MOUSE + FMSG_BASE_MOUSE_EVTYPE*mou_ev +
                FMSG_BASE_MOUSE_X*mou_c + FMSG_BASE_MOUSE_Y*mou_r;
    }
    
    return key;
}

/* ------------------------------------------------------------- */

#define ESC_TIMEOUT 600       // milliseconds

static int have_esc = 0;

int getmessage (int interval)
{
    int                  i, ch0, ch1;
    unsigned long long   k;
    struct timeval       tv;

    if (interval >=0 && backgrounded) return 0;

again:
    gettimeofday (&tv, NULL);
    ch0 = mouse_check (tv.tv_sec*1000+tv.tv_usec/1000);
    if (ch0 != -1) return ch0;
    
    if (have_esc)
    {
       ch0 = _Esc;
       have_esc = 0;
    }
    else
    {
       ch0 = getct (interval);
    }
    
    if (ch0 == 0) return 0;
    if (ch0 == '`') return _Esc;
    if (ch0 == 10) return _Enter;
    if (ch0 == _CtrlZ)
    {
        raise (SIGTSTP);
        return 0;
    }

    if (ch0 == _Esc)
    {
        k = ch0;
        for (i=1; i<8; i++)
        {
            // get next member of sequence
            if (i == 1)
                ch1 = getct (ESC_TIMEOUT);
            else
                ch1 = getct (60);
            // stop when no sequence members are standing in the input queue
            if (ch1 == 0) break;
            // check for Esc-Esc
            if (ch1 == _Esc && i == 1) return _Esc;
            // check for start of next esc sequence
            if (ch1 == _Esc && i > 1)
            {
                have_esc = 1;
                break;
            }
            // combine into single value
            k += ((unsigned long long)ch1) << (i*8);
        }
    }
    else
        k = ch0;

    ch0 = convert_to_asv_key (k);
    if (ch0 == 0) goto again;

    return ch0;
}

/* ------------------------------------------------------------- */

void set_cursor (int flag)
{
    char *cmd = NULL;

    if (!flag) cmd = cmd_hide_cursor;
    if (flag)  cmd = cmd_show_cursor;

    if (cmd != NULL) 
    {
        ansi_out (cmd);
        if (!USE_DIRECT_WRITE) fflush (stdout);
    }
}

/* ------------------------------------------------------------- */

void move_cursor (int row, int col)
{
    ansi_move_cursor (row, col);
    if (!USE_DIRECT_WRITE) fflush (stdout);
}

/* ------------------------------------------------------------- */

void screen_draw (char *s, int len, int row, int col, char *a)
{
    char buffer[1024];

    if (backgrounded) return;

    // check for lower right corner
    if (!can_print_rd_corner && row == video_vsize()-1 && col+len >= video_hsize())
        len = video_hsize() - col - 1;
    
    // make a NULL-terminated copy
    memcpy (buffer, s, min1(len, sizeof(buffer)-1));
    buffer[min1(len, sizeof(buffer)-1)] = '\0';

    // optimize for space-trails: 
    // background colour is black, and string spans to the end of the line
    ansi_set_attributes (*a);
    ansi_move_cursor (row, col);
    if (fl_opt.use_ceol && *a < 0x10 && col + len >= video_hsize())
    {
        str_strip (buffer, " ");
        if (strlen (buffer) != 0)
        {
            // not only spaces
            ansi_out (buffer);
        }
        ansi_cel ();
    }
    else
    {
        ansi_out (buffer);
    }
}

/* ------------------------------------------------------------- */

void video_update1 (int *forced)
{
    // check for detached state
    if (backgrounded && (tcgetpgrp(STDIN_FD) == getpid()))
    {
        *forced = 1;
        backgrounded = 0;
        tcsetattr (STDIN_FD, TCSADRAIN, &tio);
    }
}

#ifndef FLY_XEMX
/* ------------------------------------------------------------- */

void beep2 (int duration, int frequency)
{
    ansi_out ("\a");
    if (!USE_DIRECT_WRITE) fflush (stdout);
}

#endif // FLY_XEMX

/* ------------------------------------------------------------- */

char *get_clipboard (void)
{
    if (clipboard_text == NULL) return NULL;
    return strdup (clipboard_text);
}

/* ------------------------------------------------------------- */

void put_clipboard (char *s)
{
    if (clipboard_text != NULL) free (clipboard_text);
    clipboard_text = strdup (s); 
}

/* ------------------------------------------------------------- */

static void sig_interrupt (int sig)
{
    tcsetattr (STDIN_FD, TCSADRAIN, &saved_tio);
    set_cursor (1);
    ansi_fullclear ();
    if (!USE_DIRECT_WRITE) fflush (stdout);
    exit (1);
}

/* -------------------------------------------------------------
 Ctrl-Z
 backgrounded: 0 -> 1
 */

static void sig_stopped (int sig)
{
    backgrounded = 1;
    ansi_fullclear ();
    set_cursor (1);
    
    if (fl_opt.mouse_active == TRUE) ansi_mouse (FALSE);

    //fprintf (stdout, "suspended...\n");
    if (!USE_DIRECT_WRITE) fflush (stdout);

    raise (sig);
    signal2 (sig, sig_stopped);
}

/* -------------------------------------------------------------
 1) stopped -> foreground (Ctrl-Z, fg):
 foreground: 0
 background: 1 -> 0
 */

static void sig_resumed (int sig)
{
    int foreground;

    foreground = (tcgetpgrp(STDIN_FD) == getpid());

    if (!foreground) backgrounded = 1;
    if (backgrounded && foreground)
    {
        // from stopped to foreground
        backgrounded = 0;
        set_cursor (video_cursor_state (-1));
        tcsetattr (STDIN_FD, TCSADRAIN, &tio);
        if (fl_opt.mouse_active == TRUE) ansi_mouse (TRUE);
        video_update (1);
    }
    
    raise (sig);
    signal2 (sig, sig_resumed);
}

/* ------------------------------------------------------------- */

sighandler_t2 signal2 (int signum, sighandler_t2 handler)
{
    struct sigaction sa1, sa2;

    sa1.sa_handler = handler;
    sigemptyset (&sa1.sa_mask);
    sa1.sa_flags   = SA_RESETHAND | SA_NODEFER;

    if (sigaction (signum, &sa1, &sa2) < 0)
        return SIG_ERR;

    return sa2.sa_handler;
}

/* ------------------------------------------------------------- 
 note: SIG_WINCH produces char 236
 */

static void sig_winch (int sig)
{
    struct winsize   ws;
    int              rows, cols;
    char             *p;
    
    /* determine terminal dimensions */
    rows = 0;
    cols = 0;
    
    if (ioctl (STDIN_FD, TIOCGWINSZ, &ws) == 0)
    {
        rows = ws.ws_row;
        cols = ws.ws_col;
    }
    else
    {
        p = getenv ("LINES");
        if (p != NULL) rows = atoi (p);
        p = getenv ("COLUMNS");
        if (p != NULL) cols = atoi (p);
    }
    
    // fallback values
    if (rows == 0) rows = 24;
    if (cols == 0) cols = 80;

    //debug_tools ("got sigwinch; new dimensions are %d lines, %d columns\n", rows, cols);
    video_init (rows, cols);
    
    signal2 (sig, sig_winch);
}

/* -------------------------------------------------------------
 reads key definitions from /etc/termcap. returns 0 if error, 1
 if all entries are found, 2 if some are missing.
 YE (boolean) - does CR after printing in last column
 vi (string)  - cursor invisible
 ve (string)  - cursor normal
 */

static struct
{
    char *keystr;
    int  key;
}
termcap_keys[] =
{
    {"k1", _F1}, {"k2", _F2},  {"k3", _F3},  {"k4", _F4},
    {"k5", _F5}, {"k6", _F6},  {"k7", _F7},  {"k8", _F8},
    {"k9", _F9}, {"k;", _F10}, {"F1", _F11}, {"F2", _F12},

    {"ku", _Up},   {"kd", _Down}, {"kl", _Left}, {"kr", _Right},
    {"kh", _Home}, {"@7", _End},  {"kN", _PgDn}, {"kP", _PgUp},

    {"kI", _Insert}, {"kD", _Delete}
};


static void setup_termcap (void)
{
    char bp[8192], value[1024], *p, *term;
    int i, j;
    
    // set up defaults
    can_print_rd_corner = FALSE;
    cmd_hide_cursor = NULL;
    cmd_show_cursor = NULL;
    cmd_ceol = "\x1b[K";

    if (!fl_opt.use_termcap) return;

    // extract TERM variable
    term = getenv ("TERM");
    if (term == NULL)
    {
        fprintf (stderr, "warning: TERM variable is not set\n");
        return;
    }

    // get terminal entry from termcap
    if (tgetent (bp, term) != 1)
    {
        fprintf (stderr, "warning: terminal `%s' is not in termcap\n", term);
        return;
    }

    // get keys from termcap and put them into TransTable2
    for (i=0; i<sizeof(termcap_keys)/sizeof(termcap_keys[0]); i++)
    {
        p = value;
        memset (value, '\0', sizeof (value));
        p = tgetstr (termcap_keys[i].keystr, &p);
        if (p == NULL) continue;
        for (j=0; j<sizeof(TransTable2)/sizeof(TransTable2[0]); j++)
        {
            if (TransTable2[j].asv_key == termcap_keys[i].key)
            {
                TransTable2[j].curses_key = string2key (value);
                break;
            }
        }
    }

    // determine whether we can print in last column of last row
    can_print_rd_corner = !(tgetflag ("YE") | tgetflag("am"));
    
    // disable cursor
    p = value;
    p = tgetstr ("vi", &p);
    if (p != NULL) cmd_hide_cursor = strdup (value);
    
    // enable cursor
    p = value;
    p = tgetstr ("ve", &p);
    if (p != NULL) cmd_show_cursor = strdup (value);

    // clear to the end of line
    p = value;
    p = tgetstr ("ce", &p);
    if (p != NULL) cmd_ceol = strdup (value);
}

/* ------------------------------------------------------------- */

static unsigned long long string2key (char *str)
{
    unsigned long long k = 0LL;
    int  i;
    
    for (i=0; i<8; i++)
    {
        if (str[i] == 0) break;
        k += ((unsigned long long)(str[i])) << (i*8);
    }
    return k;
}

/* ------------------------------------------------------------- */

void fly_launch (char *command, int wait, int pause)
{
    char  *cmd, *p;
    
    if (wait)
    {
        tcsetattr (STDIN_FD, TCSADRAIN, &saved_tio);
        set_cursor (1);
        ansi_fullclear ();
        if (!USE_DIRECT_WRITE) fflush (stdout);

        signal  (SIGINT, SIG_IGN);
        p = get_window_name ();
        set_window_name ("running %s...", command);
        system (command);
        if (pause)
        {
            fprintf (stderr, "\nPress ENTER...");
            fgetc (stdin);
        }
        signal  (SIGINT, sig_interrupt);
        
        tcsetattr (STDIN_FD, TCSADRAIN, &tio);
        set_cursor (video_cursor_state (-1));
        video_update (1);
    }
    else
    {
        cmd = malloc (strlen (command)+20);
        strcpy (cmd, command);
        strcat (cmd, " 2>/dev/null 1>&2 &");
        system (cmd);
        free (cmd);
    }
}

/* ------------------------------------------------------------- */

static void ansi_out (char *s)
{
    int len = strlen (s);
    
    if (USE_DIRECT_WRITE)
        write (STDOUT_FD, s, len);
    else
        fputs (s, stdout);

    if (s[0] == '\x1b')
    {
        count_ctrl += len;
    }
    else
    {
        count_plain += len;
        current_col += len;
    }
    //debug_tools ("output: \"%s\"\n", s);
}

/* ------------------------------------------------------------- */

static void ansi_move_cursor (int row, int col)
{
    char buffer[32];
    
    if (backgrounded) return;
    if (current_row == row && current_col == col) return;
    
    snprintf1 (buffer, sizeof(buffer), "\x1b[%d;%dH", row+1, col+1);
    ansi_out (buffer);
    current_row = row;
    current_col = col;
}

/* ------------------------------------------------------------- */

static void ansi_fullclear (void)
{
    ansi_out ("\x1b[0m\x1b[2J");
    current_row = -1;
    current_col = -1;
    current_attribute = -1;
}

/* ------------------------------------------------------------- */

static void ansi_cel (void)
{
    ansi_out ("\x1b[K"); // clear to the end of line
    current_row = -1, current_col = -1;
}

/* ------------------------------------------------------------- */

static void ansi_set_attributes (int a)
{
    int  f, b, i;
    char buffer[32], *p;

    if (backgrounded) return;

    if (current_attribute == a) return;
    
    if (mono)
    {
        switch (a)
        {
        case VID_REVERSE:
            p = "\x1b[m\x1b[7m"; break;
        case VID_BRIGHT:
            p = "\x1b[m\x1b[1m"; break;
        case VID_NORMAL:
        default:
            p = "\x1b[m";
        }
        ansi_out (p);
    }
    else
    {
        f = 30 + pc2ansi [a      & 0x07];
        b = 40 + pc2ansi [(a>>4) & 0x07];
        i = (a & 0x08) ? 1 : 0;

        snprintf1 (buffer, sizeof(buffer), "\x1b[0;%d;%d;%dm", i, b, f);
        ansi_out (buffer);
    }
    current_attribute = a;
}

/* ------------------------------------------------------------- */

static void ansi_mouse (int flag)
{
    if (flag)
        ansi_out ("\033[?1000h");
    else
        ansi_out ("\033[?1000l");
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
    return 1;
}

/* ------------------------------------------------------------- */

int fly_set_font (char *fontname)
{
    return 1;
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

