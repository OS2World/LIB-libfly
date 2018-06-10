#include <fly/fly.h>

#include <stdio.h>
#include <string.h>

int fly_main (int argc, char *argv[], char **envp)
{
    int     rows, cols, n, run, i, msg, k;
    char    **p, *p1, buffer[80], buffer2[1024], *name, *wintitle;
    int     events[8192], nev=0;

    tools_debug = fopen ("debug.txt", "w");
    
    fly_initialize ();
    fly_process_args (&argc, &argv, &envp); 
    
    fly_init (-1, -1, -1, -1, NULL);
    fly_mouse (1);

    wintitle = get_window_name ();
    set_window_name ("Fly! Input method testing...");

    /*
    i = fly_ask (ASK_WARN, " Would you like \n to know the truth about %s? ",
                 " I want truth \n I want lie \n Leave me alone ", "Lewinski");
    fly_ask (0, "Response was %d", NULL, i);
    
    i = fly_ask (ASK_WARN|ASK_YN, " Are you sure??? ", NULL);
    fly_ask (0, "Response was %d", NULL, i);
    */
    
    run = TRUE;
    do
    {
        video_cursor_state (0);
        rows = video_vsize ();
        cols = video_hsize ();
        video_put_str ("X to exit", 9, 0, cols-9-1);
        for (i=0; i<rows; i++)
        {
            n = nev - (rows-1) + i - 1;
            if (n < 0) continue;
            strcpy (buffer, "???");
            if (IS_MOUSE (events[n]))
            {
                name = "?";
                switch (MOU_EVTYPE(events[n]))
                {
                case MOUEV_MOVE:    name = "Move     "; break;
                case MOUEV_B1DC:    name = "B1 Dclick"; break;
                case MOUEV_B2DC:    name = "B2 Dclick"; break;
                case MOUEV_B3DC:    name = "B3 Dclick"; break;
                case MOUEV_B1SC:    name = "B1  click"; break;
                case MOUEV_B2SC:    name = "B2  click"; break;
                case MOUEV_B3SC:    name = "B3  click"; break;
                case MOUEV_B1MS:    name = "B1 move S"; break; 
                case MOUEV_B1DR:    name = "B1 move  "; break;
                case MOUEV_B1ME:    name = "B1 move E"; break; 
                case MOUEV_B2MS:    name = "B2 move S"; break; 
                case MOUEV_B2DR:    name = "B2 move  "; break;
                case MOUEV_B2ME:    name = "B2 move E"; break; 
                case MOUEV_B3MS:    name = "B3 move S"; break; 
                case MOUEV_B3DR:    name = "B3 move  "; break;
                case MOUEV_B3ME:    name = "B3 move E"; break;
                case MOUEV_B1AC:    name = "B1 auto  "; break;
                case MOUEV_B2AC:    name = "B2 auto  "; break;
                case MOUEV_B3AC:    name = "B3 auto  "; break;
                case MOUEV_B1DN:    name = "B1 down  "; break;
                case MOUEV_B2DN:    name = "B2 down  "; break;
                case MOUEV_B3DN:    name = "B3 down  "; break;
                case MOUEV_B1UP:    name = "B1 up    "; break;
                case MOUEV_B2UP:    name = "B2 up    "; break;
                case MOUEV_B3UP:    name = "B3 up    "; break;
                }
                snprintf1 (buffer, sizeof(buffer), "MOUSE:    %s r = %3d c = %3d", name,
                           MOU_Y(events[n]), MOU_X(events[n]));
            }
            if (IS_KEY (events[n]))
            {
                k = events[n];
                if (k >= __EXT_KEY_BASE__)
                    snprintf1 (buffer, sizeof(buffer), "KEYBOARD: %4d : %3d+%3d : ",
                            k, __EXT_KEY_BASE__, k-__EXT_KEY_BASE__);
                else
                    snprintf1 (buffer, sizeof(buffer), "KEYBOARD: %4d :         : ", k);
                p = fly_keyname (k);
                if (*p != NULL)
                    while (*p != NULL)
                    {
                        snprintf1 (buffer2, sizeof(buffer2), "[%s] ", *p++);
                        strcat (buffer, buffer2);
                    }
                else
                {
                    snprintf1 (buffer2, sizeof(buffer2), "%c", k);
                    strcat (buffer, buffer2);
                }
                p1 = fly_keyname2 (k);
                if (p1 != NULL)
                {
                    snprintf1 (buffer2, sizeof(buffer2), "(%s)", p1);
                    strcat (buffer, buffer2);
                }
            }
            if (IS_SYSTEM (events[n]))
            {
                switch (SYS_TYPE (events[n]))
                {
                case SYSTEM_RESIZE:
                    snprintf1 (buffer, sizeof(buffer), "SYSTEM:   message: Resize (%d rows, %d cols)",
                             SYS_INT2(events[n]), SYS_INT1(events[n]));
                    break;
                case SYSTEM_QUIT:
                    strcpy (buffer, "SYSTEM:   message: Quit");
                    break;
                }
            }
            if (IS_MENU (events[n]))
            {
                snprintf1 (buffer, sizeof(buffer), "MENU:    message %d", events[n]-FMSG_BASE_MENU);
            }
            video_put_n_char (' ', cols, i, 0);
            video_put (buffer, i, 0);
        }
        video_update (0);
        
        msg = getmessage (-1);
        debug_tools ("got a message %u\n", msg);
        if (nev == sizeof(events)/sizeof(events[0])-1) nev = 0;
        events[nev++] = msg;
        if (IS_KEY(msg) && msg == 'X') run = FALSE;
    }
    while (run);

    set_window_name (wintitle);
    fly_terminate ();
    return 0;
}
