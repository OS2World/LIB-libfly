#---------------------------------------------------------------------------

ifdef BE_HOST_CPU
TARGETS = beos_term
INSTALL = sh install-beos.sh
endif

ifdef windir
TARGETS = win32cons win32gui
INSTALL = install-win32.bat
endif

ifdef RUNWORKPLACE
TARGETS = os2vio os2pm os2x os2x11
INSTALL = sh install-os2.sh
endif

ifndef TARGETS
TARGETS = unix_term unix_x11
INSTALL = sh install-unix.sh
endif

#---------------------------------------------------------------------------

CC = gcc
CFLAGS = -Wall -O -g -I../../include -I/usr/X11R6/include -I/usr/local/include
LDFLAGS = -L../../lib

CFLAGS_os2vio    = -DFLY_EMX_VIO
CFLAGS_os2pm     = -DFLY_EMX_PM
CFLAGS_os2x      = -DFLY_EMX_X
CFLAGS_os2x11    = -DFLY_EMX_X11
CFLAGS_beos_term = -DFLY_BEOS_TERM
CFLAGS_win32cons = -DFLY_WIN32_CONS
CFLAGS_win32gui  = -DFLY_WIN32_GUI
CFLAGS_unix_term = -DFLY_UNIX_TERM
CFLAGS_unix_x11  = -DFLY_UNIX_X11

OBJS = platform.o video.o keyboard.o fly.o ui.o ui_text.o menu.o pmousing.o
OBJFILES = $(foreach b, $(OBJS), $(foreach a, $(TARGETS), $(a)/$b))

PLATFORMFILES = \
  os2vio.c os2pm.c \
  drives_os2.c drives_win32.c drives_no.c \
  kbd_select.c kbd_poll.c \
  setproctitle_no.c setproctitle_unix.c \
  unix.c x11.c \
  win32.c win32gui.c \
  menu_text.c winkeytables.c

all: $(foreach a, $(TARGETS), libfly_$(a).a) $(OBJFILES)

libfly_%.a: $(foreach b, $(OBJS), %/$b)
	rm -f $@
	ar rc $@ $*/*.o

%/platform.o: platform.c $(PLATFORMFILES)
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ -c platform.c

%/video.o: video.c
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ -c $^

%/keyboard.o: keyboard.c
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ -c $^

%/fly.o: fly.c
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ -c $^

%/ui.o: ui.c
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ -c $^

%/ui_text.o: ui_text.c
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ -c $^

%/menu.o: menu.c
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ -c $^

%/pmousing.o: pmousing.c
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ -c $^

clean:
	rm -f *.o *.a *.dll *.lib $(OBJFILES)

install:
	$(INSTALL) $(foreach a, $(TARGETS), libfly_$(a).a)

fly_%.dll: $(foreach b, $(OBJS), %/$b) fly_%.def
	$(CC) -Zdll -Zcrtdll -o fly_$*.dll $^ -lasvtools -ltermcap -lX11
	emxbind -qs fly_$*.dll
	emximp -o libfly_$*.a fly_$*.def
	emxomf -s -l -q libfly_$*.a

os2dll: $(foreach a, $(TARGETS), fly_$(a).dll) $(OBJFILES)
	mv fly_os2vio.dll fly_vio.dll
	mv fly_os2pm.dll fly_pm.dll
	mv fly_os2x.dll fly_x.dll
	mv fly_os2x11.dll fly_x11.dll

fly_%.def: def.% def.exports
	cat def.$* def.exports >$@
