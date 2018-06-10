#include <stdio.h>
#include <ctype.h>
#include <termios.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

void handler (int);

#define STDIN_FD      0

int main (int argc, char *argv[])
{
    struct termios   tio, tio_s;
    int              nc;    
    unsigned long    flags;
    unsigned char    c;

    //for (i=0; i<NSIG; i++) signal (i, handler);
   
    // we need unbuffered input for select() to work
    fflush (stdin);
    setvbuf (stdin, NULL, _IONBF, 0);

    flags = fcntl (STDIN_FD, F_GETFL, 0);
    fcntl (STDIN_FD, F_SETFL, flags | O_NONBLOCK);

    // get terminal mode and save it
    tcgetattr (STDIN_FD, &tio);
    tio_s = tio;

    // set terminal parameters
    /* stty sane - input:
     -ignbrk brkint -ignpar -parmrk -inpck -istrip -inlcr -igncr
     icrnl ixon -ixoff -iuclc -ixany imaxbel */
    tio.c_iflag &= ~ISTRIP;
        
    /* stty sane - output:
     opost -olcuc -ocrnl onlcr -onocr -onlret -ofill -ofdel
     nl0 cr0 tab0 bs0 vt0 ff0 */
    
    /* stty sane - control:
     -parenb -parodd cs8 hupcl -cstopb cread -clocal -crtscts */
    tio.c_cflag &= ~(CSIZE | HUPCL);
    tio.c_cflag |=  CS8;
    
    /* stty sane - local:
     isig icanon iexten echo echoe echok -echonl -noflsh
     -xcase -tostop -echoprt echoctl echoke */
    //tio.c_lflag &= ~ISIG;
    tio.c_lflag &= ~(ICANON | ECHO/* | ECHOCTL*/);

    tio.c_cc[VMIN] = 1;
    
    tcsetattr (STDIN_FD, TCSADRAIN, &tio);

    printf ("Keycodes. X to exit\n");
    // enable mouse!
    puts ("\033[?1000h");
    do
    {
        nc = read (STDIN_FD, &c, 1);
        //c = getc (stdin);
        if (nc == 1)
        printf ("nc = %d, symbol = [%c], code = %3u, %02xx\n",
                nc, isprint(c) ? c : ' ',  c, c);
    }
    while (c != 'X');
        
    tcsetattr (STDIN_FD, TCSADRAIN, &tio_s);
    return 0;
}

void handler (int signo)
{
   printf ("signal %d received\n", signo);
   fflush (stdout);

   raise (signo);
   signal (signo, handler);   
}

