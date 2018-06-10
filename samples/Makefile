PROGS = input testsuite ef
CC = gcc
CFLAGS = -Wall -O -g -I../src -I../../include 
LDFLAGS = -L../src -L../../lib

all: $(foreach b, $(PROGS), $(b)_os2vio.exe $(b)_os2pm.exe $(b)_os2x.exe $(b)_os2x11.exe)

unix: $(foreach b, $(PROGS), $(b)_unix_term $(b)_unix_x11)

beos: $(foreach b, $(PROGS), $(b)_beosterm)

win:  $(foreach b, $(PROGS), $(b)_win32cons.exe $(b)_win32gui.exe)

#input_os2vio.exe: input.c ../libfly_os2vio.a
#	$(CC) $(CFLAGS) -Zbin-files -o $@ $^ -lasvtools

%_os2vio.exe: %.o main.o
	$(CC) $(LDFLAGS) -s -Zbin-files -Zcrtdll -o $@ $^ -lfly_os2vio -lasvtools

%_os2x.exe: %.o main.o
	$(CC) $(LDFLAGS) -s -Zbin-files -Zcrtdll -o $@ $^ -lfly_os2x -ltermcap -lasvtools

%_os2x11.exe: %.o main.o
	$(CC) $(LDFLAGS) -s -Zbin-files -Zcrtdll -o $@ $^ -lfly_os2x11 -lasvtools -lX11

%_os2pm.exe: %.o main.o
	$(CC) $(LDFLAGS) -Zbin-files -Zcrtdll -o $@ $^ $*.def $*.res -lfly_os2pm -lasvtools
		
%_unix_term: %.o main.o
	$(CC) $(LDFLAGS) -o $@ $^ -lfly_unix_term -ltermcap -lasvtools

%_unix_x11: %.o main.o
	$(CC) $(LDFLAGS) -s -o $@ $^ -lfly_unix_x11 -lX11 -lasvtools
		
%_beosterm: %.o main.o
	$(CC) $(LDFLAGS) -s -o $@ $^ -lfly_beosterm -ltermcap -lasvtools

%_win32cons.exe: %.o main.o
	$(CC) $(LDFLAGS) -o $@ $^ -lfly_win32cons -lasvtools -luser32

%_win32gui.exe: %.o winmain.o
	$(CC) $(LDFLAGS) -o $@ $^ -lfly_win32gui -lasvtools -luser32 -lgdi32 -mwindows

clean:
	rm -f *.exe debug.txt keycodes core
	rm -f $(foreach b, $(PROGS), $(b)_unix_term $(b)_unix_x11)
	rm -f $(foreach b, $(PROGS), $(b)_beosterm)

