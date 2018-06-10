#include "fly/fly.h"
#include <string.h>

/* ------------------------------------------------------------- */

int ifly_ask_txt (int flags, char *questions, char *answers)
{
    int           lines, cols, ch, key, mw, slack;
    char          *savebuf, *p, *p1;
    int           i, choice, numchoices, cursor;
    int           r1, c1, r2, c2, r, c, mou_r, mou_c;
    int           ch_pos[16], ch_len[16], ch_ind[16], chlen, chskip;
    char          attr_text;
    
    if (flags & ASK_WARN)
        attr_text = fl_clr.dbox_back_warn;
    else
        attr_text = fl_clr.dbox_back;
    
    // determine sizes
    p = questions;   lines = 0;   mw = 0;   ch = 0;
    while (1)
    {
        if (*p=='\n' || *p==0)
        {
            mw = max1 (mw, ch);
            lines++;
            ch = 0;
        }
        else
            ch++;
        if (*p++ == 0) break;
    }
    lines = min1 (lines, video_vsize()-4);
    lines = max1 (1, lines);
    cols  = max1 (mw, strlen (answers) + 4);
    cols += 10;
    cols  = max1 (40, cols);
    cols  = min1 (cols, video_hsize()-2);
    slack = max1 (0, (cols - mw)/2);
    
    menu_disable ();
    // save screen before putting text
    savebuf = video_save (0, 0, video_vsize()-1, video_hsize()-1);
    
    // compute corner coordinates
    r1 = (video_vsize()-lines-4)/2;
    r2 = r1 + lines + 4 - 1;
    c1 = (video_hsize()-cols-2)/2;
    c2 = c1 + cols + 2 - 1;
    
    // clear an area and draw frames
    video_clear (r1, c1, r2, c2, attr_text);
    fly_drawframe (r1, c1, r2, c2);
    video_put_n_char (fl_sym.h,   c2-c1-1, r2-2, c1+1);
    video_put_n_char (fl_sym.t_l, 1, r2-2, c1);
    video_put_n_char (fl_sym.t_r, 1, r2-2, c2);

    // draw description lines
    p  = questions;  p1 = p;   r = r1+1;
    while (1)
    {
        if (*p=='\n' || *p==0)
        {
            video_put_str_attr (p1, p-p1, r++, c1+1+slack, attr_text);
            p1 = p + 1;
        }
        if (*p++ == 0) break;
    }
    
    // parse 'answers' string
    p = answers; numchoices = 0; p1 = p; c = c1+2;
    while (1)
    {
        if (*p=='\n' || *p==0)
        {
            ch_ind[numchoices] = p1 - answers;
            ch_pos[numchoices] = c;
            ch_len[numchoices] = p - p1;
            c += p - p1 + 1;
            p1 = p + 1;
            numchoices++;
        }
        if (*p++ == 0) break;
    }

    // compute placements
    chlen = 1;
    for (i=0; i<numchoices; i++)
        chlen += ch_len[i] + 1;
    chskip = (c2-c1-1-chlen)/2;
    for (i=0; i<numchoices; i++)
        ch_pos[i] += chskip;

    // place actual strings
    for (i=0; i<numchoices; i++)
        video_put_str_attr (answers+ch_ind[i], ch_len[i],
                            r2-1, ch_pos[i], attr_text);

    // handle keystrokes
    choice = -1;
    cursor = 1;
    do
    {
        // set attributes according cursor
        for (i=0; i<numchoices; i++)
            video_put_n_attr (cursor-1==i ? fl_clr.dbox_sel : attr_text,
                              ch_len[i], r2-1, ch_pos[i]);
        video_update (0);
        key = getmessage (-1);
        if (IS_KEY(key))
        {
            switch (key)
            {
            case _F1:
                break;

            case _Left:
                cursor = (cursor > 1) ? cursor-1 : numchoices;
                break;

            case _Right:
                cursor = (cursor < numchoices) ? cursor+1 : 1;
                break;

            case _Esc:
                choice = 0;
                break;

            case _Enter:
                choice = cursor;
                break;
            }
        }

        if (IS_MOUSE (key))
        {
            switch (MOU_EVTYPE(key))
            {
            case MOUEV_B1SC:
                mou_r = MOU_Y (key);
                mou_c = MOU_X (key);
                //debug_tools ("button1 down; r = %d, c = %d\n", mou_r, mou_c);
                if (mou_r == r2-1)
                    for (i=0; i<numchoices; i++)
                        if (mou_c >= ch_pos[i] && mou_c < ch_pos[i]+ch_len[i])
                            choice = i+1;
                break;

            case MOUEV_B2SC:
                choice = 0;
                break;
            }
        }
    }
    while (choice == -1);
    
    // restore screen and exit
    video_restore (savebuf);
    menu_enable ();
    video_update (0);
    if ((flags & ASK_YN) && choice == numchoices) choice = 0;
    return choice;
}

/* ------------------------------------------------------------- */

void ifly_error_txt (char *message)
{
    if (fl_opt.has_console)
    {
        if (fl_opt.initialized) fly_terminate ();
        fprintf (stderr, "%s", message);
    }
    else
    {
        if (fl_opt.initialized)
            ifly_ask_txt (ASK_WARN, message, fl_str.ok);
    }
}
