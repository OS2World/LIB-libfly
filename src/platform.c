#include "fly/fly.h"
#include "fly-internal.h"

/* --------------------------------------------------------------------- */

#ifdef FLY_UNIX_TERM
#include "unix.c"
#ifdef __linux__
#include "kbd_select.c"
#else
#include "kbd_poll.c"
#endif
#include "setproctitle_unix.c"
#include "drives_no.c"
#include "menu_text.c"
#endif

#ifdef FLY_UNIX_X11
#include "x11.c"
#include "drives_no.c"
#include "menu_text.c"
#endif

#ifdef FLY_BEOS_TERM
#include "unix.c"
#include "kbd_poll.c"
#include "setproctitle_no.c"
#include "drives_no.c"
#include "menu_text.c"
#endif

#ifdef FLY_EMX_VIO
#include "os2vio.c"
#include "drives_os2.c"
#include "menu_text.c"
#endif

#ifdef FLY_EMX_PM
#include "os2pm.c"
#include "drives_os2.c"
#endif

#ifdef FLY_EMX_X
#include "unix.c"
#include "kbd_select.c"
#include "drives_os2.c"
#include "setproctitle_no.c"
#include "menu_text.c"
#endif

#ifdef FLY_EMX_X11
#include "x11.c"
#include "drives_os2.c"
#include "menu_text.c"
#endif

#ifdef FLY_WIN32_CONS
#include "win32.c"
#include "drives_win32.c"
#include "menu_text.c"
#endif

#ifdef FLY_WIN32_GUI
#include "win32gui.c"
#include "drives_win32.c"
#endif

/* --------------------------------------------------------------- 
   ...some additional defines for some braindead OSes...
   --------------------------------------------------------------- */

#ifdef FLY_EMX_X
        #define SIGTSTP  19
        #define SIGCONT  20
        #define SIGWINCH 28
        
        #define ONLCR    0
	#define ECHOCTL  0

        #define SA_RESETHAND SA_SYSV
        #define SA_NODEFER 0

        #define TIOCGWINSZ  0x5413
        struct winsize
        {
           unsigned short ws_row;
           unsigned short ws_col;
           unsigned short ws_xpixel;
           unsigned short ws_ypixel;
	};                               

        #define _POSIX_VDISABLE 0xff
#endif

#ifdef FLY_BEOS_TERM
    #if !defined(ECHOCTL)
	   #define ECHOCTL 0
	#endif
	#if !defined(SA_RESETHAND)
	   #define SA_RESETHAND 0x80000000
	#endif
	#if !defined(SA_NODEFER)
	   #define SA_NODEFER 0x40000000
	#endif
#endif

#ifdef __bsdi__
	#define SA_RESETHAND 0x0
	#define SA_NODEFER 0x0
#endif


