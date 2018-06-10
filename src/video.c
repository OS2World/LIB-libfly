#include <stdlib.h>
#include <string.h>
#include <asvtools.h>

#include "fly/fly.h"
#include "fly-internal.h"

/* scrn_store_* reflect what is on the screen,
 scrn_cache_* are buffers where operations are performed */
static char  *scrn_store_chars = NULL, *scrn_store_attrs = NULL;
static char  *scrn_cache_chars = NULL, *scrn_cache_attrs = NULL;

static int   cursor_cache = 0, cursor_store = 0;

/* data about screen and working storage */
static int   CURSOR_ROW=0, CURSOR_COL=0, ROWS=0, COLS=0;

int backgrounded = 0;

/* ------------------------------------------------------------- */

static void fix_string (char *s, int n)
{
    int             i;
    unsigned char   uc;

    if (n <= 0) return;
    // replace '\0's chars with spaces
    for (i=0; i<n; i++)
    {
        uc = (unsigned char) s[i];
        if (uc < ' ' || uc == 127) s[i] = ' ';
    }
}

/* ------------------------------------------------------------- */

void video_init (int rows, int cols)
{
    int   row;
    char  *scrn_cache_chars1, *scrn_cache_attrs1;

    // free old store, create new one
    if (scrn_store_chars != NULL)  free (scrn_store_chars);
    if (scrn_store_attrs != NULL)  free (scrn_store_attrs);
    scrn_store_chars = malloc (rows*cols);
    scrn_store_attrs = malloc (rows*cols);

    // allocate new backing store buffers
    scrn_cache_chars1 = malloc (rows*cols);
    scrn_cache_attrs1 = malloc (rows*cols);
    memset (scrn_cache_chars1, ' ', rows*cols);
    memset (scrn_cache_attrs1, fl_clr.background, rows*cols);
    if (scrn_cache_chars != NULL && scrn_cache_attrs != NULL)
    {
        for (row=0; row < min1(ROWS,rows); row++)
        {
            memcpy (scrn_cache_chars1+row*cols, scrn_cache_chars+row*COLS, min1(cols, COLS));
            memcpy (scrn_cache_attrs1+row*cols, scrn_cache_attrs+row*COLS, min1(cols, COLS));
        }
        free (scrn_cache_chars);
        free (scrn_cache_attrs);
        CURSOR_ROW = min1 (rows-1, CURSOR_ROW);
        CURSOR_COL = min1 (cols-1, CURSOR_COL);
    }
    else
    {
        CURSOR_ROW = 0;
        CURSOR_COL = 0;
    }
    scrn_cache_chars = scrn_cache_chars1;
    scrn_cache_attrs = scrn_cache_attrs1;
    
    // bring the screen to the initial state
    ROWS = rows;
    COLS = cols;
    set_cursor (cursor_store);
    
    video_update (1);
}

/* ------------------------------------------------------------- */

void video_terminate (void)
{
    free (scrn_store_chars);   scrn_store_chars = NULL;
    free (scrn_store_attrs);   scrn_store_attrs = NULL;
    free (scrn_cache_chars);   scrn_cache_chars = NULL;
    free (scrn_cache_attrs);   scrn_cache_attrs = NULL;
}
    
/* ------------------------------------------------------------- */

void video_put_n_char (char c, int n, int row, int col)
{
    if (n <= 0 || row < 0 || col < 0) return;
    if (row >= ROWS || col >= COLS) return;
    n = min1 (n, COLS-col);
    memset (scrn_cache_chars+row*COLS+col, c == '\0' ? ' ' : c, n);
}

/* ------------------------------------------------------------- */

void video_put_n_attr (char a, int n, int row, int col)
{
    if (n <= 0 || row < 0 || col < 0) return;
    if (row >= ROWS || col >= COLS) return;
    n = min1 (n, COLS-col);
    memset (scrn_cache_attrs+row*COLS+col, a, n);
}

/* ------------------------------------------------------------- */

void video_put_n_cell (char c, char a, int n, int row, int col)
{
    video_put_n_char (c, n, row, col);
    video_put_n_attr (a, n, row, col);
}

/* ------------------------------------------------------------- */

void video_put_str (char *s, int n, int row, int col)
{
    if (n <= 0 || row < 0 || col < 0) return;
    if (row >= ROWS || col >= COLS) return;
    n = min1 (n, COLS-col);
    memcpy (scrn_cache_chars+row*COLS+col, s, n);
    fix_string (scrn_cache_chars+row*COLS+col, n);
}

/* ------------------------------------------------------------- */

void video_put_str_attr (char *s, int n, int row, int col, char a)
{
    video_put_str (s, n, row, col);
    video_put_n_attr (a, n, row, col);
}

/* ------------------------------------------------------------- */

void video_clear (int row1, int col1, int row2, int col2, char a)
{
    int  row;

    for (row=row1; row<=row2; row++)
        video_put_n_cell (' ', a, col2-col1+1, row, col1);
}

/* ------------------------------------------------------------- */

char *video_save (int row1, int col1, int row2, int col2)
{
    char    *s;
    int     r, ws, lw, lls;

    col1 = max1 (0, min1 (col1, COLS-1));
    col2 = max1 (0, min1 (col2, COLS-1));
    row1 = max1 (0, min1 (row1, ROWS-1));
    row2 = max1 (0, min1 (row2, ROWS-1));
    
    ws  = (row2-row1+1)*(col2-col1+1);
    lw  = col2-col1+1;
    lls = sizeof(int) * 4;
    s   = malloc (ws*2 + lls);

    ((int *)s)[0] = row1;
    ((int *)s)[1] = col1;
    ((int *)s)[2] = row2;
    ((int *)s)[3] = col2;

    for (r=row1; r<=row2; r++)
    {
        memcpy (s+(r-row1)*lw+lls, scrn_cache_chars+r*COLS+col1, lw);
        memcpy (s+(r-row1)*lw+ws+lls, scrn_cache_attrs+r*COLS+col1, lw);
    }
    return s;
}

/* ------------------------------------------------------------- */

void video_restore (char *s)
{
    int     r, ws, lw, lls;
    int     row1, col1, row2, col2;

    if (s == NULL) return;
    
    row1 = ((int *)s)[0];
    col1 = ((int *)s)[1];
    row2 = ((int *)s)[2];
    col2 = ((int *)s)[3];
    
    ws = (row2-row1+1)*(col2-col1+1);
    lw = col2 - col1 + 1;
    lls = sizeof(int) * 4;
    
    if (row1 > row2 || row1 >= ROWS ||
        col1 > col2 || col1 >= COLS)
    {
        free (s);
        return;
    }
                        
    for (r=row1; r<=min1(row2, ROWS-1); r++)
    {
        memcpy (scrn_cache_chars+r*COLS+col1, s+(r-row1)*lw+lls, min1(col2-col1+1, COLS-1-col1+1));
        memcpy (scrn_cache_attrs+r*COLS+col1, s+(r-row1)*lw+ws+lls, min1(col2-col1+1, COLS-1-col1+1));
    }
    
    free (s);
}

/* ------------------------------------------------------------- */

void video_set_cursor (int row, int col)
{
    CURSOR_ROW = row;
    CURSOR_COL = col;
}

/* ------------------------------------------------------------- */

void video_get_cursor (int *row, int *col)
{
    *row = CURSOR_ROW;
    *col = CURSOR_COL;
}

/* ------------------------------------------------------------- */

int video_cursor_state (int newstate)
{
    int oldstate;
    
    if (newstate == -1) return cursor_cache;

    oldstate = cursor_cache;
    cursor_cache = newstate;
    return oldstate;
}

/* ------------------------------------------------------------- */

int video_vsize (void)
{
    return ROWS;
}

/* ------------------------------------------------------------- */

int video_hsize (void)
{
    return COLS;
}

/* ------------------------------------------------------------- */

void video_update (int forced)
{
    char   *p_char, *p_attr, a;
    int    r, c1, c2;

    video_update1 (&forced);
    if (backgrounded) return;

    // update every line
    for (r=0; r<ROWS; r++)
    {
        // set pointers to start of line
        p_char = scrn_cache_chars + r*COLS;
        p_attr = scrn_cache_attrs + r*COLS;
        c1 = 0;
        do
        {
        again:
            // find the start of changed area (c1)
            while (!forced && c1 < COLS &&
                   p_char[c1] == scrn_store_chars[r*COLS+c1] &&
                   p_attr[c1] == scrn_store_attrs[r*COLS+c1])
                c1++;
            if (c1 == COLS) break;
            a = p_attr[c1];
            // c2 (end of update region) is where:
            for (c2=c1+1; c2 < COLS; c2++)
            {
                // a) attributes are different
                if (p_attr[c2] != a) break;
                // b) attributes are the same and symbols are the same,
                // and next chars also have the same attributes and value
                if (!forced && c2+4 < COLS-1 &&
                    p_attr[c2] == scrn_store_attrs[r*COLS+c2] &&
                    p_char[c2] == scrn_store_chars[r*COLS+c2] &&
                    p_attr[c2+1] == scrn_store_attrs[r*COLS+c2+1] &&
                    p_char[c2+1] == scrn_store_chars[r*COLS+c2+1] &&
                    p_attr[c2+2] == scrn_store_attrs[r*COLS+c2+2] &&
                    p_char[c2+2] == scrn_store_chars[r*COLS+c2+2] &&
                    p_attr[c2+3] == scrn_store_attrs[r*COLS+c2+3] &&
                    p_char[c2+3] == scrn_store_chars[r*COLS+c2+3] &&
                    p_attr[c2+4] == scrn_store_attrs[r*COLS+c2+4] &&
                    p_char[c2+4] == scrn_store_chars[r*COLS+c2+4]
                   ) break;
            }
            // update segment from c1 (inclusive) to c2 (non-inclusive)
            screen_draw (p_char+c1, c2-c1, r, c1, p_attr+c1);
            c1 = c2;
            if (c2 < COLS && p_attr[c2] != a) goto again;
        }
        while (c2 < COLS);
    }
    
    if (cursor_cache != cursor_store)
    {
        set_cursor (cursor_cache);
        cursor_store = cursor_cache;
    }
    
    // under Unix, move_cursor performs fflush(stdout)
    move_cursor (CURSOR_ROW, CURSOR_COL);
    
    memcpy (scrn_store_chars, scrn_cache_chars, ROWS*COLS);
    memcpy (scrn_store_attrs, scrn_cache_attrs, ROWS*COLS);

    video_flush ();
}

/* ------------------------------------------------------------- */

void video_update_rect (int r1, int c1, int r2, int c2)
{
    int  r;
    char *p_char, *p_attr;

    // invalidate rectange
    for (r=r1; r<=r2; r++)
    {
        p_char = scrn_store_chars + r*COLS;
        p_attr = scrn_store_attrs + r*COLS;
        memset (p_char+c1, 0, c2-c1+1);
        memset (p_attr+c1, 0, c2-c1+1);
    }

    // refresh
    video_update (0);
}

/* ------------------------------------------------------------- */

void video_put (char *s, int row, int col)
{
    int l = strlen (s);
    if (l != 0)
        video_put_str (s, l, row, col);
}

/* ------------------------------------------------------------- */

char video_char (int row, int col)
{
    if (row < 0 || col < 0) return 0;
    if (row >= ROWS || col >= COLS) return 0;
    return *(scrn_cache_chars + row*COLS + col);
}

/* ------------------------------------------------------------- */

char video_attr (int row, int col)
{
    if (row < 0 || col < 0) return 0;
    if (row >= ROWS || col >= COLS) return 0;
    return *(scrn_cache_attrs + row*COLS + col);
}
