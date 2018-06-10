#define INCL_MOU
#include <os2.h>

#include <stdio.h>
#include <string.h>

void main (void)
{
    MOUEVENTINFO  event;
    HMOU          MouHandle;
    USHORT        NButtons, wait = MOU_NOWAIT, eventmask;
    char          ev_name[1024];
    int           rc;

    rc = MouOpen (NULL, &MouHandle);
    printf ("rc = %d from MouOpen\n", rc);
    if (rc) return;

    MouGetNumButtons (&NButtons, MouHandle);
    printf ("%d buttons\n", NButtons);

    rc = MouGetEventMask (&eventmask, MouHandle);
    printf ("rc = %d from MouGetEventMask\n", rc);
    eventmask = (MOUSE_MOTION | MOUSE_BN1_DOWN | MOUSE_BN2_DOWN | MOUSE_BN3_DOWN);
    eventmask |= (MOUSE_MOTION_WITH_BN1_DOWN |
                  MOUSE_MOTION_WITH_BN2_DOWN | MOUSE_MOTION_WITH_BN3_DOWN);
    rc = MouSetEventMask (&eventmask, MouHandle);
    printf ("rc = %d from MouSetEventMask\n", rc);
    rc = MouDrawPtr (MouHandle);
    printf ("rc = %d from MouDrawPtr\n", rc);
    
    while (1)
    {
        rc = MouReadEventQue (&event, &wait, MouHandle);
        if (rc) printf ("rc = %d from MouReadEventQue\n", rc);
        if (rc) continue;
        if (event.time == 0) continue;
        
        ev_name[0] = '\0';
        if (event.fs & MOUSE_BN1_DOWN)             strcat (ev_name, "B1 down     | ");
        if (event.fs & MOUSE_BN2_DOWN)             strcat (ev_name, "B2 down     | ");
        if (event.fs & MOUSE_BN3_DOWN)             strcat (ev_name, "B3 down     | ");
        if (event.fs & MOUSE_MOTION)               strcat (ev_name, "Movement    | ");
        if (event.fs & MOUSE_MOTION_WITH_BN1_DOWN) strcat (ev_name, "B1 movement | ");
        if (event.fs & MOUSE_MOTION_WITH_BN2_DOWN) strcat (ev_name, "B2 movement | ");
        if (event.fs & MOUSE_MOTION_WITH_BN3_DOWN) strcat (ev_name, "B3 movement | ");
        if (ev_name[0] == '\0')                    strcat (ev_name, "release     | ");
        
        printf ("%s row = %3d, col = %3d, flags = %x\n", ev_name, event.row, event.col, event.fs);
    }

    MouClose (MouHandle);
}
