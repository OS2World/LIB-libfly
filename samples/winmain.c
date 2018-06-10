#include <stdlib.h>
#include <string.h>

#include <windows.h>
#include <fly/fly.h>

extern HINSTANCE s_hInst;
extern int s_nShow;

/* ------------------------------------------------------------- */

#define MAXCMDTOKENS 128

int STDCALL WinMain (HINSTANCE hInst, HINSTANCE hPrev, LPSTR lpCmd, int nShow)
{
    int     _argc=0;
    LPSTR   _argv[MAXCMDTOKENS];
    
    s_hInst = hInst;
    s_nShow = nShow;
    
    _argv[_argc] = "nftp.exe";
    _argv[++_argc] = strtok (lpCmd, " ");
    while (_argv[_argc] != NULL)
        _argv[++_argc] = strtok (NULL, " ");
        
    return fly_main (_argc, _argv, environ);
}

