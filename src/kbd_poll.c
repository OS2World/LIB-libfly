#include <unistd.h>

/* BeOS is broken */

#ifdef __BEOS__
#include <socket.h>
#include <be/kernel/OS.h>
#define usleep snooze
#endif

/* ------------------------------------------------------------- */

static int getct (int interval)
{
    double          timergoal, now;
    struct timeval  tv;
    int             key, nc;
    unsigned char   c;

    if (!isatty (STDIN_FD))
    {
        if (interval < 0)
        {
            fly_terminate ();
            exit (0);
        }
        tv.tv_sec = 0;
        tv.tv_usec = interval*1000;
        select (0, NULL, NULL, NULL, &tv);
        return 0;
    }

    // with interval == 0, we only check for keys in buffer
    if (interval == 0)
    {
        // set nonblocking read
        fcntl (STDIN_FD, F_SETFL, O_NONBLOCK);
        nc = read (STDIN_FD, &c, 1);
        fcntl (STDIN_FD, F_SETFL, 0);
        if (nc == 1) key = c;
        else         key = 0;
        return key;
    }
    
    // if there's interval specified, wait for key while interval expires
    if (interval > 0)
    {
        // set nonblocking read
        fcntl (STDIN_FD, F_SETFL, O_NONBLOCK);
        timergoal = clock1() + ((double)interval)/1000.;
        now = timergoal-1.0;
        key = 0;
        nc = 0;
        while (now < timergoal)
        {
            nc = read (STDIN_FD, &c, 1);
            if (nc == 1) break;
            if (interval > 1000 || interval < 0) sleep (1);
            else                                 usleep (100000L);
            now = clock1();
        }
        fcntl (STDIN_FD, F_SETFL, 0);
        if (nc == 1) key = c;
        return key;
    }

    key = 0;
    nc = read (STDIN_FD, &c, 1);
    if (nc == 1) key = c;
    return key;
}

