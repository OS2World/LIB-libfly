#ifndef FLY_TYPES_H_INCLUDED
#define FLY_TYPES_H_INCLUDED

#define FLY_APP_FRAMEWND        1

/* ---------------------------------------------------------------------
 platform codes
 ----------------------------------------------------------------------- */
#define PLATFORM_OS2_VIO      1
#define PLATFORM_OS2_PM       2
#define PLATFORM_OS2_X        3
#define PLATFORM_OS2_X11      4
#define PLATFORM_BEOS_TERM   10
#define PLATFORM_UNIX_TERM   20
#define PLATFORM_UNIX_X11    21
#define PLATFORM_WIN32_CONS  30
#define PLATFORM_WIN32_GUI   31

/* ---------------------------------------------------------------------
 screen colours (PC-style)
 ----------------------------------------------------------------------- */

#define _Black       0
#define _Blue        1
#define _Green       2
#define _Cyan        _Blue+_Green
#define _Red         4
#define _Magenta     _Red+_Blue
#define _Brown       _Red+_Green
#define _White       _Red+_Green+_Blue

#define _High        8

#define _BackBlack   0
#define _BackBlue    16
#define _BackGreen   32
#define _BackCyan    _BackBlue+_BackGreen
#define _BackRed     64
#define _BackMagenta _BackRed+_BackBlue
#define _BackBrown   _BackRed+_BackGreen
#define _BackWhite   _BackRed+_BackGreen+_BackBlue

#define _Blink       128

#define VID_NORMAL  (_BackBlack + _White)
#define VID_REVERSE (_BackWhite + _Black)
#define VID_BRIGHT  (_BackBlack + _High + _White)

/* ---------------------------------------------------------------------
 type definitions
 ----------------------------------------------------------------------- */

union _item_body
{
    int          action;
    struct _item *submenu;
    int          *checkbox;
};

struct _item
{
    char *text;            /* Menu item text, without hotkey name */
    char  type;            /* type: submenu/checkbox/action */
    union _item_body body; /* Body of menu item */
    int   submenu_id;      /* internally used for dynamically created submenus */
    char  *keydesc;        /* key name */
    int   pos;             /* position; computed at run-time */
    char  enabled;         /* active item? */
    char  state;           /* state for switches */
    char  hotkey, hkpos;   /* hotkey and its position inside the text */
};

typedef struct
{
    int   cx, cy;        // cell dimensions
    char  *name;         // user-readable font name
    void  *signature;    // used internally for font identification
}
flyfont;

typedef struct
{
    int   initialized; // whether FLY active or not
    
    char  *appname; // short application name
    char  *platform_name; // platform name
    char  *platform_nick; // short platform name

    int   mono; // set to true to use monochrome displays
    int   use_termcap; // whether to pull key definitions from termcap
    int   mouse_active; // whether to use mouse
    int   mouse_nb; // number of mouse buttons
    int   platform; // see PLATFORM_... codes above
    int   sb_width; // scrollbar width (can be 1 or 2)
    int   charset; // default charset for display
    int   mouse_doubleclick; // interval in milliseconds between mouse clicks which are
                             // to be interpreted as double click
    int   mouse_singleclick; // interval in milliseconds between mouse down/up which are
                             // to be interpreted as single click
    int   mouse_autorepeat; // interval in milliseconds after which automatic mouse messages
                            // will be generated when button is pressed
    int   mouse_autodelay; // interval in milliseconds between automatically generated messages
    int   menu_hotmouse; // whether to enter menu when mouse pointer hovers over menu line
    int   menu_onscreen; // whether menu is shown on screen
    int   shadows; // should dialog boxes cast shadows
    
    int   use_ceol; // whether to use clear-to-the-end-of-line (might cause screen corruption
    int   use_gui_controls; // whether to use GUI control (dialog boxes)
    
    int   is_unix; // when true, system is UNIX-like (no drive letters etc.)
    int   is_x11; // whether we are working under X11

    int   has_console; // whether true textmode input/output is available before starting FLY
    int   has_clipboard; // whether it supports system-wide clipboard
    int   has_osmenu; // OS provides menu?
    int   has_driveletters; // OS has drive letters?
}
fly_options;

typedef struct
{
    char  background;
    
    char  menu_main;
    char  menu_main_disabled;
    char  menu_main_hotkey;
    char  menu_cursor;
    char  menu_cursor_hotkey;
    char  menu_cursor_disabled;
    
    char  dbox_back;
    char  dbox_back_warn;
    char  dbox_sel;

    char  selbox_back;
    char  selbox_pointer;
    
    char  entryfield;
    
    char viewer_text;
    char viewer_header;
    char viewer_found;
}
fly_colours;

typedef struct
{
    char h, v, x, c_lu, c_ru, c_ld, c_rd, t_l, t_r, t_u, t_d;
    char placeholder, arrow_down, arrow_up, marksign;
    char fill1, fill2, fill3, fill4;
}
fly_symbols;

typedef struct
{
    char *yes, *no, *ok, *cancel;
}
fly_strings;

#endif
