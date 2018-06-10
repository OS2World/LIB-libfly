/* ------------------------------------------------------------- */

static int getct (int interval)
{
    fd_set          rdfs;
    struct timeval  tv;
    int             rc;

    if (!isatty (STDIN_FD))
    {
        if (interval < 0)
        {
            fly_terminate ();
            exit (0);
        }
        tv.tv_sec = 0;
        tv.tv_usec = interval*1000;
        FD_ZERO (&rdfs);
        FD_SET (STDIN_FD, &rdfs);
        select (0, NULL, NULL, NULL, &tv);
        return 0;
    }

    if (interval >= 0)
    {
        tv.tv_sec = 0;
        tv.tv_usec = interval*1000;
        FD_ZERO (&rdfs);
        FD_SET (STDIN_FD, &rdfs);

        rc = select (STDIN_FD+1, &rdfs, NULL, NULL, &tv);
        // when select() returns error, we no longer have tty
        if (rc < 0)
        {
            fly_terminate ();
            exit (0);
        }
        if (rc == 0) return 0;
    }

    return getc (stdin);
}

