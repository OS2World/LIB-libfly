/* ---------------------------------------------------------------------
 FLY: user-visible part
 ----------------------------------------------------------------------- */
#ifndef FLY_H_INCLUDED
#define FLY_H_INCLUDED

#include <asvtools.h>
#include "types.h"
#include "messages.h"

/* ---------------------------------------------------------------------
 options. see fly_types.h for explanations
 ----------------------------------------------------------------------- */
extern fly_options fl_opt;
extern fly_colours fl_clr;
extern fly_symbols fl_sym;
extern fly_strings fl_str;

/* ---------------------------------------------------------------------
 this function must be called before anything else. it sets some internal
 variables; it does not initialize FLY subsystem.
 ----------------------------------------------------------------------- */
int  fly_main (int argc, char *argv[], char *envp[]);
void fly_initialize (void);
void fly_process_args (int *argc, char **argv[], char ***envp);

/* ---------------------------------------------------------------------
 these functions must be called after the start and before exit.
 fly_init() performs necessary initializations and clears screen, making
 video_*() calls available. fly_terminate() stops FLY and returns screen
 to its state which was before fly_init (on Unix, the screen contents
 isn't preserved. fly_terminate() just clears the screen).
 ----------------------------------------------------------------------- */
void fly_init (int x0, int y0, int rows, int cols, char *font);
void fly_terminate (void);

/* ---------------------------------------------------------------------
 changes mode of operations from colour to mono (sets colours also)
 ----------------------------------------------------------------------- */
void fly_usemono (void);

/* ---------------------------------------------------------------------
 enables/disables mouse. set 'enabled' to 1 to start receiving mouse
 messages. On OS/2 fullscreen sessions, it also enables mouse cursor.
 ----------------------------------------------------------------------- */
void fly_mouse (int enabled);

/* ---------------------------------------------------------------------
 fly_ask produces a dialog box with text and buttons. Text is made by
 printf() with 'format' and whatever arguments are specified after 'answers'.
 '\n' inside text cause line breaks thus providing primitive formatting
 capabilities. 'answers' must contain button texts separated by '\n'.
 When 'answers' is NULL, single "OK" button is used.
 Function returns 0 if cancelled, or number of answer (counting from 1)
 if one of them is selected. Flag ASK_YN produces two buttons named "Yes"
 and "No". As a special feature, in this case selecting "No" returns 0
 instead of 2, thus "No" becomes equivalent to hitting Esc. Examples:
 choice = fly_ask (0, " Would you like \n to know the truth about %s? ",
 " I want truth \n I want lie \n Leave me alone ", "Lewinski");
 choice = fly_ask (ASK_WARN|ASK_YN, "    Are you sure???   ", NULL);
 ----------------------------------------------------------------------- */
#define ASK_WARN                1  // display dialog box in warning colour
#define ASK_YN                  2  // supply "Yes" and "No" buttons

int fly_ask (int flags, char *format, char *answers, ...);
int fly_ask_ok (int flags, char *format, ...);
int fly_ask_ok_cancel (int flags, char *format, ...);

/* ---------------------------------------------------------------------
 entry field. this is raw form: you provide everything including position
 and attribute. Note that not all platforms support Shift-Tab (Unixes don't).
 ----------------------------------------------------------------------- */
/* return values */
#define PRESSED_ESC      1
#define PRESSED_ENTER    2
#define PRESSED_TAB      3
#define PRESSED_SHIFTTAB 4

/* ---------------------------------------------------------------------
 user interface widgets: entry field
 ----------------------------------------------------------------------- */
void fly_drawframe (int r1, int c1, int r2, int c2);
void fly_shadow (int r1, int c1, int r2, int c2);
void fly_scroll_it (int key, int *first, int *current, int n, int page);

/* ---------------------------------------------------------------------
 user interface widgets: fly_choose
 this is a powerful selection box.
 ----------------------------------------------------------------------- */
/* flags for choose */
#define RC_CHOOSE_CANCELLED    -1

#define CHOOSE_ALLOW_HSCROLL   0x0001
#define CHOOSE_LEFT_IS_ESC     0x0002
#define CHOOSE_RIGHT_IS_ENTER  0x0004
#define CHOOSE_SHIFT_LEFT      0x0008
#define CHOOSE_SHIFT_RIGHT     0x0010

int fly_choose (char *header, int flags, int *nchoices, int startpos,
                void (*drawline)(int n, int len, int shift, int pointer, int row, int col),
                int  (*callback)(int *nchoices, int *n, int key));

/* ---------------------------------------------------------------------
 starts another program. 'wait' determines whether to wait for its
 completion, and when 'pause' is not 0, FLY will display message
 like "press enter to continue" and wait for user input. Some systems
 ignore 'pause' parameter (e.g., os2pm target). When 'wait' is 0,
 'pause' is ignored.
 ----------------------------------------------------------------------- */
void fly_launch (char *command, int wait, int pause);

/* ---------------------------------------------------------------------
 produces error message on console
 ----------------------------------------------------------------------- */
void fly_error (char *format, ...);

/* ---------------------------------------------------------------------
 menu functions
 ----------------------------------------------------------------------- */
#define ITEM_TYPE_ACTION     0
#define ITEM_TYPE_SWITCH     1
#define ITEM_TYPE_SUBMENU    2
#define ITEM_TYPE_SEP        3
#define ITEM_TYPE_END        4

#define MENU_TYPE_HORIZONTAL 1
#define MENU_TYPE_VERTICAL   2

#define MENU_RC_CANCELLED       -2
#define MENU_RC_NEXT_FIELD      -3
#define MENU_RC_PREV_FIELD      -4
#define MENU_RC_CANCELLED2      -5
#define MENU_RC_MOUSE_EXIT      -6

struct _item *menu_load (char *menu_filename, int *keytable, int n_keytable);
void menu_unload (struct _item *L);
int  menu_process_line (struct _item *L, int init, int opendrop);
int  menu_check_key (struct _item *L, int key);
void menu_chstatus (int actions[], int n, int newstatus);
void menu_set_switch (int action, int newstate);
void menu_activate (struct _item *L);
void menu_deactivate (struct _item *L);
void menu_enable (void);
void menu_disable (void);

/* ---------------------------------------------------------------------
 keyboard functions
 ----------------------------------------------------------------------- */
int    fly_keyvalue (char *s);
char **fly_keyname (int key);
char  *fly_keyname2 (int key);

/* ---------------------------------------------------------------------
 copy to buffer / restore to part of the screen characters with their
 attributes. video_save() returns pointed to malloc()ed buffer,
 video_restore will free it (`s' is pointer to buffer).
 Rectangle to save has (row1,col1) -> (row2,col2) coordinates.
 Do need video_update().
 ----------------------------------------------------------------------- */
char *video_save   (int row1, int col1, int row2, int col2);
void video_restore (char *s);

/* ---------------------------------------------------------------------
 this function causes real screen update, i.e. all screen-altering calls
 actually just draw somewhere in the off-screen buffer.
 ----------------------------------------------------------------------- */
void video_update (int forced);

/* ---------------------------------------------------------------------
 Needs video_update().
 ----------------------------------------------------------------------- */
void video_put_n_char (char c, int n, int row, int col);

/* ---------------------------------------------------------------------
 Needs video_update().
 ----------------------------------------------------------------------- */
void video_put_n_attr (char a, int n, int row, int col);

/* ---------------------------------------------------------------------
 Needs video_update().
 ----------------------------------------------------------------------- */
void video_put_str (char *s, int n, int row, int col);

/* ---------------------------------------------------------------------
 Needs video_update().
 ----------------------------------------------------------------------- */
void video_put_str_attr (char *s, int n, int row, int col, char a);

/* ---------------------------------------------------------------------
 fills a row of positions starting at (row, col) with specified
 char/attr. char is c[0], attr is c[1]. `n' is number of positions to
 write. behaviour at the end of the line is undefined (it might wrap to next
 line or might not).
 Needs video_update().
 ----------------------------------------------------------------------- */
void video_put_n_cell (char c, char a, int n, int row, int col);

/* ---------------------------------------------------------------------
 fills rectangle with specified char/attr. upper left corner is at
 (row1, col1), lower right -- (row2, col2). these points are inclusive.
 there's no error return (when values are invalid, only part of the
 screen may be cleared, or nothing at all). char is c[0], attr is c[1]
 Needs video_update().
 ----------------------------------------------------------------------- */
void video_clear (int row1, int col1, int row2, int col2, char c);

/* ---------------------------------------------------------------------
 moves hardware cursor / reads hardware cursor position. Upper left corner
 is (0,0), lower right -- (79, 24) (for 80x25 screen). There's no error
 return.
 Need video_update().
 ----------------------------------------------------------------------- */
void video_set_cursor (int row, int col);
void video_get_cursor (int *row, int *col);

/* ---------------------------------------------------------------------
 return screen size in rows/columns. Typical values are
 80 x 25. There's no error return.
 ----------------------------------------------------------------------- */
int video_vsize (void);
int video_hsize (void);

/* ---------------------------------------------------------------------
 sets window size, in rows/columns. returns 1 if error, 0 if ok
 ----------------------------------------------------------------------- */
int video_set_window_size (int rows, int cols);

/* ---------------------------------------------------------------------
 retrieve/set window position. get_window_pos returns 1 when error occurs
 ----------------------------------------------------------------------- */
int  video_get_window_pos (int *x0, int *y0);
void video_set_window_pos (int x0, int y0);

/* ---------------------------------------------------------------------
 puts null-terminated string at specified location
 ----------------------------------------------------------------------- */
void video_put (char *s, int row, int col);

/* ---------------------------------------------------------------------
 hides(flag = 0) / shows(flag = 1) hardware cursor. Returns previous
 cursor state. There's no error return. when flag is -1, simply return
 current state and do not change it
 ----------------------------------------------------------------------- */
int video_cursor_state (int flag);

/* ---------------------------------------------------------------------
 these functions return the character and attributes at the given position
 ----------------------------------------------------------------------- */
char video_char (int row, int col);
char video_attr (int row, int col);

/* ---------------------------------------------------------------------
 waits for keystroke during `interval' milliseconds (or indefinitely with
 interval=-1). If no keys were pressed, returns 0, otherwise key code
 (see keydefs.h). On some systems, some keys do not generate recognizable
 codes and therefore ignored.
 ----------------------------------------------------------------------- */
int getkey (int interval);
int getmessage (int interval);

/* ---------------------------------------------------------------------
 changes current drive to `drive'+'A', i.e. A: -- 0, B: -- 1, etc.
 When error occurs, returns -1; otherwise 0. When system does not have
 drive letters, always returns 0 (no error).
 ----------------------------------------------------------------------- */
int change_drive (int drive);

/* ---------------------------------------------------------------------
 returns current drive. A: -- 0, B: -- 1, etc. Should return something
 (2?) even when system does not have drive letters. There's no error
 return.
 ----------------------------------------------------------------------- */
int query_drive (void);

/* ---------------------------------------------------------------------
 returns long integer; its bits show whether corresponding drive is
 present in the system. Bit 0 is for drive A:, 1 -- B: etc. Should return
 at least one drive even when system does not have drive letters. There's
 no error return.
 ----------------------------------------------------------------------- */
unsigned long query_drivemap (void);

/* ---------------------------------------------------------------------
 issues audible beep via speaker with `frequency' Hz frequency and of
 'duration' milliseconds. Under some systems, frequency and duration are
 ignored. There's no return value.
 ----------------------------------------------------------------------- */
void beep2 (int frequency, int duration);

/* ---------------------------------------------------------------------
 set_window_name tries to set process name. Syntax is the same as printf,
 i.e. with format string and arguments. There's no return value (errors
 are ignored). get_window_name() returns malloc()ed buffer with current
 name. Bogus malloc()ed string is returned when cannot guess a title.
 ----------------------------------------------------------------------- */
char *get_window_name (void);
void  set_window_name (char *format, ...);

/* ---------------------------------------------------------------------
 attaches icon to the current process (where applicable). 'filename' must
 point at icon file in the native format (currently, only OS/2 is supported)
 ----------------------------------------------------------------------- */
void set_icon (char *filename);

/* ---------------------------------------------------------------------
 get/set_font_size query/change display font. fly_get_font() returns malloc()ed
 buffer with font name suitable for later feeding into fly_set_font (and
 is the same as returned by fly_get_fontlist() in F[].name). fly_set_font()
 returns 0 when OK, 1 if error. some platforms do not allow to change font
 or query it (bogus malloc()ed value is returned)
 ----------------------------------------------------------------------- */
char *fly_get_font (void);
int  fly_set_font (char *fontname);
int  fly_get_fontlist (flyfont **F, int restrict_by_language);

/* ---------------------------------------------------------------------
 extract string from clipboard / put string into clipboard.
 String is NULL-terminated. get_clipboard() returns pointer
 to malloc()ed buffer or NULL if failed. There's no return
 value for put_clipboard().
 ----------------------------------------------------------------------- */
char *get_clipboard (void);
void put_clipboard (char *text);

#endif
