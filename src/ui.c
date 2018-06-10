#include "fly/fly.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "fly-internal.h"

/* ------------------------------------------------------------- */

void fly_drawframe (int r1, int c1, int r2, int c2)
{
    int  r;

    if (r1 >= r2 || c1 >= c2) return;

    video_put_n_char (fl_sym.h, c2-c1-1, r1, c1+1);
    video_put_n_char (fl_sym.h, c2-c1-1, r2, c1+1);

    for (r=r1+1; r<=r2-1; r++)
    {
        video_put_n_char (fl_sym.v, 1, r, c1);
        video_put_n_char (fl_sym.v, 1, r, c2);
    }

    video_put_n_char (fl_sym.c_lu, 1, r1, c1);
    video_put_n_char (fl_sym.c_ru, 1, r1, c2);
    video_put_n_char (fl_sym.c_ld, 1, r2, c1);
    video_put_n_char (fl_sym.c_rd, 1, r2, c2);

    if (fl_opt.shadows)
        fly_shadow (r1, c1, r2, c2);
}

/*--------------------------------------------------------------------*/

void fly_shadow (int r1, int c1, int r2, int c2)
{
    int  i;
    char a;

    for (i=0; i<r2-r1+1; i++)
    {
        a = video_attr (r1+1+i, c2+1) & ~(0xf8);
        if (a == 0) a = 7;
        video_put_n_attr (a, 1, r1+1+i, c2+1);
        a = video_attr (r1+1+i, c2+2) & ~(0xf8);
        if (a == 0) a = 7;
        video_put_n_attr (a, 1, r1+1+i, c2+2);
    }
    for (i=0; i<c2-c1+1; i++)
    {
        a = video_attr (r2+1, c1+1+i) & ~(0xf8);
        if (a == 0) a = 7;
        video_put_n_attr (a, 1, r2+1, c1+1+i);
    }
}

/*--------------------------------------------------------------------*/

int fly_choose (char *header, int flags, int *nchoices, int startpos,
                void (*drawline)(int n, int len, int shift, int pointer, int row, int col),
                int  (*callback)(int *nchoices, int *n, int key))
{
    int     fline, cursor, shift, row, selection, ind, mou_r, mou_c,mp;
    int     i, r1, r2, c1, c2, key;
    int     moudrag = -1;
    int     pagesize, scrollbar, slider_start=0, slider_end=0, slider_len;
    char    ch;

    // make a selection
    fline    = 0;
    shift    = 0;
    cursor   = 0;
    selection  = -2;

    if (startpos > 0 && startpos < *nchoices) cursor = startpos;
    if (fline > cursor) fline = cursor;

    menu_disable ();
    do
    {
        pagesize = video_vsize()-6;
        r1 = 2; r2 = video_vsize()-3;
        if (flags & CHOOSE_SHIFT_LEFT)
        {
            c1 = 4; c2 = video_hsize() - 5 - video_hsize()/4;
        }
        else if (flags & CHOOSE_SHIFT_RIGHT)
        {
            c1 = 4 + video_hsize()/4; c2 = video_hsize() - 5;
        }
        else
        {
            c1 = 4; c2 = video_hsize()-5;
        }

        if (cursor >= *nchoices) cursor = *nchoices - 1;
        if (fline+pagesize <= cursor) fline = cursor - pagesize/2 + 1;
        if (fline > cursor) fline = cursor;
        if (fline < 0) fline = 0;
        if (cursor < 0 && *nchoices > 0) cursor = 0;

        video_clear (r1, c1, r2, c2, fl_clr.selbox_back);
        fly_drawframe (r1, c1, r2, c2);
        video_put_str (header, strlen(header), r1, c1+1);

        // draw scrollbar
        if (*nchoices > pagesize)
        {
            slider_len = pagesize*(pagesize-2)/(*nchoices);
            slider_start = fline*(pagesize-2)/(*nchoices) + 1;
            slider_end = slider_start + slider_len;
            for (i=1; i<pagesize-1; i++)
            {
                if (i < slider_start || i > slider_end) ch = fl_sym.fill1;
                else ch = fl_sym.fill2;
                video_put_n_char (ch, fl_opt.sb_width, r1+1+i, c2-fl_opt.sb_width);
            }
            if (fl_opt.sb_width == 1)
            {
                video_put_n_char (fl_sym.arrow_up, 1, r1+1+0, c2-1);
                video_put_n_char (fl_sym.arrow_down, 1, r1+1+pagesize-1, c2-1);
            }
            else
            {
                video_put_n_char ('/',  1, r1+1+0, c2-fl_opt.sb_width);
                video_put_n_char ('\\', 1, r1+1+0, c2-1);
                video_put_n_char ('\\', 1, r1+1+pagesize-1, c2-fl_opt.sb_width);
                video_put_n_char ('/',  1, r1+1+pagesize-1, c2-1);
            }
            scrollbar = TRUE;
        }
        else
            scrollbar = FALSE;

        // draw choices
        for (row=r1+1; row<r2; row++)
        {
            ind = row-r1-1+fline;
            if (ind < *nchoices)
                (*drawline) (ind, scrollbar ? c2-c1-2-fl_opt.sb_width : c2-c1-1,
                             shift, ind==cursor, row, c1+1);
        }

        video_update (0);
        key = getmessage (-1);

        if (callback != NULL)
            selection = (*callback) (nchoices, &cursor, key);
        if (selection == -3)
        {
            selection = -2;
            continue;
        }
        if (selection != -2) break;

        if (IS_KEY(key))
        {
            switch (key)
            {
            case _Home:
            case _End:
                shift = 0;
            case _Up:
            case _Down:
            case _PgUp:
            case _PgDn:
                fly_scroll_it (key, &fline, &cursor, *nchoices, pagesize);
                break;

            case _Left:
                if (flags & CHOOSE_LEFT_IS_ESC)
                {
                    selection = -1;
                    break;
                }
                if (flags & CHOOSE_ALLOW_HSCROLL)
                {
                    shift = max1 (0, shift-1);
                    break;
                }
                if (callback != NULL)
                    selection = (*callback) (nchoices, &cursor, key);
                break;

            case _Right:
                if (flags & CHOOSE_RIGHT_IS_ENTER)
                {
                    selection = cursor;
                    break;
                }
                if (flags & CHOOSE_ALLOW_HSCROLL)
                {
                    shift = shift+1;
                    break;
                }
                if (callback != NULL)
                    selection = (*callback) (nchoices, &cursor, key);
                break;

            case _CtrlLeft:
            case '<':
                shift = max1 (0, shift-10);
                break;

            case _CtrlRight:
            case '>':
                shift = shift+10;
                break;

            case _Esc:
                selection = -1;
                break;

            case _Enter:
                selection = cursor;
                break;
            }
        }

        if (IS_MOUSE (key))
        {
            mou_r = MOU_Y(key);
            mou_c = MOU_X(key);
            // mouse events on selection entries
            if (mou_r > r1 && mou_r < r2 && mou_c > c1+1 &&
                mou_c < c2-1-(scrollbar ? fl_opt.sb_width : 0))
            {
                switch (MOU_EVTYPE(key))
                {
                case MOUEV_B1SC:
                    mp = mou_r-r1-1 + fline;
                    if (mp < *nchoices)
                        cursor = mp;
                    break;

                case MOUEV_B1DC:
                    mp = mou_r-r1-1 + fline;
                    if (mp < *nchoices)
                        selection = mp;
                    break;
                }
            }

            // mouse events on a scrollbar
            if (scrollbar && mou_r > r1 && mou_r < r2 &&
                mou_c >= c2-fl_opt.sb_width && mou_c <= c2-1)
            {
                switch (MOU_EVTYPE(key))
                {
                case MOUEV_B1SC:
                case MOUEV_B1DC:
                    if (mou_r == r1+1)
                        fly_scroll_it (_Scroll1Up, &fline, &cursor, *nchoices, pagesize);
                    else if (mou_r == r2-1)
                        fly_scroll_it (_Scroll1Down, &fline, &cursor, *nchoices, pagesize);
                    else if (mou_r-r1-1 < slider_start)
                        fly_scroll_it (_ScrollPgUp, &fline, &cursor, *nchoices, pagesize);
                    else if (mou_r-r1-1 > slider_end)
                        fly_scroll_it (_ScrollPgDown, &fline, &cursor, *nchoices, pagesize);
                    break;

                case MOUEV_B1AC:
                    if (mou_r == r1+1)
                        fly_scroll_it (_Scroll1Up, &fline, &cursor, *nchoices, pagesize);
                    else if (mou_r == r2-1)
                        fly_scroll_it (_Scroll1Down, &fline, &cursor, *nchoices, pagesize);
                    else if (mou_r-r1-1 < slider_start)
                        fly_scroll_it (_ScrollPgUp, &fline, &cursor, *nchoices, pagesize);
                    else if (mou_r-r1-1 > slider_end)
                        fly_scroll_it (_ScrollPgDown, &fline, &cursor, *nchoices, pagesize);
                    break;

                case MOUEV_B1MS:
                    if (mou_r-r1-1 >= slider_start && mou_r-r1-1 <= slider_end)
                        moudrag = mou_r-r1-1 - slider_start;
                    break;

                case MOUEV_B1DR:
                    fline = (mou_r-r1-1 - moudrag)*(*nchoices)/(pagesize-2);
                    if (cursor < fline) cursor = fline;
                    if (cursor > fline+pagesize-1) cursor = fline+pagesize-1;
                    moudrag = mou_r-r1-1 - slider_start;
                    break;
                }
            }

            // common events
            switch (MOU_EVTYPE(key))
            {
            case MOUEV_B2SC:
                selection = -1;
                break;

            case MOUEV_B1ME:
                moudrag = -1;
                break;
            }
        }
    }
    while (selection == -2);
    menu_enable ();

    return selection;
}

/* ------------------------------------------------------------- */

void fly_scroll_it (int key, int *first, int *current, int n, int page)
{
    if (n == 0) return;

    switch (key)
    {
    case _Up:
        *current = max1 (0, *current-1);
        *first = min1 (*first, *current);
        break;

    case _Down:
        *current = min1 (n-1, *current+1);
        *first = max1 (*first, min1 (*first+1, *current-page+1));
        break;

    case _Home:
        *current = 0;
        *first = 0;
        break;

    case _End:
        *current = n-1;
        *first = max1 (0, *current-page+1);
        break;

    case _PgUp:
        *current = max1 (0, *current-page+1);
        *first = min1 (*first, *current);
        break;

    case _PgDn:
        *current = min1 (n-1, *current+page-1);
        *first = max1 (*first, *current-page+1);
        break;

    case _Scroll1Up:
        if (*first == 0)
        {
            *current = max1 (*current-1, 0);
        }
        else
        {
            *first = max1 (*first-1, 0);
            *current = min1 (*current, *first+page-1);
        }
        break;

    case _Scroll1Down:
        if (*first == n-page)
        {
            *current = min1 (*current+1, n-1);
        }
        else
        {
            *first = min1 (*first+1, n-page);
            *current = max1 (*current, *first);
        }
        break;

    case _ScrollPgUp:
        *first = max1 (*first-page+1, 0);
        *current = max1 (*current-page, 0);
        break;

    case _ScrollPgDown:
        *first = min1 (*first+page-1, n-page);
        *current = min1 (*current+page, n-1);
        break;
    }
}

/* ------------------------------------------------------------- */
int fly_ask_ok (int flags, char *format, ...)
{
    va_list       args;
    char          buffer[8192];

    if (!fl_opt.initialized)
    {
        va_start (args, format);
        vfprintf (stderr, format, args);
        va_end (args);
        return 0;
    }
    else
    {
        va_start (args, format);
        vsnprintf1 (buffer, sizeof(buffer), format, args);
        va_end (args);
        if (fl_opt.use_gui_controls)
            return ifly_ask_gui (flags, buffer, fl_str.ok);
        else
            return ifly_ask_txt (flags, buffer, fl_str.ok);
    }
}

/* ------------------------------------------------------------- */
int fly_ask_ok_cancel (int flags, char *format, ...)
{
    va_list       args;
    char          buffer[8192], buffer2[8192];

    if (!fl_opt.initialized) return 1;

    va_start (args, format);
    vsnprintf1 (buffer, sizeof(buffer), format, args);
    va_end (args);

    snprintf1 (buffer2, sizeof(buffer2), "%s\n%s", fl_str.ok, fl_str.cancel);

    if (fl_opt.use_gui_controls)
        return ifly_ask_gui (flags, buffer, buffer2);
    else
        return ifly_ask_txt (flags, buffer, buffer2);
}

/* -------------------------------------------------------------
returns: 0 - esc hit; 1...n - choice is selected */
int fly_ask (int flags, char *format, char *answers, ...)
{
    va_list       args;
    char          buffer[8192], buffer2[8192];

    va_start (args, answers);
    vsnprintf1 (buffer, sizeof(buffer), format, args);
    va_end (args);

    if (answers != NULL)
    {
        str_scopy (buffer2, answers);
    }
    else
    {
        if (flags & ASK_YN)
            snprintf1 (buffer2, sizeof(buffer2), "%s\n%s", fl_str.yes, fl_str.no);
        else
            strcpy (buffer2, fl_str.ok);
    }

    if (fl_opt.use_gui_controls)
        return ifly_ask_gui (flags, buffer, buffer2);
    else
        return ifly_ask_txt (flags, buffer, buffer2);
}

/* -------------------------------------------------------------  */

void fly_error (char *format, ...)
{
    va_list       args;
    char          buffer[4096];

    va_start (args, format);
    vsnprintf1 (buffer, sizeof(buffer), format, args);
    va_end (args);

    if (fl_opt.use_gui_controls)
        ifly_error_gui (buffer);
    else
        ifly_error_txt (buffer);

    exit (1);
}
