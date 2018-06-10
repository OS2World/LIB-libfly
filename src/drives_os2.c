#define INCL_SUB
#define INCL_DOS
#include <os2.h>

#include <stdarg.h>
#include <string.h>

/* ------------------------------------------------------------- */

/* drives start at 0 (A: - 0) */
int  change_drive (int drive)
{
    return DosSetDefaultDisk (drive+1);
}

/* ------------------------------------------------------------- */

int  query_drive (void)
{
    unsigned long  drive, drivemap;
    DosQueryCurrentDisk (&drive, &drivemap);
    return (int)drive-1;
}

/* ------------------------------------------------------------- */

unsigned long query_drivemap (void)
{
    unsigned long  drive, drivemap;
    DosQueryCurrentDisk (&drive, &drivemap);
    return drivemap;
}
