#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static char            clipboard_text[MAX_CLIP+1];

/* ------------------------------------------------------------- */

void fly_init (int argc, char **argv, char **envp)
{
    video_init (80, 24);
}

/* ------------------------------------------------------------- */

void fly_terminate (void)
{
}

/* ------------------------------------------------------------- */

int getmessage (int interval)
{
    int   c;

    if (interval == 0) return 0;
    video_set_cursor (0, 0);
    fprintf (stderr, "\n(%d):", interval);
    scanf ("%d", &c);
    return c;
}

/* ------------------------------------------------------------- */

void screen_draw (char *s, int len, int row, int col, char *a)
{
    char *b = malloc (len+2);
    memcpy (b, s, len);
    b[len] = '\0';
    if (strspn (b, " ") != strlen (b))
        printf ("%s\n", b);
    free (b);
}

/* ------------------------------------------------------------- */

void video_update1 (int *forced)
{
}

/* ------------------------------------------------------------- */

void move_cursor (int row, int col)
{
}

/* ------------------------------------------------------------- */

void set_cursor (int flag)
{
}

/* ------------------------------------------------------------- */

void beep2 (int duration, int frequency)
{
}

/* ------------------------------------------------------------- */

void set_window_name (char *format, ...)
{
}

/* ------------------------------------------------------------- */

char *get_window_name (void)
{
    return strdup (" ");
}

/* ------------------------------------------------------------- */

/* drives start at 0 (A: - 0) */
int  change_drive (int drive)
{
    return 0;
}

/* ------------------------------------------------------------- */

int  query_drive (void)
{
    return 0;
}

/* ------------------------------------------------------------- */

unsigned long query_drivemap (void)
{
    return 1L;
}

/* ------------------------------------------------------------- */

char *get_clipboard (void)
{
    return clipboard_text;
}

/* ------------------------------------------------------------- */

void put_clipboard (char *s)
{
    strncpy (clipboard_text, s, MAX_CLIP);
    clipboard_text[MAX_CLIP] = '\0';
}
