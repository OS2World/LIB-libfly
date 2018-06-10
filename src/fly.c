#include "fly/fly.h"
#include "fly-internal.h"

#include <manycode.h>
#include <stdarg.h>

fly_options fl_opt = {0};
fly_colours fl_clr = {0};
fly_symbols fl_sym = {0};
fly_strings fl_str = {NULL};

/* -------------------------------------------------------------------- */

void fly_initialize (void)
{
    /* -----------------------------------------------------------
     Options
     ------------------------------------------------------------- */

    fl_opt.appname = "FLY";
    fl_opt.initialized = FALSE;
    fl_opt.mono = FALSE;
    fl_opt.use_termcap = TRUE;
    fl_opt.mouse_nb = 2;
    fl_opt.sb_width = 2;
    fl_opt.mouse_singleclick = 300;
    fl_opt.mouse_doubleclick = 300;
    fl_opt.mouse_autorepeat = 300;
    fl_opt.mouse_autodelay = 1000;
    fl_opt.use_ceol = FALSE;
    fl_opt.menu_hotmouse = TRUE;
    fl_opt.shadows = TRUE;
    fl_opt.use_gui_controls = TRUE;

#ifdef FLY_UNIX_TERM
    fl_opt.platform = PLATFORM_UNIX_TERM;
    fl_opt.is_unix  = TRUE;
    fl_opt.has_osmenu  = FALSE;
    fl_opt.has_console = TRUE;
    fl_opt.platform_name = "Unix terminal";
    fl_opt.platform_nick = "unixterm";
    fl_opt.is_x11 = FALSE;
    fl_opt.has_clipboard = FALSE;
    fl_opt.has_driveletters = FALSE;
    fl_opt.mouse_active = FALSE;
    fl_opt.charset = EN_KOI8R;
#endif

#ifdef FLY_UNIX_X11
    fl_opt.platform = PLATFORM_UNIX_X11;
    fl_opt.is_unix  = TRUE;
    fl_opt.has_osmenu  = FALSE;
    fl_opt.has_console = TRUE;
    fl_opt.platform_name = "X11 Unix";
    fl_opt.platform_nick = "unix_X11";
    fl_opt.is_x11 = TRUE;
    fl_opt.has_clipboard = TRUE;
    fl_opt.has_driveletters = FALSE;
    fl_opt.mouse_active = TRUE;
    fl_opt.charset = EN_KOI8R;
#endif

#ifdef FLY_BEOS_TERM
    fl_opt.platform = PLATFORM_BEOS_TERM;
    fl_opt.is_unix  = TRUE;
    fl_opt.has_osmenu  = FALSE;
    fl_opt.has_console = TRUE;
    fl_opt.platform_name = "BeOS Terminal";
    fl_opt.platform_nick = "beosterm";
    fl_opt.is_x11 = FALSE;
    fl_opt.has_clipboard = FALSE;
    fl_opt.has_driveletters = FALSE;
    fl_opt.mouse_active = FALSE;
    fl_opt.charset = EN_ISO;
#endif

#ifdef FLY_EMX_VIO
    fl_opt.platform = PLATFORM_OS2_VIO;
    fl_opt.is_unix  = FALSE;
    fl_opt.has_osmenu  = FALSE;
    fl_opt.has_console = TRUE;
    fl_opt.platform_name = "OS/2 VIO session";
    fl_opt.platform_nick = "os2vio";
    fl_opt.is_x11 = FALSE;
    fl_opt.has_clipboard = TRUE;
    fl_opt.has_driveletters = TRUE;
    fl_opt.mouse_active = TRUE;
    fl_opt.charset = EN_ALT;
#endif

#ifdef FLY_EMX_PM
    fl_opt.platform = PLATFORM_OS2_PM;
    fl_opt.is_unix  = FALSE;
    fl_opt.has_osmenu  = TRUE;
    fl_opt.has_console = FALSE;
    fl_opt.platform_name = "OS/2 Presentation Manager";
    fl_opt.platform_nick = "os2pm";
    fl_opt.is_x11 = FALSE;
    fl_opt.has_clipboard = TRUE;
    fl_opt.has_driveletters = TRUE;
    fl_opt.mouse_active = TRUE;
    fl_opt.charset = EN_ALT;
#endif

#ifdef FLY_EMX_X
    fl_opt.platform = PLATFORM_OS2_X;
    fl_opt.is_unix  = FALSE;
    fl_opt.has_osmenu  = FALSE;
    fl_opt.has_console = TRUE;
    fl_opt.platform_name = "XFree86/OS2";
    fl_opt.platform_nick = "os2x";
    fl_opt.is_x11 = FALSE;
    fl_opt.has_clipboard = TRUE;
    fl_opt.has_driveletters = TRUE;
    fl_opt.mouse_active = FALSE;
    fl_opt.charset = EN_KOI8R;
#endif

#ifdef FLY_EMX_X11
    fl_opt.platform = PLATFORM_OS2_X11;
    fl_opt.is_unix  = FALSE;
    fl_opt.has_osmenu  = FALSE;
    fl_opt.has_console = TRUE;
    fl_opt.platform_name = "X11 (XFree86/OS2)";
    fl_opt.platform_nick = "os2X11";
    fl_opt.is_x11 = TRUE;
    fl_opt.has_clipboard = TRUE;
    fl_opt.has_driveletters = TRUE;
    fl_opt.mouse_active = TRUE;
    fl_opt.charset = EN_KOI8R;
#endif

#ifdef FLY_WIN32_CONS
    fl_opt.platform = PLATFORM_WIN32_CONS;
    fl_opt.is_unix  = FALSE;
    fl_opt.has_osmenu  = FALSE;
    fl_opt.has_console = TRUE;
    fl_opt.platform_name = "Windows 95/98/NT/2000 console";
    fl_opt.platform_nick = "win32cons";
    fl_opt.is_x11 = FALSE;
    fl_opt.has_clipboard = TRUE;
    fl_opt.has_driveletters = TRUE;
    fl_opt.mouse_active = TRUE;
    fl_opt.charset = EN_ALT;
#endif
    
#ifdef FLY_WIN32_GUI
    fl_opt.platform = PLATFORM_WIN32_GUI;
    fl_opt.is_unix  = FALSE;
    fl_opt.has_osmenu  = TRUE;
    fl_opt.has_console = FALSE;
    fl_opt.platform_name = "Windows 95/98/NT";
    fl_opt.platform_nick = "win32gui";
    fl_opt.is_x11 = FALSE;
    fl_opt.has_clipboard = TRUE;
    fl_opt.has_driveletters = TRUE;
    fl_opt.mouse_active = TRUE;
    fl_opt.charset = EN_ALT;
#endif

    if (fl_opt.has_osmenu)
        fl_opt.menu_onscreen = FALSE;
    else
        fl_opt.menu_onscreen = TRUE;

    fl_clr.background         = _BackBlack + _White;
    fl_clr.dbox_back          = _BackWhite + _Black;
    fl_clr.dbox_back_warn     = _BackRed + _White;
    fl_clr.dbox_sel           = _BackBlack + _High + _White;
    fl_clr.entryfield         = _BackCyan + _Black;

    fl_clr.viewer_text        = _BackBlack + _White;
    fl_clr.viewer_header      = _BackCyan + _Black;
    fl_clr.viewer_found       = _BackBlack + _High + _Red;

    fl_clr.menu_main            =  _BackWhite+_Black;
    fl_clr.menu_main_disabled   =  _BackWhite+_Blue;
    fl_clr.menu_main_hotkey     =  _BackWhite+_High+_Red;
    fl_clr.menu_cursor          =  _White;
    fl_clr.menu_cursor_hotkey   =  _High+_Green;
    fl_clr.menu_cursor_disabled =  _High+_Black;
    fl_clr.menu_cursor_disabled =  _High+_Blue;
    if (fl_opt.is_unix || fl_opt.platform == PLATFORM_OS2_X)
    {
        fl_clr.menu_cursor_disabled =  _High+_Blue;
    }

    fl_clr.selbox_back          = _BackWhite+_Black;
    fl_clr.selbox_pointer       = _BackCyan + _Black;
    
    /* -----------------------------------------------------------
     Symbols
     ------------------------------------------------------------- */
    
    if (fl_opt.is_unix || fl_opt.platform == PLATFORM_OS2_X || fl_opt.platform == PLATFORM_OS2_X11)
    {
        fl_sym.h           = '-';
        fl_sym.v           = ' ';
        fl_sym.x           = ' ';
        fl_sym.c_lu        = '-';
        fl_sym.c_ru        = '-';
        fl_sym.c_ld        = '-';
        fl_sym.c_rd        = '-';
        fl_sym.t_l         = '-';
        fl_sym.t_r         = '-';
        fl_sym.t_u         = '-';
        fl_sym.t_d         = '-';
        fl_sym.fill1       = ' ';
        fl_sym.fill2       = '.';
        fl_sym.fill3       = '+';
        fl_sym.fill4       = 'X';
        fl_sym.placeholder = ' ';
        fl_sym.arrow_down  = '+';
        fl_sym.arrow_up    = '+';
        fl_sym.marksign    = 'x';
    }
    else
    {
/*
     fill1 176 �        LD 192 �            208 �
     fill2 177 �        dt 193 �            209 �
     fill3 178 �        ut 194 �            210 �
      vbar 179 �        LT 195 �            211 �
        RT 180 �      hbar 196 �            212 �
           181 �         x 197 �            213 �
           182 �           198 �            214 �
           183 �           199 �            215 �
           184 �           200 �            216 �
           185 �           201 �         RD 217 �
     vbar2 186 �           202 �         LU 218 �
           187 �           203 �      fill4 219 �
           188 �           204 �            220 �
           189 �     hbar2 205 �            221 �
           190 �           206 �            222 �
        RU 191 �           207 �            223 �
 */

        fl_sym.h           = '�';
        fl_sym.v           = '�';
        fl_sym.x           = '�';
        fl_sym.fill1       = '�';
        fl_sym.fill2       = '�';
        fl_sym.fill2       = '�';
        fl_sym.fill4       = '�';
        fl_sym.c_lu        = '�';
        fl_sym.c_ru        = '�';
        fl_sym.c_ld        = '�';
        fl_sym.c_rd        = '�';
        fl_sym.t_l         = '�';
        fl_sym.t_r         = '�';
        fl_sym.t_u         = '�';
        fl_sym.t_d         = '�';
        fl_sym.placeholder = '�';
        fl_sym.arrow_down  = '';
        fl_sym.arrow_up    = '';
        fl_sym.marksign    = '*';
    }

    /* -----------------------------------------------------------
     Strings
     ------------------------------------------------------------- */
    
    fl_str.yes    = "  Yes  ";
    fl_str.no     = "  No  ";
    fl_str.ok     = "  Ok  ";
    fl_str.cancel = "  Cancel  ";
}

/* -------------------------------------------------------------------- */

void fly_usemono (void)
{
    fl_opt.mono = TRUE;
    
    fl_clr.dbox_back          = VID_REVERSE;
    fl_clr.dbox_back_warn     = VID_REVERSE;
    fl_clr.dbox_sel           = VID_BRIGHT;

    fl_clr.entryfield         = VID_NORMAL;

    fl_clr.viewer_text        = VID_NORMAL;
    fl_clr.viewer_header      = VID_REVERSE;
    fl_clr.viewer_found       = VID_BRIGHT;

    fl_clr.menu_main            =  VID_REVERSE;
    fl_clr.menu_main_disabled   =  VID_REVERSE;
    fl_clr.menu_main_hotkey     =  VID_BRIGHT;
    fl_clr.menu_cursor          =  VID_NORMAL;
    fl_clr.menu_cursor_hotkey   =  VID_BRIGHT;
    fl_clr.menu_cursor_disabled =  VID_NORMAL;
    
    fl_clr.selbox_back          = VID_REVERSE;
    fl_clr.selbox_pointer       = VID_NORMAL;
}
