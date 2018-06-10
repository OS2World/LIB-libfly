#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>

#ifdef __EMX__
#define INCL_DOS
#include <os2.h>
#define usleep(t) DosSleep (t/1000)
#endif

#include "pc2rgb.h"

/* To do:
 + parse command-line arguments (-display, -geometry, -fn)
 - verify different visuals
 + cursor
 + launch
 - async updates
 - optimize Expose
 - keys
 + suspend?
 + get/set windowpos
 + clipboard
 + beep
 + menu: disable auto-open
 */

static Display      *x11_d;
static Window        x11_w;
static XFontStruct  *x11_f, *x11_kf;
static GC            x11_gc[256];
static GC            x11_kgc[256];
static XColor        x11_clr[16];
static Colormap      x11_cm;
static char         *x11_font = NULL;
static Atom          x11_buf;

enum {NotDBCS, Korean, Japanese, Chinese};

static int    cursor_visible = TRUE, cursor_X=0, cursor_Y=0;
static char   *cmdarg_display=NULL, *cmdarg_font=NULL, *cmdarg_geom=NULL;
static int    dbcs_lang = NotDBCS;

#define is_dbcs (dbcs_lang != NotDBCS)
#define CX    (x11_f->max_bounds.width)
#define CY    (x11_f->max_bounds.ascent + x11_f->max_bounds.descent)
#define DCX   (x11_kf->max_bounds.width)
#define DCY   (x11_kf->max_bounds.ascent + x11_kf->max_bounds.descent)

static int x_getkey (void);
static int convert_x11_key (KeySym keysym, int k_shift, int k_ctrl, int k_alt, int k_lock);
static int font_compare (__const__  void *f1, __const__  void *f2);
//static void printchars (char *hdr, char *buf, int n);
static void Xsetup_font (char *font);
static void Xsetup_colour (void);

static void SetColor (int i)
{
    x11_clr[i].blue  = (pc2rgb[i].b << 8) | pc2rgb[i].b;
    x11_clr[i].green = (pc2rgb[i].g << 8) | pc2rgb[i].g;
    x11_clr[i].red   = (pc2rgb[i].r << 8) | pc2rgb[i].r;
    x11_clr[i].flags = DoRed | DoGreen | DoBlue;
}

/* ------------------------------------------------------------- */

void fly_init (int x0, int y0, int rows, int cols, char *font)
{
    unsigned long          mask;
    char                   *p;
    XSetWindowAttributes   xa;
    XClassHint             *xh;

    p = cmdarg_display;
    if (p == NULL) p = getenv ("DISPLAY");
    if (p == NULL)
    {
        p = "localhost:0.0";
        fprintf (stderr, "DISPLAY variable not set, trying %s\n", p);
    }
    x11_d = XOpenDisplay (p);
    if (x11_d == NULL) error1 ("cannot open X11 display (%s)\n", p);

    x11_w = XCreateSimpleWindow (x11_d, RootWindow (x11_d, 0), 200, 300, 1, 1, 2, 0, 1);

    mask = ExposureMask | StructureNotifyMask | VisibilityChangeMask | PointerMotionMask |
        FocusChangeMask | KeyPressMask | ButtonPressMask | ButtonReleaseMask | ButtonMotionMask |
        LeaveWindowMask;
    XSelectInput (x11_d, x11_w, mask);

    //printf ("is_dbcs = %d\n", fl_opt.is_dbcs);
    Xsetup_font (font);
    Xsetup_colour ();
    
    xa.bit_gravity = NorthWestGravity;
    xa.backing_store = WhenMapped;
    XChangeWindowAttributes (x11_d, x11_w, CWBitGravity|CWBackingStore, &xa);

    // add WM_CLASS property
    xh = XAllocClassHint ();
    xh->res_name  = fl_opt.appname;
    p = str_strdup1 (fl_opt.appname, 2);
    strcpy (p, "X");
    strcat (p, fl_opt.appname);
    xh->res_class = p;
    XSetClassHint (x11_d, x11_w, xh);
    XFree (xh);
    free (p);

    // give hint to window manager about icon name
    //XSetWMIconName (x11_d, x11_w, fl_opt.appname);

    //printf ("requested size: %d rows x %d cols\n", rows, cols);
    if (rows != -1 && cols != -1)
        video_init (rows, cols);
    else
        video_init (25, 80);

    //printf ("requested position: x0 = %d, y0 = %d\n", x0, y0);
    if (x0 != -1 && y0 != -1)
        XMoveWindow (x11_d, x11_w, x0, y0);
    XResizeWindow (x11_d, x11_w, video_hsize() * CX, video_vsize() * CY);
    XMapWindow (x11_d, x11_w);

    x11_buf = XInternAtom (x11_d, "NFTP_clipboard", False);

    fl_opt.initialized = TRUE;
}

/* ------------------------------------------------------------- */

void fly_mouse (int enabled)
{
    // always enabled in X :-)
}

/* ------------------------------------------------------------- */

void fly_terminate (void)
{
    XDestroyWindow (x11_d, x11_w);
    XCloseDisplay (x11_d);
    fl_opt.initialized = FALSE;
}

/* ------------------------------------------------------------- */

int getmessage (int interval)
{
    int        key;
    double     timergoal, now;

    if (interval < 0) interval = -1;
    
    switch (interval)
    {
    case 0:
        now = clock1 ();
        if (fl_opt.mouse_active)
        {
            key = mouse_check ((unsigned long)(now/1000.));
            if (key > 0) return key;
        }
        while (XPending (x11_d) > 0)
        {
            key = x_getkey ();
            if (key != -1) return key;
        }
        return 0;
        
    case -1:
        while (TRUE)
        {
            now = clock1 ();
            if (fl_opt.mouse_active)
            {
                key = mouse_check ((unsigned long)(now/1000.));
                if (key > 0) return key;
            }
            key = x_getkey ();
            if (key != -1) return key;
        }

    default:
        timergoal = clock1 () + (double)interval/1000.;
        do
        {
            now = clock1 ();
            if (fl_opt.mouse_active)
            {
                key = mouse_check ((unsigned long)(now/1000.));
                if (key > 0) return key;
            }
            while (XPending (x11_d) > 0)
            {
                key = x_getkey ();
                if (key != -1) return key;
            }
            usleep (10000);
        }
        while (interval == -1 || now < timergoal);
    }
    return -1;
}

/* ------------------------------------------------------------- */

static int x_getkey (void)
{
    XEvent event;

    //XAnyEvent            *anyEvent       = (XAnyEvent *) &event;
    XExposeEvent         *exposeEvent    = (XExposeEvent *) &event;
    XButtonEvent         *buttonEvent    = (XButtonEvent *) &event;
    XKeyEvent            *keyEvent       = (XKeyEvent *) &event;
    XConfigureEvent      *configureEvent = (XConfigureEvent *) &event;
    XLeaveWindowEvent    *leaveEvent     = (XLeaveWindowEvent *) &event;
    //XGraphicsExposeEvent *gexposeEvent   = (XGraphicsExposeEvent *) &event;
    //XMotionEvent         *motionEvent    = (XMotionEvent *) &event;
    KeySym               keysym;

    int               key = -1, k_alt, k_shift, k_ctrl, k_lock;
    int               r1, c1, r2, c2, b1, b2, b3, newx, newy, r, c;
    static double     last_expose = 0.;

    XNextEvent (x11_d, &event);
    if (XFilterEvent (&event, None)) return -1;

    switch (event.type)
    {
    case Expose:
        r1 = (exposeEvent->y)/CY;
        c1 = (exposeEvent->x)/CX;
        r2 = (exposeEvent->y + exposeEvent->height)/CY;
        c2 = (exposeEvent->x + exposeEvent->width)/CX;
        //printf ("Expose: (%d, %d) -> (%d, %d)\n", r1, c1, r2, c2);
        if (r1 == 0 && c1 == 0 && r2 == video_vsize()-1 && c2 == video_hsize()-1 &&
            clock1() - last_expose < 0.5) break;
        //video_update_rect (r1, c1, r2, c2);
        video_update (1);
        if (r1 == 0 && c1 == 0 && r2 == video_vsize()-1 && c2 == video_hsize()-1)
            last_expose = clock1 ();
        break;

    case KeyPress:
        keysym = XLookupKeysym (keyEvent, 0);
        k_alt   = keyEvent->state & Mod1Mask;
        k_shift = keyEvent->state & ShiftMask;
        k_ctrl  = keyEvent->state & ControlMask;
        k_lock  = keyEvent->state & LockMask;
        //printf ("received event: %ld %lx (S%u C%u A%u L%u)\n", keysym, keysym, k_shift, k_ctrl, k_alt, k_lock);
        key = convert_x11_key (keysym, k_shift, k_ctrl, k_alt, k_lock);
        break;

    case MotionNotify:
        if (!fl_opt.mouse_active) break;
        b1 = buttonEvent->state & Button1Mask ? 1 : 0;
        b3 = buttonEvent->state & Button3Mask ? 1 : 0;
        b2 = buttonEvent->state & Button2Mask ? 1 : 0;
        mouse_received (b1, b2, b3, (int)(clock1()/1000.), buttonEvent->y/CY, buttonEvent->x/CX);
        break;
        
    case ButtonPress:
        if (!fl_opt.mouse_active) break;
        b1 = buttonEvent->button == Button1 ? 1 : (buttonEvent->state & Button1Mask ? 1 : 0);
        b3 = buttonEvent->button == Button2 ? 1 : (buttonEvent->state & Button2Mask ? 1 : 0);
        b2 = buttonEvent->button == Button3 ? 1 : (buttonEvent->state & Button3Mask ? 1 : 0);
        r = buttonEvent->y/CY; c = buttonEvent->x/CX;
        //printf ("ButtonPress %d %d %d, x=%d, y=%d\n", b1, b2, b3, r, c);
        mouse_received (b1, b2, b3, (int)(clock1()/1000.), r, c);
        break;

    case ButtonRelease:
        if (!fl_opt.mouse_active) break;
        b1 = buttonEvent->button == Button1 ? 0 : (buttonEvent->state & Button1Mask ? 1 : 0);
        b3 = buttonEvent->button == Button2 ? 0 : (buttonEvent->state & Button2Mask ? 1 : 0);
        b2 = buttonEvent->button == Button3 ? 0 : (buttonEvent->state & Button3Mask ? 1 : 0);
        //printf ("ButtonRelease %d %d %d\n", b1, b2, b3);
        mouse_received (b1, b2, b3, (int)(clock1()/1000.), buttonEvent->y/CY, buttonEvent->x/CX);
        break;
        
    case LeaveNotify:
        if (!fl_opt.mouse_active) break;
        //fprintf (stderr, "pointer leaves window\n");
        //b1 = leaveEvent->state & Button1Mask ? 1 : 0;
        //b3 = leaveEvent->state & Button3Mask ? 1 : 0;
        //b2 = leaveEvent->state & Button2Mask ? 1 : 0;
        //mouse_received (b1, b2, b3, (int)(clock1()/1000.), 10000, 10000);
        /* we assume that pointer always leaves window with all buttons up.
         this could cause effect as if we release pointer at the edge of the
         window but we don't care much about it. */
        //mouse_received (0, 0, 0, (int)(clock1()/1000.), 999, 999);
        break;
        
    case ConfigureNotify:
        while (XPending(x11_d) > 0 && XCheckTypedWindowEvent (x11_d, x11_w, ConfigureNotify, &event))
            XSync (x11_d, 0);
        newx = configureEvent->width/CX;
        newy = configureEvent->height/CY;
        if (newx != video_hsize() || newy != video_vsize())
        {
            video_init (newy, newx);
            key = FMSG_BASE_SYSTEM + FMSG_BASE_SYSTEM_TYPE*SYSTEM_RESIZE +
                FMSG_BASE_SYSTEM_INT2*newy + FMSG_BASE_SYSTEM_INT1*newx;
        }
        break;
    }
    
    return key;
}

/* ------------------------------------------------------------- */

void set_cursor (int flag)
{
    cursor_visible = flag;
    move_cursor (cursor_Y, cursor_X);
}

/* ------------------------------------------------------------- */

#define XDrawS(s,n,r,c,a) XDrawImageString (x11_d, x11_w, x11_gc[(unsigned int)(a)], (c)*CX, x11_f->max_bounds.ascent + (r)*CY, s, n)
#define XDrawD(s,n,r,c,a) XDrawImageString16 (x11_d, x11_w, x11_kgc[(unsigned int)(a)], (c)*CX, x11_f->max_bounds.ascent + (r)*CY, (XChar2b *)s, n)
#define XDrawW(s,n,r,c,a) XDrawImageString16 (x11_d, x11_w, x11_kgc[(unsigned int)(a)], (c)*DCX/2, x11_f->max_bounds.ascent + (r)*DCY, (XChar2b *)s, n)

void move_cursor (int row, int col)
{
    char s[2], oldattr, newattr;

    //if (is_dbcs)
    //{
    //}
    //else
    {
        // draw over old cursor cell
        s[0] = video_char (cursor_Y, cursor_X);
        s[1] = '\0';
        XDrawS (s, 1, cursor_Y, cursor_X, (unsigned int)video_attr (cursor_Y, cursor_X));
        //XDrawImageString (x11_d, x11_w,
        //                  x11_gc[(unsigned int)video_attr (cursor_Y, cursor_X)],
        //                  cursor_X*CX, x11_f->max_bounds.ascent + cursor_Y*CY,
        //                  s, 1);

        if (cursor_visible)
        {
            // display cursor in the new place
            s[0] = video_char (row, col);
            oldattr = video_attr (row, col);
            newattr = 0x70 - (oldattr & 0xF0) + (oldattr & 0x0F);
            XDrawS (s, 1, row, col, (unsigned int)newattr);
            //XDrawImageString (x11_d, x11_w, x11_gc[(unsigned int)newattr],
            //                  col*CX, x11_f->max_bounds.ascent + row*CY,
            //                  s, 1);
        }
        cursor_X = col;
        cursor_Y = row;
    }
}

/* ------------------------------------------------------------- */

void screen_draw (char *s, int len, int row, int col, char *a)
{
    //XChar2b  s2[256], *ps2 = s2;
    char     s2[256], *ps2 = s2;
    int      i, ss, ds, de;

    if (dbcs_lang == NotDBCS)
    {
        XDrawImageString (x11_d, x11_w, x11_gc[(unsigned int)(*a)],
                          col*CX, x11_f->max_bounds.ascent + row*CY,
                          s, len);
    }
    else if (dbcs_lang == Korean)
    {
        if (len >= 255) ps2 = malloc (len * sizeof (XChar2b)+1);
        // look at what is DBCS and what is not
        ss = 0; // we assume the start of the line is in SBCS (perhaps zero length)
        ds = 0;
        //printf ("%d,%d (%d)", row, col, *a);
        //printchars ("displayed", s, len);
        do
        {
            // find where DBCS part starts
            while (ds < len && !(s[ds] & 0x80)) ds++;
            // find where DBCS part ends
            de = ds;
            while (de < len && (s[de] & 0x80)) de++;
            // need to display SBCS part?
            if (ss != ds)
            {
                //printchars ("SBCS", s+ss, ds-ss);
                XDrawS (s+ss, ds-ss, row, col+ss, *a);
            }
            // need to display DBCS part?
            if (ds != de)
            {
                // this is an ugly hack: since different fonts have different
                // glyph sizes and we are counting on SBCS size, we first paint
                // the area with spaces in SBCS size
                memset (s2, ' ', de-ds);
                XDrawS (s2, de-ds, row, col+ds, *a);
                memcpy (s2, s+ds, de-ds);
                if ((de-ds) % 2 != 0)
                {
                    //printf ("non-even: %d\n", de-ds);
                    //printchars ("DBCS", s2, de-ds);
                }
                for (i=ds; i<de; i++)
                {
                    s2[i-ds] &= 0x7f;
                }
                XDrawD (s2, (de-ds/*+1*/)/2, row, col+ds, *a);
            }
            // back...
            ds = de;  ss = de;
        }
        while (de < len);
        if (len >= 255) free (ps2);
    }
}

/* ------------------------------------------------------------- */

void video_update1 (int *forced)
{
}

/* ------------------------------------------------------------- */

void beep2 (int duration, int frequency)
{
    XBell (x11_d, 50);
}

/* ------------------------------------------------------------- */

char *get_clipboard (void)
{
    XEvent        event;
    Atom          type;
    long          extra, length;
    int           nb, format;
    char          *p, *p1;
    double        now;

    /* check if somebody owns current primary selection. if nobody
     owns it we can proceed with simple XFetchBytes. */
    if (XGetSelectionOwner (x11_d, XA_PRIMARY) == None)
    {
        p1 = XFetchBytes (x11_d, &nb);
        if (p1 == NULL) return NULL;

        p = xmalloc (nb+1);
        memcpy (p, p1, nb);
        p[nb] = '\0';
        XFree (p1);
        return p;
    }

    /* check if we have created atom to convert successfully */
    if (x11_buf == None) return NULL;
    /* this did not work (current selection is owned by someone else).
     therefore we ask to convert selection into our text buffer */
    XConvertSelection (x11_d, XA_PRIMARY, XA_STRING,
                       x11_buf, x11_w, CurrentTime);

    /* now we wait for 5 seconds for SelectionNotify event*/
    now = clock1 ();
    do
    {
        if (XCheckTypedWindowEvent (x11_d, x11_w, SelectionNotify, &event))
            break;
    }
    while (clock1() - now < 5.0);

    if (event.type == SelectionNotify && event.xselection.property != None)
    {
        XGetWindowProperty (x11_d, event.xselection.requestor,
                            event.xselection.property,
                            /* offset and max length */
                            0, 0x10000,
                            /* delete after retrieving */
                            True,
                            /* identifier for atom which holds converted string */
                            event.xselection.target,
                            /* resulting type */
                            &type,
                            /* format of returned data */
                            &format,
                            /* bytes returned */
                            &length,
                            /* bytes remaining */
                            &extra,
                            /* actual data returned (NULL-terminated) */
                            (unsigned char **)&p1);
        /* some checks for data sanity */
        if (format != 8 || type != XA_STRING) return NULL;
        p = strdup (p1);
        XFree (p1);
        return p;
    }

    return NULL;
}

/* ------------------------------------------------------------- */

void put_clipboard (char *s)
{
    XStoreBytes (x11_d, s, strlen (s)+1);
    XSetSelectionOwner (x11_d, XA_PRIMARY, None, CurrentTime);
}

/* ------------------------------------------------------------- */

void fly_launch (char *command, int wait, int pause)
{
    char *p;

    switch (fl_opt.platform)
    {
    case PLATFORM_UNIX_X11:
        p = str_strdup1 (command, 32);
        //strcpy (p, "xterm -ls -e sh ");
        //strcat (p, command);
        if (!wait) strcat (p, " &");
        break;
        
    case PLATFORM_OS2_X11:
        p = str_strdup1 (command, 16);
        p[0] = '\0';
        if (!wait) strcpy (p, "detach ");
        strcat (p, command);
        break;

    default:
        p = strdup (command);
    }

    system (p);
    free (p);
}

/* ------------------------------------------------------------- */

void video_set_window_pos (int x0, int y0)
{
    XMoveWindow (x11_d, x11_w, x0, y0);
}

/* ------------------------------------------------------------- */

int video_get_window_pos (int *x0, int *y0)
{
    int status;
    Window child;

    status = XTranslateCoordinates (x11_d, x11_w, XDefaultRootWindow (x11_d),
                                    0, 0, x0, y0, &child);
    if (status == 0) return 1;

    //printf ("status = %d, x0 = %d, y0 = %d\n", status, *x0, *y0);
    return 0;
}

/* ------------------------------------------------------------- */

char *fly_get_font (void)
{
    return strdup (x11_font);
}

/* ------------------------------------------------------------- */

int fly_get_fontlist (flyfont **F, int restrict_by_language)
{
    char         **fontlist;
    int          nfonts, i, nf = 0;
    XFontStruct  *xfs;

    /*"-cronyx-courier-medium-r-normal--20-140-100-100-m-120-koi8-r"*/
    fontlist = XListFonts (x11_d, "-*-*-*-*-*-m-*-*", 1000, &nfonts);
    if (fontlist != NULL && nfonts > 0)
    {
        *F = malloc (sizeof (flyfont) * nfonts);
        for (i=0; i<nfonts; i++)
        {
            // ignore scalable fonts
            if (strstr (fontlist[i], "-0-0-0-0-") != NULL) continue;
            xfs = XLoadQueryFont (x11_d, fontlist[i]);
            if (xfs != NULL)
            {
                (*F)[nf].cx = xfs->max_bounds.width;
                (*F)[nf].cy = xfs->max_bounds.ascent + xfs->max_bounds.descent;
                (*F)[nf].name = strdup (fontlist[i]);
                (*F)[nf].signature = (void *)xfs->fid;
                nf++;
                XFreeFont (x11_d, xfs);
            }
            else
                fprintf (stderr, "failed to load font: %s\n", fontlist[i]);
        }
        XFreeFontNames (fontlist);
    }
    
    fontlist = XListFonts (x11_d, "-*-*-*-*-*-c-*-*", 1000, &nfonts);
    if (fontlist != NULL && nfonts > 0)
    {
        *F = realloc (*F, sizeof (flyfont) * (nfonts+nf));
        for (i=0; i<nfonts; i++)
        {
            // ignore scalable fonts
            if (strstr (fontlist[i], "-0-0-0-0-") != NULL) continue;
            xfs = XLoadQueryFont (x11_d, fontlist[i]);
            if (xfs != NULL)
            {
                (*F)[nf].cx = xfs->max_bounds.width;
                (*F)[nf].cy = xfs->max_bounds.ascent + xfs->max_bounds.descent;
                (*F)[nf].name = strdup (fontlist[i]);
                (*F)[nf].signature = (void *)xfs->fid;
                nf++;
                XFreeFont (x11_d, xfs);
            }
            else
                fprintf (stderr, "failed to load font: %s\n", fontlist[i]);
        }
        XFreeFontNames (fontlist);
    }

    // now we sort them by cy, cx
    qsort (*F, nf, sizeof (flyfont), font_compare);

    // now make the names unique
    nf = uniq (*F, nf, sizeof (flyfont), font_compare);
    
    return nf;
}

/* ------------------------------------------------------------- */

static int font_compare (__const__  void *f1, __const__  void *f2)
{
    flyfont *font1 = (flyfont *)f1, *font2 = (flyfont *)f2;

    if (font1->cy != font2->cy) return font1->cy - font2->cy;
    if (font1->cx != font2->cx) return font1->cx - font2->cx;
    return strcmp (font1->name, font2->name);
}

/* ------------------------------------------------------------- */

int fly_set_font (char *fontname)
{
    /*
    XFontStruct     *xfs;
    XGCValues       gcv;
    unsigned long   mask;
    int             i;

    xfs = XLoadQueryFont (x11_d, fontname);
    if (xfs == NULL) return 1;

    XFreeFont (x11_d, x11_f);
    x11_f = xfs;
    
    gcv.font = x11_f->fid;
    mask = GCFont;
    for (i = 0; i<256; i++)
    {
        XChangeGC (x11_d, x11_gc[i], mask, &gcv);
    }

    if (x11_font != NULL) free (x11_font);
    x11_font = strdup (fontname);
    */
    
    Xsetup_font (fontname);
    Xsetup_colour ();
    XResizeWindow (x11_d, x11_w, video_hsize() * CX, video_vsize() * CY);
    return 0;
}

/* ------------------------------------------------------------- */

int video_set_window_size (int rows, int cols)
{
    if (rows == video_vsize() && cols == video_hsize()) return 0;

    XResizeWindow (x11_d, x11_w, cols * CX, rows * CY);
    video_init (rows, cols);
    
    return 0;
}

/* ------------------------------------------------------------- */

void set_window_name (char *format, ...)
{
    char     buffer[8192];
    va_list  args;
    
    va_start (args, format);
    vsnprintf1 (buffer, sizeof(buffer), format, args);
    va_end (args);
    
    XStoreName (x11_d, x11_w, buffer);
}

/* ------------------------------------------------------------- */

char *get_window_name (void)
{
    char *p, *p1;

    XFetchName (x11_d, x11_w, &p);
    if (p == NULL)
    {
        p1 = strdup (" ");
    }
    else
    {
        p1 = strdup (p);
        XFree (p);
    }
    
    return p1;
}

/* ------------------------------------------------------------- */

typedef struct {int x, a;} key_x_fly;

key_x_fly x11_plain[] =
{
    {XK_BackSpace, _BackSpace}, {XK_Tab, _Tab}, 
    {XK_Return, _Enter},  {XK_Escape, _Esc}, 
    {XK_Delete, _Delete}, {XK_Insert, _Insert},

    {XK_F1, _F1},    {XK_F2, _F2},    {XK_F3, _F3},    {XK_F4, _F4},
    {XK_F5, _F5},    {XK_F6, _F6},    {XK_F7, _F7},    {XK_F8, _F8},
    {XK_F9, _F9},    {XK_F10, _F10},  {XK_F11, _F11},  {XK_F12, _F12},

    {XK_Home, _Home}, {XK_Page_Up, _PgUp}, {XK_Page_Down, _PgDn}, {XK_End, _End}, /*XK_Begin ? */
    {XK_Left, _Left}, {XK_Up, _Up}, {XK_Right, _Right}, {XK_Down, _Down},
    
    {XK_KP_Home, _Home}, {XK_KP_Page_Up, _PgUp}, {XK_KP_Page_Down, _PgDn}, {XK_KP_End, _End}, /*XK_Begin ? */
    {XK_KP_Left, _Left}, {XK_KP_Up, _Up}, {XK_KP_Right, _Right}, {XK_KP_Down, _Down},
    {XK_KP_Enter, _Enter}, {XK_KP_Insert, _Insert}, {XK_KP_Delete, _Delete},

    {XK_KP_0, _Insert}, {XK_KP_1, _End}, {XK_KP_2, _Down}, {XK_KP_3, _PgDn}, {XK_KP_4, _Left},
    {XK_KP_5, _Gold}, {XK_KP_6, _Right}, {XK_KP_7, _Home}, {XK_KP_8, _Up}, {XK_KP_9, _PgUp},
    {XK_KP_Decimal, _Delete},
    
    {XK_KP_Add, '+'}, {XK_KP_Subtract, '-'}, {XK_KP_Multiply, '*'}, {XK_KP_Divide, '/'}
};

key_x_fly x11_shift[] =
{
    {XK_F1, _ShiftF1},    {XK_F2, _ShiftF2},    {XK_F3, _ShiftF3},    {XK_F4, _ShiftF4},
    {XK_F5, _ShiftF5},    {XK_F6, _ShiftF6},    {XK_F7, _ShiftF7},    {XK_F8, _ShiftF8},
    {XK_F9, _ShiftF9},    {XK_F10, _ShiftF10},  {XK_F11, _ShiftF11},  {XK_F12, _ShiftF12},

    {XK_Tab, _ShiftTab},  {XK_Insert, _ShiftInsert},
    
    {XK_KP_Home, _ShiftHome}, {XK_KP_Page_Up, _ShiftPgUp}, {XK_KP_Page_Down, _ShiftPgDn}, {XK_KP_End, _ShiftEnd}, /*XK_Begin ? */
    {XK_KP_Left, _ShiftLeft}, {XK_KP_Up, _ShiftUp}, {XK_KP_Right, _ShiftRight}, {XK_KP_Down, _ShiftDown},
    {XK_KP_Enter, _ShiftEnter}, {XK_KP_Insert, _ShiftInsert}, {XK_KP_Delete, _ShiftDelete},

    {'`', '~'}, {'1', '!'},  {'2', '@'}, {'3', '#'}, {'4', '$'}, {'5', '%'},
    {'6', '^'}, {'7', '&'},  {'8', '*'}, {'9', '('}, {'0', ')'}, {'-', '_'},
    {'=', '+'}, {'\\', '|'}, {'[', '{'}, {']', '}'}, {';', ':'}, {'\'', '"'},
    {',', '<'}, {'.', '>'},  {'/', '?'},
    {' ', ' '}
};

key_x_fly x11_ctrl[] =
{
    {XK_F1, _CtrlF1},    {XK_F2, _CtrlF2},    {XK_F3, _CtrlF3},    {XK_F4, _CtrlF4},
    {XK_F5, _CtrlF5},    {XK_F6, _CtrlF6},    {XK_F7, _CtrlF7},    {XK_F8, _CtrlF8},
    {XK_F9, _CtrlF9},    {XK_F10, _CtrlF10},  {XK_F11, _CtrlF11},  {XK_F12, _CtrlF12},

    {XK_Home, _CtrlHome}, {XK_Page_Up, _CtrlPgUp}, {XK_Page_Down, _CtrlPgDn}, {XK_End, _CtrlEnd}, /*XK_Begin ? */
    {XK_Left, _CtrlLeft}, {XK_Up, _CtrlUp}, {XK_Right, _CtrlRight}, {XK_Down, _CtrlDown},
    
    {XK_BackSpace, _CtrlBackSpace}, {XK_Tab, _CtrlTab}, {XK_Return, _CtrlEnter},
    {XK_Delete, _CtrlDelete}, {XK_Insert, _CtrlInsert},
    
    {XK_KP_Home, _CtrlHome}, {XK_KP_Page_Up, _CtrlPgUp}, {XK_KP_Page_Down, _CtrlPgDn}, {XK_KP_End, _CtrlEnd}, /*XK_Begin ? */
    {XK_KP_Left, _CtrlLeft}, {XK_KP_Up, _CtrlUp}, {XK_KP_Right, _CtrlRight}, {XK_KP_Down, _CtrlDown},
    {XK_KP_Enter, _CtrlEnter}, {XK_KP_Insert, _CtrlInsert}, {XK_KP_Delete, _CtrlDelete}
};

key_x_fly x11_alt[] =
{
    {XK_F1, _AltF1},    {XK_F2, _AltF2},    {XK_F3, _AltF3},    {XK_F4, _AltF4},
    {XK_F5, _AltF5},    {XK_F6, _AltF6},    {XK_F7, _AltF7},    {XK_F8, _AltF8},
    {XK_F9, _AltF9},    {XK_F10, _AltF10},  {XK_F11, _AltF11},  {XK_F12, _AltF12},

    {XK_Home, _AltHome}, {XK_Page_Up, _AltPageUp}, {XK_Page_Down, _AltPageDn}, {XK_End, _AltEnd}, /*XK_Begin ? */
    {XK_Left, _AltLeft}, {XK_Up, _AltUp}, {XK_Right, _AltRight}, {XK_Down, _AltDown},
    
    {XK_BackSpace, _AltBackSpace}, {XK_Tab, _AltTab}, /*XK_Clear*/ {XK_Return, _AltEnter},
    {XK_Delete, _AltDelete}, {XK_Insert, _AltInsert},

    {'a', _AltA},    {'b', _AltB},    {'c', _AltC},    {'d', _AltD},
    {'e', _AltE},    {'f', _AltF},    {'g', _AltG},    {'h', _AltH},
    {'i', _AltI},    {'j', _AltJ},    {'k', _AltK},    {'l', _AltL},
    {'m', _AltM},    {'n', _AltN},    {'o', _AltO},    {'p', _AltP},
    {'q', _AltQ},    {'r', _AltR},    {'s', _AltS},    {'t', _AltT},
    {'u', _AltU},    {'v', _AltV},    {'w', _AltW},    {'x', _AltX},
    {'y', _AltY},    {'z', _AltZ},       

    {'1', _Alt1},    {'2', _Alt2},    {'3', _Alt3},    {'4', _Alt4},
    {'5', _Alt5},    {'6', _Alt6},    {'7', _Alt7},    {'8', _Alt8},
    {'9', _Alt9},    {'0', _Alt0},
    
    {XK_KP_Home, _AltHome}, {XK_KP_Page_Up, _AltPageUp}, {XK_KP_Page_Down, _AltPageDn}, {XK_KP_End, _AltEnd}, /*XK_Begin ? */
    {XK_KP_Left, _AltLeft}, {XK_KP_Up, _AltUp}, {XK_KP_Right, _AltRight}, {XK_KP_Down, _AltDown},
    {XK_KP_Enter, _AltEnter}, {XK_KP_Insert, _AltInsert}, {XK_KP_Delete, _AltDelete}
};

static int convert_x11_key (KeySym keysym, int k_shift, int k_ctrl, int k_alt, int k_lock)
{
    int i;

    // ignore multiple modifiers
    if ((k_shift ? 1 : 0) + (k_ctrl ? 1 : 0) + (k_alt ? 1 : 0) > 1) return -1;

    // keys without modifiers
    if (!k_shift && !k_ctrl && !k_alt && !k_lock)
    {
        // simple ASCII input
        if (keysym >= ' ' && keysym <= '~') return keysym;
        // lookup key
        for (i=0; i<sizeof(x11_plain)/sizeof(key_x_fly); i++)
            if (keysym == x11_plain[i].x) return x11_plain[i].a;
        return -1;
    }

    // capslocked keys
    if (k_lock && !k_shift && !k_ctrl && !k_alt)
    {
        // lookup key
        for (i=0; i<sizeof(x11_plain)/sizeof(key_x_fly); i++)
            if (keysym == x11_plain[i].x) return x11_plain[i].a;
    }

    // shifted keys
    if ((k_shift && !k_lock) || (!k_shift && k_lock))
    {
        // simple ASCII input
        if (keysym >= 'a' && keysym <= 'z') return keysym+'A'-'a';
        // lookup key
        for (i=0; i<sizeof(x11_shift)/sizeof(key_x_fly); i++)
            if (keysym == x11_shift[i].x) return x11_shift[i].a;
        return -1;
    }

    // ctrl-keys
    if (k_ctrl)
    {
        // simple ASCII input
        if (keysym >= 'a' && keysym <= 'z') return keysym-'a'+1;
        // lookup key
        for (i=0; i<sizeof(x11_ctrl)/sizeof(key_x_fly); i++)
            if (keysym == x11_ctrl[i].x) return x11_ctrl[i].a;
        return -1;
    }
    
    // alt-keys
    if (k_alt)
    {
        // lookup key
        for (i=0; i<sizeof(x11_alt)/sizeof(key_x_fly); i++)
            if (keysym == x11_alt[i].x) return x11_alt[i].a;
        return -1;
    }
    
    return -1;
}

/* ------------------------------------------------------------- */

void fly_process_args (int *argc, char **argv[], char ***envp)
{
    int  i, j;
    
    // check for -display argument
    for (i=1; i<*argc; i++)
    {
        if (strcmp ((*argv)[i], "-display") == 0 && i != *argc-1)
        {
            cmdarg_display = strdup ((*argv)[i+1]);
            for (j=i; j<(*argc)-2; j++)
                (*argv)[j] = (*argv)[j+2];
            *argc -= 2;
            break;
        }
    }
    
    // check for -fn argument
    for (i=1; i<*argc; i++)
    {
        if (strcmp ((*argv)[i], "-fn") == 0 && i != *argc-1)
        {
            cmdarg_font = strdup ((*argv)[i+1]);
            for (j=i; j<(*argc)-2; j++)
                (*argv)[j] = (*argv)[j+2];
            *argc -= 2;
            break;
        }
    }
    
    // check for -geometry argument
    for (i=1; i<*argc; i++)
    {
        if (strcmp ((*argv)[i], "-geometry") == 0 && i != *argc-1)
        {
            cmdarg_geom = strdup ((*argv)[i+1]);
            for (j=i; j<(*argc)-2; j++)
                (*argv)[j] = (*argv)[j+2];
            *argc -= 2;
            break;
        }
    }
    (*argv)[*argc] = NULL;
    
    //printf ("%d arguments:\n", *argc);
    //for (i=0; i<*argc; i++)
    //    printf ("%3d: %s\n", i, (*argv)[i]);
}

/* -------------------------------------------------------------

static void printchars (char *hdr, char *buf, int n)
{
    int i;
    
    printf ("%s: [", hdr);
    for (i=0; i<n; i++)
    {
        printf("%c", buf[i]);
    }
    printf ("]\n");
}

------------------------------------------------------------- */

static void Xsetup_colour (void)
{
    long            d = 0x7FFFFFFF, d1;
    unsigned long   mask;
    int             i, j, num;
    unsigned long   pix;
    long            d_red, d_green, d_blue;
    long            u_red, u_green, u_blue;
    XColor          clr;
    XGCValues       gcv;
    
    // create GC values for all 256 possible colours
    x11_cm = DefaultColormap (x11_d, DefaultScreen (x11_d));
    for (i=0; i<16; i++)
    {
        SetColor (i);
        if (XAllocColor (x11_d, x11_cm, &x11_clr[i]) == 0)
        {
            SetColor(i);
            pix = 0xFFFFFFFF;
            num = DisplayCells (x11_d, DefaultScreen(x11_d));
            for (j = 0; j < num; j++)
            {
                clr.pixel = j;
                XQueryColor (x11_d, x11_cm, &clr);

                d_red = (clr.red - x11_clr[i].red) >> 3;
                d_green = (clr.green - x11_clr[i].green) >> 3;
                d_blue = (clr.blue - x11_clr[i].blue) >> 3;

                u_red = d_red / 100 * d_red * 3;
                u_green = d_green / 100 * d_green * 4;
                u_blue = d_blue / 100 * d_blue * 2;

                d1 = u_red + u_blue + u_green;

                if (d1 < 0)
                    d1 = -d1;
                if (pix == ~0UL || d1 < d)
                {
                    pix = j;
                    d = d1;
                }
            }
            clr.pixel = pix;
            XQueryColor (x11_d, x11_cm, &clr);
            x11_clr[i] = clr;
            if (XAllocColor (x11_d, x11_cm, &x11_clr[i]) == 0)
            {
                fprintf(stderr, "Color alloc failed for #%04X%04X%04X\n",
                        x11_clr[i].red,
                        x11_clr[i].green,
                        x11_clr[i].blue);
            }
        }
    }

    gcv.font = x11_f->fid;
    mask = GCForeground | GCBackground | GCFont;
    for (i = 0; i<256; i++)
    {
        gcv.foreground = x11_clr[i % 16].pixel;
        gcv.background = x11_clr[(i / 16)].pixel;
        x11_gc[i] = XCreateGC (x11_d, x11_w, mask, &gcv);
    }

    if (is_dbcs)
    {
        gcv.font = x11_kf->fid;
        for (i = 0; i<256; i++)
        {
            gcv.foreground = x11_clr[i % 16].pixel;
            gcv.background = x11_clr[(i / 16)].pixel;
            x11_kgc[i] = XCreateGC (x11_d, x11_w, mask, &gcv);
        }
    }
}

/* ------------------------------------------------------------- */

static void Xsetup_font (char *font)
{
    char  *language, sbcs_font[32];
    int   dbcs_cx, dbcs_cy;

    language = cfg_get_string (0, fl_opt.platform_nick, "language");
    if (language[0] == '\0') language = "english";
    //printf ("language is %s\n", language);
    
    if (!strcmp (language, "korean"))         dbcs_lang = Korean;
    else if (!strcmp (language, "japanese"))  dbcs_lang = Japanese;
    else if (!strcmp (language, "chinese"))   dbcs_lang = Chinese;
    else                                      dbcs_lang = NotDBCS;
    
    if (is_dbcs)
    {
        // the primary font specified is DBCS font
        if (cmdarg_font != NULL) font = cmdarg_font;
        if (font == NULL)
        {
            switch (dbcs_lang)
            {
            case Korean:
                font = "-daewoo-mincho-medium-r-normal--16-120-100-100-c-160-ksc5601.1987-0"; break;
            case Japanese:
                font = "-jis-fixed-medium-r-normal--16-150-75-75-c-160-jisx0208.1983-0"; break;
            case Chinese:
                font = "-isas-song ti-medium-r-normal--16-160-72-72-c-160-gb2312.1980-0"; break;
            }
        }
        x11_kf = XLoadQueryFont (x11_d, font);
        if (x11_kf == NULL)
            fly_error ("cannot load font \"%s\"\n", font);
        // we need to find the SBCS font to match its size
        dbcs_cx = x11_kf->max_bounds.width;
        dbcs_cy = x11_kf->max_bounds.ascent + x11_kf->max_bounds.descent;
        if (dbcs_cy == dbcs_cx) dbcs_cx /= 2;
        snprintf1 (sbcs_font, sizeof (sbcs_font), "%dx%d", dbcs_cx, dbcs_cy);
        x11_f = XLoadQueryFont (x11_d, sbcs_font);
        if (x11_f == NULL)
            fly_error ("cannot load font \"%s\"\n", sbcs_font);
        fprintf (stderr, "using ISO8859-1 font %s\n", sbcs_font);
    }
    else
    {
        if (cmdarg_font != NULL) font = cmdarg_font;
        if (font == NULL) font = "9x15";
        x11_f = XLoadQueryFont (x11_d, font);
        if (x11_f == NULL)
        {
            fprintf (stderr, "cannot load font \"%s\", trying \"fixed\"\n", font);
            font = "fixed";
            x11_f = XLoadQueryFont (x11_d, font);
            if (x11_f == NULL)
                fly_error ("cannot load font \"fixed\"");
        }
        if (x11_font != NULL) free (x11_font);
    }
    
    x11_font = strdup (font);
}

/* ------------------------------------------------------------- */

void video_flush (void)
{
    XSync (x11_d, 0);
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

