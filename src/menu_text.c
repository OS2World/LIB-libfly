#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "menu.h"

static int mouse_clicked = -1;

static int  process_box_menu (struct _item *L, struct _item *M, int row, int col);
static void draw_upper_line (struct _item *L, int highlight);
static int  find_line_item (struct _item *L, int key);
    
/* ------------------------------------------------------------------------ */

void advance_pointer (struct _item *I, int n, int *pointer, int incr)
{
    int d;

    if (*pointer == 0 && incr == -1)
    {
        *pointer = n-1;
    }
    else if (*pointer == n-1 && incr == +1)
    {
        *pointer = 0;
    }
    else
    {
        *pointer += incr;
        if (*pointer < 0)   *pointer = 0;
        if (*pointer > n-1) *pointer = n-1;

        d = (incr > 0) ? 1 : -1;
        if (I[*pointer].type == ITEM_TYPE_SEP) (*pointer) += d;
    }
}

/* ------------------------------------------------------------------------ */

void draw_line_item (struct _item *I, int pointer)
{
    int     col = 0;
    char    attr1, attr2;

    col = I->pos;
    if (menu_enabled)
    {
        if (I->enabled)
        {
            attr1 = pointer ? fl_clr.menu_cursor : fl_clr.menu_main;
            attr2 = pointer ? fl_clr.menu_cursor_hotkey : fl_clr.menu_main_hotkey;
        }
        else
        {
            attr1 = pointer ? fl_clr.menu_cursor_disabled : fl_clr.menu_main_disabled;
            attr2 = attr1;
        }
    }
    else
    {
        attr1 = pointer ? fl_clr.menu_cursor_disabled : fl_clr.menu_main_disabled;
        attr2 = attr1;
    }

    // fill the place with required attribute
    video_put_n_cell (' ', attr1, strlen(I->text)+2, 0, col);

    // draw the menu item
    video_put_str (I->text, strlen(I->text), 0, col+1);

    // draw hotkey if necessary
    if (I->hotkey && I->enabled)
        video_put_n_attr (attr2, 1, 0, col+1+I->hkpos);
}

/* ------------------------------------------------------------------------ */

void draw_box_item (struct _item *I, int pointer, int row, int col, int width)
{
    char    attr1, attr2;

    if (I->enabled)
    {
        attr1 = pointer ? fl_clr.menu_cursor : fl_clr.menu_main;
        attr2 = pointer ? fl_clr.menu_cursor_hotkey : fl_clr.menu_main_hotkey;
    }
    else
    {
        attr1 = pointer ? fl_clr.menu_cursor_disabled : fl_clr.menu_main_disabled;
        attr2 = attr1;
    }

    // fill the place with required attribute
    video_put_n_cell (' ', attr1, width+1, row, col);

    // draw the menu item
    if (I->type == ITEM_TYPE_SEP)
    {
        video_put_n_char (fl_sym.t_l, 1, row, col-1);
        video_put_n_char (fl_sym.h,   width+1, row, col);
        video_put_n_char (fl_sym.t_r, 1, row, col+width+1);
    }
    else
    {
        video_put_str (I->text, strlen(I->text), row, col+1);
        video_put_str (I->keydesc, strlen(I->keydesc),
                       row, col+1+width-strlen(I->keydesc)-1);

        // draw arrow when submenu
        if (I->type == ITEM_TYPE_SUBMENU)
            video_put_str ("->", 2, row, col+1+width-3);
            
        // draw hotkey if necessary
        if (I->hotkey && I->enabled)
            video_put_n_attr (attr2, 1, row, col+1+I->hkpos);

        // draw switch mark if necessary
        if (I->type == ITEM_TYPE_SWITCH && I->state)
            video_put_n_char (fl_sym.marksign, 1, row, col);
    }
}

/* ------------------------------------------------------------------------ */

static int process_box_menu (struct _item *L, struct _item *M, int row0, int col0)
{
    int   i, k, rc, pointer, nlines, width, l, action;
    int   r1, c1, r2, c2, mou_r, mou_c, mp;
    char  *saved;

    debug_tools ("-> process_box_menu()\n");
    pointer = 0;

    // compute box dimensions
    nlines = 0;
    width  = 0;
    while (M[nlines].type != ITEM_TYPE_END) nlines++;
    for (i=0; i<nlines; i++)
    {
        l = strlen(M[i].text) + strlen(M[i].keydesc);
        width = width > l ? width : l;
    }
    width += 2;

    // check whether we fit into the screen and fix if necessary
    if (col0+width+2 >= video_hsize())
        col0 = max1 (0, video_hsize() - width - 2 - 1);

    // Draw menu box
    r1 = row0;   r2 = row0+nlines+1;
    c1 = col0-1; c2 = col0+width+2;
    saved = video_save (r1, c1, r2+1, c2+2);
    
    rc = -1000;
    
    while (rc == -1000)
    {
        video_clear (r1, c1, r2, c2, fl_clr.menu_main);
        fly_drawframe (r1, c1, r2, c2);

        for (i=0; i<nlines; i++)
            draw_box_item (M+i, pointer == i, row0+i+1, col0, width+1);

        video_update (0);
        k = getmessage (-1);
        if (IS_KEY(k))
        {
            switch (k)
            {
            case _Home:
                pointer = 0;
                advance_pointer (M, nlines, &pointer, 0);
                break;

            case _End:
                pointer = nlines-1;
                advance_pointer (M, nlines, &pointer, 0);
                break;

            case _PgDn:
                advance_pointer (M, nlines, &pointer, 5);
                break;

            case _PgUp:
                advance_pointer (M, nlines, &pointer, -5);
                break;

            case _Down:
                advance_pointer (M, nlines, &pointer, 1);
                break;

            case _Up:
                advance_pointer (M, nlines, &pointer, -1);
                break;

            case _Left:
                rc = MENU_RC_PREV_FIELD;
                break;

            case _Right:
                rc = MENU_RC_NEXT_FIELD;
                break;

            case _Esc:
                rc = MENU_RC_CANCELLED;
                break;

            case _Enter:
                if (!M[pointer].enabled) break;
                switch (M[pointer].type)
                {
                case ITEM_TYPE_ACTION:
                case ITEM_TYPE_SWITCH:
                    rc = M[pointer].body.action;
                    break;
                case ITEM_TYPE_SUBMENU:
                    rc = process_box_menu (L, M[pointer].body.submenu,
                                           r1+pointer, c2+1);
                    if (rc == MENU_RC_CANCELLED ||
                        rc == MENU_RC_PREV_FIELD) rc = -1000;
                    break;
                }
                break;

            default:
                // look for hotkeys
                if (k <= 256)
                {
                    for (i=0; i<nlines; i++)
                    {
                        if (M[i].enabled && M[i].hotkey &&
                            tolower1(k) == tolower1(M[i].hotkey))
                        {
                            switch (M[i].type)
                            {
                            case ITEM_TYPE_ACTION:
                            case ITEM_TYPE_SWITCH:
                                rc = M[i].body.action;
                                break;
                            case ITEM_TYPE_SUBMENU:
                                rc = process_box_menu (L, M[i].body.submenu,
                                                       r1+i, c2+1);
                                if (rc == MENU_RC_CANCELLED ||
                                    rc == MENU_RC_PREV_FIELD) rc = -1000;
                                break;
                            }
                            break;
                        }
                    }
                }
                // look for action keys
                //action = keytable[k] != KEY_NOTHING ? keytable[k] : keytable2[k];
                action = keytable0[k];
                if (action != KEY_NOTHING && (k > 256 || k < 32))
                {
                    for (i=0; i<nlines; i++)
                    {
                        if (M[i].enabled && action == M[i].body.action)
                        {
                            rc = action;
                            break;
                        }
                    }
                }
            }
        }

        if (IS_MOUSE (k))
        {
            mou_r = MOU_Y(k);
            mou_c = MOU_X(k);
            debug_tools ("menu: mouse message: %d (%d, %d) row %d (%d, %d) column\n",
                         mou_r, r1, r2, mou_c, c1, c2);

            // check for button2 clicks (cancel)
            if (MOU_EVTYPE(k) == MOUEV_B2SC)
                rc = MENU_RC_CANCELLED;

            // look for clicks in the top menu
            if (rc == -1000 && mou_r == 0)
            {
                // check: same submenu?
                i = find_line_item (L, k);
                if (i != -1 && L[i].type == ITEM_TYPE_SUBMENU && L[i].body.submenu != M)
                {
                    switch (MOU_EVTYPE(k))
                    {
                    case MOUEV_MOVE:
                        if (saved != NULL) video_restore (saved);
                        saved = NULL;
                        draw_upper_line (L, i);
                        action = process_box_menu (L, L[i].body.submenu, 1, L[i].pos);
                        if (action != 0) rc = action;
                        else             rc = MENU_RC_CANCELLED;
                        break;
                    }
                }
            }
            
            // handle clicks within the same box menu
            if (rc == -1000 && mou_r > r1 && mou_r < r2 && mou_c > c1 && mou_c < c2)
            {
                switch (MOU_EVTYPE(k))
                {
                case MOUEV_B1UP:
                    mp = mou_r-r1-1;
                    if (!M[mp].enabled) break;
                    switch (M[mp].type)
                    {
                    case ITEM_TYPE_ACTION:
                    case ITEM_TYPE_SWITCH:
                        for (i=0; i<nlines; i++)
                            draw_box_item (M+i, mp == i, row0+i+1, col0, width+1);
                        rc = M[mp].body.action;
                        break;
                    case ITEM_TYPE_SUBMENU:
                        rc = process_box_menu (L, M[mp].body.submenu, r1+mp, c2+1);
                        if (rc == MENU_RC_CANCELLED || rc == MENU_RC_PREV_FIELD) rc = -1000;
                        break;
                    }
                    break;
                    
                case MOUEV_MOVE:
                    mp = mou_r-r1-1;
                    if (M[mp].type == ITEM_TYPE_SEP) break;
                    pointer = mou_r-r1-1;
                }
            }

            // finally, clicks outside menu area will close menu
            if (rc == -1000 && mou_r != 0 && (mou_r < r1 || mou_r > r2 || mou_c < c1 || mou_c > c2))
            {
                switch (MOU_EVTYPE(k))
                {
                case MOUEV_B1SC:
                case MOUEV_B2SC:
                case MOUEV_B3SC:
                    rc = MENU_RC_CANCELLED;
                    break;
                }
            }
        }
    }

    if (saved != NULL) video_restore (saved);
    //video_update (0);
    debug_tools ("<- process_box_menu(): returning %d\n", rc);
    return rc;
}

/* ------------------------------------------------------------------------
 with
 init=-2:   just draw the line and return
 init=-1:   enter the line menu
 init=0..n: enter nth submenu
 returns action selected, or <0 if cancelled */

int menu_process_line (struct _item *L, int init, int opendrop)
{
    int  i, k, rc, rc1, pointer, npos, mpointer;
    char *saved = NULL;

    if (L == NULL) return MENU_RC_CANCELLED;

    if (init != -2)
        saved = video_save (0, 0, 0, video_hsize()-1);

    npos = 0;
    while (L[npos].type != ITEM_TYPE_END) npos++;

    if (init >= 0)
        pointer = init > npos-1 ? npos-1 : init;
    else
        pointer = 0;
    rc  = -1000;
    k   = -1000;
    do
    {
        rc1 = -1000;
        draw_upper_line (L, init == -2 ? -1 : pointer);
        if (init == -2) return -1;

        video_update (0);
        if (k == -1000 && opendrop)
            k = _Enter;
        else
            k = getmessage (-1);
        
        if (IS_KEY (k))
        {
            switch (k)
            {
            case _Home:
                pointer = 0;
                break;

            case _End:
                pointer = npos-1;
                break;

            case _Right:
                advance_pointer (L, npos, &pointer, 1);
                break;

            case _Left:
                advance_pointer (L, npos, &pointer, -1);
                break;

            case _Esc:
                rc = MENU_RC_CANCELLED;
                break;

            case _Enter:
            case _Down:
                if (!L[pointer].enabled) break;
                switch (L[pointer].type)
                {
                case ITEM_TYPE_ACTION:
                case ITEM_TYPE_SWITCH:
                    rc = L[pointer].body.action;
                    break;
                case ITEM_TYPE_SUBMENU:
                    rc1 = process_box_menu (L, L[pointer].body.submenu, 1, L[pointer].pos);
                    break;
                }
                break;
    
            default:
                // look for hotkeys
                if (k > 256) break;
                for (i=0; i<npos; i++)
                {
                    if (L[i].enabled && L[i].hotkey &&
                        tolower1(k) == tolower1(L[i].hotkey))
                    {
                        switch (L[i].type)
                        {
                        case ITEM_TYPE_ACTION:
                        case ITEM_TYPE_SWITCH:
                            rc = L[i].body.action;
                            break;
                        case ITEM_TYPE_SUBMENU:
                            rc1 = process_box_menu (L, L[pointer].body.submenu, 1, L[pointer].pos);
                            pointer = i;
                            break;
                        }
                        break;
                    }
                }
            }
        }
        else if (IS_MOUSE (k))
        {
            if (MOU_Y(k) != 0) rc1 = MENU_RC_CANCELLED;
            if (MOU_Y(k) == 0)
            {
                switch (MOU_EVTYPE(k))
                {
                case MOUEV_MOVE:
                    i = find_line_item (L, k);
                    if (i != -1) pointer = i;
                    break;

                case MOUEV_B1DN:
                    i = find_line_item (L, k);
                    if (i != -1)
                        rc1 = process_box_menu (L, L[pointer].body.submenu, 1, L[pointer].pos);
                }
            }
        }

    again:
        if (rc1 != -1000)
        {
            switch (rc1)
            {
            case MENU_RC_CANCELLED:
                rc = MENU_RC_CANCELLED;
                break;

            case MENU_RC_CANCELLED2:
                rc = MENU_RC_CANCELLED;
                break;

            case MENU_RC_NEXT_FIELD:
                advance_pointer (L, npos, &pointer, 1);
                draw_upper_line (L, pointer);
                rc1 = process_box_menu (L, L[pointer].body.submenu, 1, L[pointer].pos);
                goto again;
                break;

            case MENU_RC_PREV_FIELD:
                advance_pointer (L, npos, &pointer, -1);
                draw_upper_line (L, pointer);
                rc1 = process_box_menu (L, L[pointer].body.submenu, 1, L[pointer].pos);
                goto again;
                break;

            case MENU_RC_MOUSE_EXIT:
                mpointer = -1;
                for (i=0; i<npos; i++)
                {
                    if (L[i].enabled && mouse_clicked >= L[i].pos &&
                        mouse_clicked <= L[i].pos+strlen(L[i].text)+2)
                    {
                        switch (L[i].type)
                        {
                        case ITEM_TYPE_ACTION:
                        case ITEM_TYPE_SWITCH:
                            rc = L[i].body.action;
                            break;
                        case ITEM_TYPE_SUBMENU:
                            mpointer = i;
                            break;
                        }
                    }
                }
                if (mpointer == -1) rc = MENU_RC_CANCELLED;
                else                pointer = mpointer;
                break;

            default:
                if (rc1 > 0) rc = rc1;
            }
        }
    }
    while (rc == -1000);
    
    video_restore (saved);
    //video_update (0);
    return rc;
}

/* ------------------------------------------------------------- */

void menu_chstatus_on (void)
{
    int i, j;
    
    for (i=0; i<ngroup; i++)
    {
        j = 0;
        while (mn[i].I[j].type != ITEM_TYPE_END)
        {
            if (mn[i].I[j].type == ITEM_TYPE_ACTION ||
                mn[i].I[j].type == ITEM_TYPE_SWITCH)
                mn[i].I[j].enabled = 1;
            j++;
        }
    }
}

/* ------------------------------------------------------------- */

void menu_activate (struct _item *L)
{
    fly_active_menu = L;
}

/* ------------------------------------------------------------- */

void menu_deactivate (struct _item *L)
{
    fly_active_menu = NULL;
}

/* --------------------------------------------------------------------- */

void menu_enable (void)
{
    menu_enabled = TRUE;
}

/* --------------------------------------------------------------------- */

void menu_disable (void)
{
    menu_enabled = FALSE;
    if (fly_active_menu == NULL || !fl_opt.menu_onscreen) return;
    draw_upper_line (fly_active_menu, -1);
}

/* ------------------------------------------------------------- */

void menu_chstatus (int actions[], int n, int newstatus)
{
    int i, j, k, action;
    
    for (i=0; i<ngroup; i++)
    {
        j = 0;
        while (mn[i].I[j].type != ITEM_TYPE_END)
        {
            if (mn[i].I[j].type == ITEM_TYPE_ACTION ||
                mn[i].I[j].type == ITEM_TYPE_SWITCH)
            {
                action = mn[i].I[j].body.action;
                for (k=0; k<n; k++)
                    if (action == actions[k])
                        mn[i].I[j].enabled = newstatus;
            }
            j++;
        }
    }
}

/* ------------------------------------------------------------- */

struct
{
    int alt;
    int ch;
}
alt2char[] =
{
    {_AltA, 'a'}, {_AltB, 'b'}, {_AltC, 'c'}, {_AltD, 'd'}, {_AltE, 'e'},
    {_AltF, 'f'}, {_AltG, 'g'}, {_AltH, 'h'}, {_AltI, 'i'}, {_AltJ, 'j'},
    {_AltK, 'k'}, {_AltL, 'l'}, {_AltM, 'm'}, {_AltN, 'n'}, {_AltO, 'o'},
    {_AltP, 'p'}, {_AltQ, 'q'}, {_AltR, 'r'}, {_AltS, 's'}, {_AltT, 't'},
    {_AltU, 'u'}, {_AltV, 'v'}, {_AltW, 'w'}, {_AltX, 'x'}, {_AltY, 'y'},
    {_AltZ, 'z'}
};

int  menu_check_key (struct _item *L, int key)
{
    int  action, i, j, npos;
    
    if (!menu_enabled) return 0;
    if (L == NULL) return 0;

    action = 0;
    npos = 0;
    while (L[npos].type != ITEM_TYPE_END) npos++;
    
    if (IS_KEY (key))
    {
        for (i=0; i<npos; i++)
        {
            if (L[i].enabled)
                for (j=0; j<sizeof(alt2char)/sizeof(alt2char[0]); j++)
                    if (alt2char[j].ch == tolower1 (L[i].hotkey))
                        if (key == alt2char[j].alt)
                        {
                            action = menu_process_line (L, i, 1);
                            return action;
                        }
        }
    }

    if (IS_MOUSE(key) && MOU_Y(key) == 0)
    {
        debug_tools ("event %d\n", MOU_EVTYPE(key));
        switch (MOU_EVTYPE(key))
        {
        case MOUEV_B1DN:
            i = find_line_item (L, key);
            debug_tools ("button 1 down; item %d\n", i);
            if (i != -1)
            {
                action = menu_process_line (L, i, 1);
                return action;
            }
            break;
            
        case MOUEV_MOVE:
            if (!fl_opt.menu_hotmouse) break;
            i = find_line_item (L, key);
            if (i != -1)
            {
                action = menu_process_line (L, i, 0);
                return action;
            }
            break;
        }
    }
    return action;
}

/* ------------------------------------------------------------- */

static void draw_upper_line (struct _item *L, int highlight)
{
    int i;
    
    video_put_n_cell (' ', fl_clr.menu_main, video_hsize(), 0, 0);
        
    /* Draw upper screen line */
    for (i=0; L[i].type != ITEM_TYPE_END; i++)
        draw_line_item (L+i, highlight == i);
}

/* ------------------------------------------------------------- */

static int find_line_item (struct _item *L, int key)
{
    int i, npos;

    debug_tools ("find_line_item: key = %d\n", key);
    npos = 0;
    while (L[npos].type != ITEM_TYPE_END) npos++;
    //debug_tools ("npos = %d\n", npos);
    
    for (i=0; i<npos; i++)
    {
        //debug_tools ("i = %d; npos = %d, pos = %d\n", i, L[i].pos);
        if (L[i].enabled && MOU_X(key) >= L[i].pos &&
            MOU_X(key) <= L[i].pos+strlen(L[i].text)+2)
        {
            //debug_tools ("returning %d\n", i);
            return i;
        }
    }
    //debug_tools ("haven't found anything\n");
    return -1;
}

/* ------------------------------------------------------------- */

void menu_set_switch (int action, int newstate)
{
    int i, j;
    
    for (i=0; i<ngroup; i++)
    {
        j = 0;
        while (mn[i].I[j].type != ITEM_TYPE_END)
        {
            if (mn[i].I[j].type == ITEM_TYPE_SWITCH &&
                mn[i].I[j].body.action == action)
            {
                mn[i].I[j].state = newstate;
            }
            j++;
        }
    }
}

