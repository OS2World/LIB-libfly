#include <stdio.h>
#include <time.h>

#include <windows.h>


/* ------------------------------------------------------------- */

int main (void)
{
    INPUT_RECORD    inp;
    DWORD           nr, prev_console_mode;
    HANDLE          h_input, h_output;
    
    h_output = GetStdHandle (STD_OUTPUT_HANDLE);
    h_input  = GetStdHandle (STD_INPUT_HANDLE);
    GetConsoleMode (h_input, &prev_console_mode);
    SetConsoleMode (h_input, ENABLE_WINDOW_INPUT);
    do
    {
        ReadConsoleInput (h_input, &inp, 1, &nr);
        switch (inp.EventType)
        {
        case KEY_EVENT:
            if (inp.Event.KeyEvent.bKeyDown == 0) break;
            printf ("KeyCode = %3d, ", inp.Event.KeyEvent.wVirtualKeyCode);
            printf ("ScanCode = %3d, ", inp.Event.KeyEvent.wVirtualScanCode);
            printf ("KeyState = %3d, ", inp.Event.KeyEvent.dwControlKeyState);
            printf ("AsciiChar = %3d (%c)\n", inp.Event.KeyEvent.uChar.AsciiChar, 
                    inp.Event.KeyEvent.uChar.AsciiChar);
            break;
        }
    }
    while (inp.Event.KeyEvent.uChar.AsciiChar != 27);

    return 0;
}
