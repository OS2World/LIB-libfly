/* ------------------------------------------------------------- */

/* drives start at 0 (A: - 0) */
int  change_drive (int drive)
{
    if (drive != 2) return 1;
    return 0;
}

/* ------------------------------------------------------------- */

int  query_drive (void)
{
    return 2;
}

/* ------------------------------------------------------------- */

unsigned long query_drivemap (void)
{
    return 1L << 2;
}
