#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <stdarg.h>
#include <time.h>
#include <string.h>
#include <unistd.h>

#include <fly/fly.h>

int  keynames_running = 0;

void test_video (int tests);
void test_misc (int tests);
void test_keyboard (int tests);
void message (int row, char *format, ...);
void pause1 (void);
void fill_screen (void);

int cursor_row, cursor_col;
char clear = _BackBlack + _White;

#define TEST_ALL             0xFFFFFFFF
#define TEST_CHANGEWINTITLE  0x00000001
#define TEST_BEEP            0x00000002
#define TEST_WAITKEY         0x00000004
#define TEST_CURSOR          0x00000008
#define TEST_DRIVES          0x00000010
#define TEST_CLIPBOARD       0x00000020
#define TEST_VID_SAVE        0x00010000
#define TEST_VID_PUT         0x00020000
#define TEST_VID_CHARSET     0x00040000
#define TEST_VID_ATTRS       0x00080000
#define TEST_VID_NPUT        0x00100000
#define TEST_VID_SIGWINCH    0x00200000

struct _optstring
{
    int  test;
    char *testname;
};

struct _optstring optstrings[] = {
    {TEST_ALL, "all"},
    {TEST_CHANGEWINTITLE, "wintitle"},
    {TEST_BEEP, "beep"},
    {TEST_WAITKEY, "waitkey"},
    {TEST_CURSOR, "cursor"},
    {TEST_DRIVES, "driveletters"},
    {TEST_CLIPBOARD, "clipboard"},
    {TEST_VID_SAVE, "vid_save"},
    {TEST_VID_PUT, "vid_put"},
    {TEST_VID_CHARSET, "vid_charset"},
    {TEST_VID_ATTRS, "vid_attrs"},
    {TEST_VID_NPUT, "vid_nput"},
    {TEST_VID_SIGWINCH, "vid_sigwinch"},
};

/* --------------------------------------------------------------------
 main: analyze arguments and call corresponding tests
 ---------------------------------------------------------------------- */
int fly_main (int argc, char *argv[], char **envp)
{
    int         i, j, tests;
    char        *wintitle;

    fly_initialize ();
    fly_process_args (&argc, &argv, &envp);

    tools_debug = fopen ("debug.txt", "w");
    
    fprintf (stderr, "\nFLY portability test suite (C) Sergey Ayukov\n\n");
    
    // analyze options and see what tests to run
    tests = 0;
    for (i=1; i<argc; i++)
        for (j=0; j<sizeof(optstrings)/sizeof(optstrings[0]); j++)
            if (strcmp (optstrings[j].testname, argv[i]) == 0)
                tests |= optstrings[j].test;
    if (tests == 0)
    {
        fprintf (stderr, "Valid tests are:");
        for (j=0; j<sizeof(optstrings)/sizeof(optstrings[0]); j++)
            fprintf (stderr, " %s", optstrings[j].testname);
        fprintf (stderr, "\n\n");
        return 1;
    }

    fly_init (-1, -1, -1, -1, NULL);
    fly_mouse (1);

    wintitle = get_window_name ();
    // run tests
    test_video (tests);
    test_keyboard (tests);
    test_misc (tests);
    set_window_name (wintitle);

    fly_terminate ();

    fprintf (stderr, "Normal exit.\n");
    return 0;
}

/* --------------------------------------------------------------------
 test_video: tests video interfaces
 ---------------------------------------------------------------------- */
void test_video (int tests)
{
    char *savebuf, buf[16], ch, at;
    char clear1 = _BackBlue + _High + _Brown;
    int  r, c, f, b, key;
    
    if (tests & TEST_VID_SAVE)
    {
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        message (1, "Testing video_save/restore. "
                 "screen size: %d rows, %d cols", video_vsize(), video_hsize());
        pause1 ();
        
        fill_screen ();
        message (1, "Screen is filled with some pattern");
        pause1 ();
        
        savebuf = video_save (5, 5, 15, 15);
        video_clear (5, 5, 15, 15, clear);
        message (1, "Part of the screen is saved and cleared (5,5; 15,15).");
        message (2, "Clearing entire screen and restoring saved part.");
        pause1 ();
        
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        video_restore (savebuf);
        pause1 ();
    }

    if (tests & TEST_VID_NPUT)
    {
        ch = '%';
        at = _BackBlue + _High + _Brown;

        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        message (1, "Testing video_put_n_char()");
        video_put_n_char (ch, 80, 2, 0);

        message (5, "Testing video_put_n_attr()");
        video_put_n_char (ch, 80, 6, 0);
        video_put_n_attr (at, 80, 6, 0);

        message (8, "Testing video_put_n_cell()");
        video_put_n_cell (ch, at, 80, 9, 0);
        pause1 ();
    }
    
    if (tests & TEST_VID_SIGWINCH)
    {
        do
        {
            video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
            video_put_n_char ('/',  1, 0,             0);
            video_put_n_char ('\\', 1, 0,             video_hsize()-1);
            video_put_n_char ('\\', 1, video_vsize()-1, 0);
            video_put_n_char ('/',  1, video_vsize()-1, video_hsize()-1);
            video_put_n_char ('-', video_hsize()-2, 0,             1);
            video_put_n_char ('-', video_hsize()-2, video_vsize()-1, 1);
            for (r=1; r<=video_vsize()-2; r++)
            {
                video_put_n_char ('|', 1, r, 0);
                video_put_n_char ('|', 1, r, video_hsize()-1);
            }
            message (5, "Window size is %d rows, %d columns",
                     video_vsize(), video_hsize());
            message (6, "Press ESC to exit");
            video_update (0);
            key = getkey (-1);
        }
        while (key != _Esc);
        pause1 ();
    }
    
    if (tests & TEST_VID_PUT)
    {
        
        message (9, "Testing video_put_str()");
        video_put_str ("string to display", 17, 10, 0);
        pause1 ();

        message (12, "Testing terminal capabilities");
        video_put_n_char ('/',  1, 0,             0);
        video_put_n_char ('\\', 1, 0,             video_hsize()-1);
        video_put_n_char ('\\', 1, video_vsize()-1, 0);
        video_put_n_char ('/',  1, video_vsize()-1, video_hsize()-1);
        video_put_n_char ('-', video_hsize()-2, 0,             1);
        video_put_n_char ('-', video_hsize()-2, video_vsize()-1, 1);
        for (r=1; r<=video_vsize()-2; r++)
        {
            video_put_n_char ('|', 1, r, 0);
            video_put_n_char ('|', 1, r, video_hsize()-1);
        }
        pause1 ();

        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear1);
        message (1, "Just tested video_clear()");
        pause1 ();
        
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
    }

    if (tests & TEST_VID_CHARSET)
    {
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        message (1, "Character table for current terminal -- low part");
        for (c = 0; c <= 127; c++)
        {
            ch = c;
            video_put_n_char (ch, 1, 3+c/8, 4+(c%8)*8);
            snprintf1 (buf, sizeof(buf), "%3d", c);
            video_put_str (buf, 3, 3+c/8, 4+(c%8)*8+3);
        }
        pause1 ();
        
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        message (1, "Character table for current terminal -- high part");
        for (c = 128; c <= 255; c++)
        {
            ch = c;
            video_put_n_char (ch, 1, 3+(c-128)/8, 4+(c%8)*8);
            snprintf1 (buf, sizeof(buf), "%3d", c);
            video_put_str    (buf, 3, 3+(c-128)/8, 4+(c%8)*8+3);
        }
        pause1 ();
    }
    
    if (tests & TEST_VID_ATTRS)
    {
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        message (1, "Screen video attributes");
        for (f = 0; f <= 15; f++)
        {
            snprintf1 (buf, sizeof(buf), "%02d", f);
            video_put_str (buf, 2, 3, 3+f*3);
        }
        for (b = 0; b <= 7; b++)
        {
            snprintf1 (buf, sizeof(buf), "%02d", b);
            video_put_str (buf, 2, 5+b*2, 0);
            for (f = 0; f <= 15; f++)
            {
                at = f + (b<<4);
                video_put_str_attr ("**", 2, 5+b*2, 3+f*3, at);
            }
        }
        pause1 ();
    }
}
/* --------------------------------------------------------------------
 test_keyboard: tests keyboard interfaces
 ---------------------------------------------------------------------- */
void test_keyboard (int tests)
{
    int      key;
    double   t1, t2;
    
    // testing getkey
    if (tests & TEST_WAITKEY)
    {
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        message (1, "Testing getkey()");
        message (2, "checking getkey(-1). waiting for a key...");
        video_update (0);
        key = getkey (-1);
        message (3, "detected keypress; keycode = %d", key);
        pause1 ();

        message (5, "checking getkey(0). waiting for a key...");
        video_update (0);
        while ((key = getkey (0)) == 0);
        message (6, "returned %d", key);
        pause1 ();

        message (8, "waiting for key during 5 sec");
        video_update (0);
        t1 = clock1 ();
        key = getkey (5000);
        t2 = clock1 ();
        message (9, "returned %d. elapsed time is %lu seconds",
                 key, (unsigned long)(t2-t1));
        pause1 ();
    }
}

/* --------------------------------------------------------------------
 test_misc: tests miscellaneous functions
 ---------------------------------------------------------------------- */
void test_misc (int tests)
{
    int             f, i, d, r, c;
    unsigned long   drivemap;
    char            b[3], letters[32], wd[16384], *p;

    if (tests & TEST_CHANGEWINTITLE)
    {
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        message (1, "Testing get/set_window_name()");
        message (2, "Current title is [%s]", get_window_name ());
        pause1 ();
        set_window_name ("Running FLY testsuite, pid=%d", getpid());
        message (3, "window title should now display "
                 "`Running FLY testsuite, pid=%d'", getpid ());
        message (4, "Current title is [%s]", get_window_name ());
        pause1 ();
        set_window_name ("Old title");
        message (5, "window title should now display `Old title'");
        pause1 ();
    }

    if (tests & TEST_BEEP)
    {
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        message (1, "Testing beep2() for frequency");
        video_update (0);
        for (f=20;    f<20000; f = (f*5)/4) beep2 (f, 200);
        for (f=20000; f>20;    f = (f*4)/5) beep2 (f, 200);
        pause1 ();
        message (2, "Testing beep2() for duration");
        video_update (0);
        for (i=0; i<50; i++) beep2 (500, 10);
        for (i=0; i<25; i++) beep2 (1000, 20);
        for (i=0; i<10; i++) beep2 (2000, 50);
        pause1 ();
    }

    if (tests & TEST_CURSOR)
    {
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        message (1, "Testing hw_cursor()");
        video_cursor_state (1);
        message (2, "cursor should be visible now");
        pause1 ();

        video_get_cursor (&r, &c);
        message (4, "current cursor position: (%d, %d)", r, c);
        pause1 ();

        video_set_cursor (video_vsize()-1, video_hsize()-1);
        message (6, "trying to move cursor to (%d, %d)", video_vsize()-1, video_hsize()-1);
        video_get_cursor (&r, &c);
        message (7, "current cursor position: (%d, %d)", r, c);
        pause1 ();

        video_cursor_state (0);
        message (9, "cursor should not be visible now");
        pause1 ();
    }

    if (tests & TEST_DRIVES)
    {
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        message (1, "Testing change_drive()/query_drive()/query_drivemap()");
        drivemap = query_drivemap ();
        strcpy (letters, "");
        strcpy (b, "A");
        for (d=0; d<='Z'-'A'; d++)
        {
            if (drivemap & (1L << d))
            {
                b[0] = 'A' + d;
                strcat (letters, b);
            }
        }
        message (2, "available drive letters are: %s", letters);
        
        d = query_drive ();
        getcwd (wd, sizeof(wd));
        message (4, "current drive is %c:, wd is %s", d+'A', wd);
        
        message (6, "trying to change to C:");
        change_drive ('C'-'A');
        getcwd (wd, sizeof(wd));
        message (7, "current drive is now %c:, wd is %s",
                 query_drive()+'A', wd);
        
        message (9, "trying to change to D:");
        change_drive ('D'-'A');
        getcwd (wd, sizeof(wd));
        message (10, "current drive is now %c:, wd is %s",
                 query_drive()+'A', wd);
        
        message (12, "trying to change back to %c:", d+'A');
        change_drive (d);
        getcwd (wd, sizeof(wd));
        message (13, "current drive is now %c:, wd is %s",
                 query_drive()+'A', wd);
        
        pause1 ();
    }

    if (tests & TEST_CLIPBOARD)
    {
        video_clear (0, 0, video_vsize()-1, video_hsize()-1, clear);
        p = get_clipboard ();
        if (p == NULL) p = "Failed!";
        message (1, "current clipboard contents: [%s]", p);
        p = get_clipboard ();
        if (p == NULL) p = "Failed!";
        message (4, "current clipboard contents: [%s]", p);
        pause1 ();

        p = "FLY portability test suite -- clipboard test";
        message (8, "putting string `%s' into clipboard...", p);
        put_clipboard (p);
        p = get_clipboard ();
        if (p == NULL) p = "Failed!";
        message (10, "current clipboard contents: [%s]", p);
        p = get_clipboard ();
        if (p == NULL) p = "Failed!";
        message (13, "current clipboard contents: [%s]", p);
        pause1 ();
    }
}

/* --------------------------------------------------------------------
 message: displays a message on the screen
 ---------------------------------------------------------------------- */
void message (int row, char *format, ...)
{
    va_list       args;
    char          buffer[1024];

    va_start (args, format);
    vsnprintf1 (buffer, sizeof(buffer), format, args);
    va_end (args);

    video_clear (row, 1, row, video_hsize()-2, clear);
    video_put_str (buffer, strlen(buffer), row, 1);
}

/* --------------------------------------------------------------------
 pause1: waits for user to press a key
 ---------------------------------------------------------------------- */
void pause1 (void)
{
    int key;
    
    video_put_str ("press esc to quit or any other key to continue",
                   46, video_vsize()-2, 0);
    video_update (0);
    key = getkey (-1);
    if (key == _Esc) exit (1);
    video_clear (video_vsize()-2, 0, video_vsize()-2, video_hsize()-1, clear);
    video_update (0);
}

/* --------------------------------------------------------------------
 fill_screen: fills screen with some pattern
 ---------------------------------------------------------------------- */
void fill_screen (void)
{
    int  row, col;
    char s, a;
    
    for (row=0; row<video_vsize(); row++)
        for (col=0; col<video_hsize(); col++)
        {
            s = (row/2)*2 == row ? '*' : '#';
            a = (row+col) % 15 + 1;
            video_put_n_cell (s, a, 1, row, col);
        }
    video_update (0);
}
