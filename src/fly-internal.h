
struct m_event
{
    int             r, c; // row, column
    int             b1, b2, b3; // button state
    unsigned long   t; // event time, in msec
    int             ev; // assigned event
};

extern int   backgrounded;
extern struct _item  *fly_active_menu;

/* ---------------------------------------------------------------------
 must be called before using video_* functions. Initializes structures,
 and draws initial screen
 ----------------------------------------------------------------------- */
void video_init (int rows, int cols);

/* ---------------------------------------------------------------------
 must be called after using video_* functions. Performs cleanups like
 freeing memory etc. Does not draw anything
 ----------------------------------------------------------------------- */
void video_terminate (void);

/* ---------------------------------------------------------------------
 intermediate update which is called before actual. Needed for Unix to
 track detached state
 ----------------------------------------------------------------------- */
void video_update1 (int *forced);

/* ---------------------------------------------------------------------
 final flushing the output (fflush() on Unix, UpdateWindow on Win32gui)
 ----------------------------------------------------------------------- */
void video_flush (void);

/* ---------------------------------------------------------------------
 perform setup for setting window title (needed for Unix systems)
 ----------------------------------------------------------------------- */
void initsetproctitle (int argc, char **argv, char **envp);

/* ---------------------------------------------------------------------
 move cursor to specified position
 ----------------------------------------------------------------------- */
void move_cursor (int row, int col);

/* ---------------------------------------------------------------------
 put a set of chars of specified length with specified attribute at
 specified location
 ----------------------------------------------------------------------- */
void screen_draw (char *s, int len, int row, int col, char *a);

/* ---------------------------------------------------------------------
 forced redraw of specified region
 ----------------------------------------------------------------------- */
void video_update_rect (int r1, int c1, int r2, int c2);

int ifly_ask (int flags, char *questions, char *answers);

void mouse_received (int b1, int b2, int b3, int t, int r, int c);
int  mouse_check (int t);

void set_cursor (int state);

void ifly_error_gui (char *message);
void ifly_error_txt (char *message);

int  ifly_ask_gui (int flags, char *questions, char *answers);
int  ifly_ask_txt (int flags, char *questions, char *answers);

