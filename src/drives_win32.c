
/* drives start at 0 (A: - 0) */
int  change_drive (int drive)
{
    char  buf[3];
    int   rc;

    if (drive < 0 || drive > 25) drive = 0;
    strcpy (buf, "A:");
    buf[0] = drive + 'A';
    rc = SetCurrentDirectory (buf) ? 0 : -1;
    debug_tools ("rc = %d after SetCurrentDirectory; error = %d\n",
                 GetLastError());
    return rc;
}

/* ------------------------------------------------------------- */

int  query_drive (void)
{
    char        buf[260];
    int		drive, rc;

    rc = GetCurrentDirectory (sizeof(buf), buf);
    debug_tools ("rc = %d after GetCurrentDirectory; error = %d\n",
                 GetLastError());
    drive = toupper (buf[0]) - 'A';
    return (drive < 0 || drive > 25) ? 0 : drive;
}

/* ------------------------------------------------------------- */

unsigned long query_drivemap (void)
{
    unsigned long map = 0L;
    int           d, type;
    char          buf[4];

    strcpy (buf, "A:\\");
    for (d=0; d<='Z'-'A'; d++)
    {
        buf[0] = 'A' + d;
        type = GetDriveType (buf);
        if (type != 0 && type != 1) map |= (1L << d);
    }
    return map;
}

