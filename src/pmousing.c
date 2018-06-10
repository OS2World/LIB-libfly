/*
 
All mouse processing is based on one fundamental message which contains
the following information:
- what buttons are down    (b1, b2, b3)
- time of the event        (t)
- coordinates              (r, c)
Resulting event is designated by ev.
Suppose current event is N, previous are P, PP.

MOUEV_MOVE P.b1==N.b1 && P.b2==N.b2 && P.b3==N.b3 && (P.r!=N.r || P.c!=N.c)

MOUEV_B1SC PP.ev==MOUEV_B1DN && P.ev==MOUEV_B1UP
MOUEV_B1DC PP.ev=MOUEV_B1SC && P.ev==MOUEV_B1SC && P.t-PP.t < dc_pause
MOUEV_B1MS P.ev==MOUEV_B1DN && (P.r!=N.r || P.c!=N.c)
MOUEV_B1ME PP.ev==MOUEV_MOVE && P.ev==MOUEV_B1UP
MOUEV_B1DN P.b1==0 && N.b1 && P.b2==N.b2 && P.b3==N.b3 && P.r==N.r && P.c==N.c
MOUEV_B1UP P.b1 && N.b1==0 && P.b2==N.b2 && P.b3==N.b3 && P.r==N.r && P.c==N.c

*/

#include "fly/fly.h"
#include "fly-internal.h"

/* --------------------------------------------------------------
 small FIFO queue implementation
 ---------------------------------------------------------------- */
#define MSG_QUEUE_SIZE   8
static struct m_event queue[MSG_QUEUE_SIZE];
static int nq = 0;

static struct m_event fifo_get (int t);
static void fifo_put (int ev, int b1, int b2, int b3, int t, int r, int c);

static struct m_event
    PPPP = {0, 0, 0, 0, 0, 0, 0},
    PPP = {0, 0, 0, 0, 0, 0, 0},
    PP = {0, 0, 0, 0, 0, 0, 0},
    P = {0, 0, 0, 0, 0, 0, 0};

void mouse_received (int b1, int b2, int b3, int t, int r, int c)
{
    static int b1_move = 0, b2_move = 0, b3_move = 0;

    //printf ("mouse: %d %d %d, %d x %d\n", b1, b2, b3, r, c);
    // discard events with same buttons and coords
    if (b1 == P.b1 && b2 == P.b2 && b3 == P.b3 && r == P.r && c == P.c)
    {
        return;
    }

    // movements
    if (r != P.r || c != P.c)
        fifo_put (MOUEV_MOVE, b1, b2, b3, t, r, c);

    // button 1
    if (b1 && !P.b1)
        fifo_put (MOUEV_B1DN, b1, b2, b3, t, r, c);
    
    if (!b1 && P.b1 && !PP.b1 &&
        r == P.r && c == P.c && t-P.t < fl_opt.mouse_singleclick)
        fifo_put (MOUEV_B1SC, b1, b2, b3, t, r, c);
    
    if (!b1 && P.b1 && !PP.b1 && PPP.b1 && !PPPP.b1 &&
        r == P.r && c == P.c && r == PP.r && c == PP.c &&
        r == PPP.r && c == PPP.c && P.t-PP.t < fl_opt.mouse_doubleclick)
        fifo_put (MOUEV_B1DC, b1, b2, b3, t, r, c);
    
    if (!b1 && P.b1)
    {
        fifo_put (MOUEV_B1UP, b1, b2, b3, t, r, c);
        if (b1_move)
            b1_move = 0, fifo_put (MOUEV_B1ME, b1, b2, b3, t, r, c);
    }

    if ((r != P.r || c != P.c) && P.b1 &&!PP.b1)
        b1_move = 1, fifo_put (MOUEV_B1MS, b1, b2, b3, t, r, c);

    // button 2
    if (b2 && !P.b2)
        fifo_put (MOUEV_B2DN, b1, b2, b3, t, r, c);
    
    if (!b2 && P.b2 && !PP.b2 &&
        r == P.r && c == P.c && t-P.t < fl_opt.mouse_singleclick)
        fifo_put (MOUEV_B2SC, b1, b2, b3, t, r, c);
    
    if (!b2 && P.b2 && !PP.b2 && PPP.b2 && !PPPP.b2 &&
        r == P.r && c == P.c && r == PP.r && c == PP.c &&
        r == PPP.r && c == PPP.c && P.t-PP.t < fl_opt.mouse_doubleclick)
        fifo_put (MOUEV_B2DC, b1, b2, b3, t, r, c);
    
    if (!b2 && P.b2)
    {
        fifo_put (MOUEV_B2UP, b1, b2, b3, t, r, c);
        if (b2_move)
            b2_move = 0, fifo_put (MOUEV_B2ME, b1, b2, b3, t, r, c);
    }

    if ((r != P.r || c != P.c) && P.b2 &&!PP.b2)
        b2_move = 1, fifo_put (MOUEV_B2MS, b1, b2, b3, t, r, c);

    // button 3
    if (b3 && !P.b3)
        fifo_put (MOUEV_B3DN, b1, b2, b3, t, r, c);
    
    if (!b3 && P.b3 && !PP.b3 &&
        r == P.r && c == P.c && t-P.t < fl_opt.mouse_singleclick)
        fifo_put (MOUEV_B3SC, b1, b2, b3, t, r, c);
    
    if (!b3 && P.b3 && !PP.b3 && PPP.b3 && !PPPP.b3 &&
        r == P.r && c == P.c && r == PP.r && c == PP.c &&
        r == PPP.r && c == PPP.c && P.t-PP.t < fl_opt.mouse_doubleclick)
        fifo_put (MOUEV_B3DC, b1, b2, b3, t, r, c);
    
    if (!b3 && P.b3)
    {
        fifo_put (MOUEV_B3UP, b1, b2, b3, t, r, c);
        if (b3_move)
            b3_move = 0, fifo_put (MOUEV_B3ME, b1, b2, b3, t, r, c);
    }

    if ((r != P.r || c != P.c) && P.b3 &&!PP.b3)
        b2_move = 1, fifo_put (MOUEV_B3MS, b1, b2, b3, t, r, c);

    PPPP = PPP, PPP = PP, PP = P;
    P.b1 = b1;   P.b2 = b2;   P.b3 = b3;   P.t = t;   P.r = r;   P.c = c;
}

/* ------------------------------------------------------- */

int mouse_check (int t)
{
    struct m_event  N;
    static int last_auto;

    // check for 'autoclick'
    if (nq == 0 && P.b1 && t-P.t > fl_opt.mouse_autorepeat &&
       t-last_auto > fl_opt.mouse_autodelay)
    {
        last_auto = t;
        return FMSG_BASE_MOUSE + FMSG_BASE_MOUSE_EVTYPE*MOUEV_B1AC +
            FMSG_BASE_MOUSE_X * min1(P.c, 999) +
            FMSG_BASE_MOUSE_Y * min1(P.r, 999);
    }

    if (nq)
    {
        N = fifo_get (t);
        //printf ("ev=%d, c=%d, r=%d\n", N.ev, N.c, N.r);
        return FMSG_BASE_MOUSE + FMSG_BASE_MOUSE_EVTYPE*N.ev +
            FMSG_BASE_MOUSE_X * min1 (N.c, 999) +
            FMSG_BASE_MOUSE_Y * min1 (N.r, 999);
    }
    
    return -1;
}

/* --------------------------------------------------------------------- */

static void fifo_put (int ev, int b1, int b2, int b3, int t, int r, int c)
{
    if (nq == MSG_QUEUE_SIZE-1) return;

    queue[nq].ev = ev;
    queue[nq].b1 = b1;
    queue[nq].b2 = b2;
    queue[nq].b3 = b3;
    queue[nq].t  = t;
    queue[nq].r  = r;
    queue[nq].c  = c;
    nq++;
}

/* --------------------------------------------------------------------- */

static struct m_event fifo_get (int t)
{
    int i;
    struct m_event result;
    
    if (nq == 0) return queue[0];
    
    result = queue[0];
    for (i=1; i<nq; i++)
        queue[i-1] = queue[i];
    nq--;
    
    return result;
}
