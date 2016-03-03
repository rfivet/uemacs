# Makefile for emacs, updated Fri, Feb 13, 2015 12:02:52 PM

SRC=basic.c bind.c bindable.c buffer.c crypt.c display.c ebind.c eval.c exec.c execute.c file.c fileio.c flook.c input.c isearch.c line.c lock.c main.c mingw32.c mlout.c names.c pklock.c posix.c random.c region.c search.c spawn.c tcap.c termio.c utf8.c window.c word.c wrapper.c wscreen.c
OBJ=basic.o bind.o bindable.o buffer.o crypt.o display.o ebind.o eval.o exec.o execute.o file.o fileio.o flook.o input.o isearch.o line.o lock.o main.o mingw32.o mlout.o names.o pklock.o posix.o random.o region.o search.o spawn.o tcap.o termio.o utf8.o window.o word.o wrapper.o wscreen.o
HDR=basic.h bind.h bindable.h buffer.h crypt.h defines.h display.h ebind.h estruct.h eval.h exec.h execute.h file.h fileio.h flook.h input.h isearch.h line.h lock.h mlout.h names.h pklock.h random.h region.h retcode.h search.h spawn.h terminal.h termio.h utf8.h version.h window.h word.h wrapper.h wscreen.h

# DO NOT ADD OR MODIFY ANY LINES ABOVE THIS -- make source creates them

# Make the build silent by default
V =

ifeq ($(strip $(V)),)
	E = @echo
	Q = @
else
	E = @\#
	Q =
endif
export E Q

uname_S := $(shell sh -c 'uname -s 2>/dev/null || echo not')
# for windows based target, insure we strip the variant part
# CYGWIN_NT-6.1, CYGWIN_NT-6.1-WOW, CYGWIN_NT-6.1-WOW64, MINGW32_NT-6.1
uname_S := $(shell sh -c 'echo $(uname_S) | sed s/_.*$$//')

PROGRAM=ue

CC=gcc
WARNINGS=-pedantic -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter
CFLAGS=-O2 $(WARNINGS)
#CC=c89 +O3			# HP
#CFLAGS= -D_HPUX_SOURCE -DSYSV
#CFLAGS=-O4 -DSVR4		# Sun
#CFLAGS=-O -qchars=signed	# RS/6000
ifeq ($(uname_S),Linux)
 DEFINES=-DAUTOCONF -DPROGRAM=$(PROGRAM) -DPOSIX -DUSG -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=600
 LIBS=-lcurses
endif
ifeq ($(uname_S),FreeBSD)
 DEFINES=-DAUTOCONF -DPOSIX -DSYSV -D_FREEBSD_C_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=600
endif
ifeq ($(uname_S),Darwin)
 DEFINES=-DAUTOCONF -DPOSIX -DSYSV -D_DARWIN_C_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=600
endif
ifeq ($(uname_S),CYGWIN)
 DEFINES=-DAUTOCONF -DCYGWIN -DSYSV -DPROGRAM=$(PROGRAM)
 LIBS=-lcurses
endif
ifeq ($(uname_S),MINGW32)
 DEFINES=-DAUTOCONF -DSYSV -DMINGW32 -DPROGRAM=$(PROGRAM)
 LIBS=
endif
#DEFINES=-DAUTOCONF
#LIBS=-ltermcap			# BSD
#LIBS=-lcurses			# SYSV
#LIBS=-ltermlib
#LIBS=-L/usr/lib/termcap -ltermcap
LFLAGS=-hbx
BINDIR=/usr/bin
LIBDIR=/usr/lib

$(PROGRAM): $(OBJ)
	$(E) "  LINK    " $@
	$(Q) $(CC) $(LDFLAGS) $(DEFINES) -o $@ $(OBJ) $(LIBS)

SPARSE=sparse
SPARSE_FLAGS=-D__LITTLE_ENDIAN__ -D__x86_64__ -D__linux__ -D__unix__

sparse:
	$(SPARSE) $(SPARSE_FLAGS) $(DEFINES) $(SRC)

clean:
	$(E) "  CLEAN"
	$(Q) rm -f $(PROGRAM) core lintout makeout tags Makefile.bak *.o

install: $(PROGRAM)
	strip $(PROGRAM)
	cp $(PROGRAM) ${BINDIR}
	cp emacs.hlp ${LIBDIR}
	cp emacs.rc ${LIBDIR}/.emacsrc
	chmod 755 ${BINDIR}/$(PROGRAM)
	chmod 644 ${LIBDIR}/emacs.hlp ${LIBDIR}/.emacsrc

lint:	${SRC}
	@rm -f lintout
	lint ${LFLAGS} ${SRC} >lintout
	cat lintout

splint:
	splint -weak $(DEFINES) $(SRC) -booltype boolean -booltrue TRUE -boolfalse FALSE +posixlib +matchanyintegral

errs:
	@rm -f makeout
	make $(PROGRAM) >makeout

tags:	${SRC}
	@rm -f tags
	ctags ${SRC}

source:
	@mv Makefile Makefile.bak
	@echo "# Makefile for emacs, updated `date`" >Makefile
	@echo '' >>Makefile
#Sorted
	@echo SRC=`ls *.c` >>Makefile
	@echo OBJ=`ls *.c | sed s/c$$/o/` >>Makefile
	@echo HDR=`ls *.h` >>Makefile
#UnSorted
#	@echo SRC=$(wildcard *.c) >>Makefile
#	@echo OBJ=$(patsubst %.c,%.o,$(wildcard *.c)) >>Makefile
#	@echo HDR=$(wildcard *.h) >>Makefile
	@echo '' >>Makefile
	@sed -n -e '/^# DO NOT ADD OR MODIFY/,$$p' <Makefile.bak >>Makefile

depend: ${SRC}
	@mv Makefile Makefile.bak
	@sed -n -e '1,/^# DO NOT DELETE THIS LINE/p' Makefile.bak > Makefile
	@echo >> Makefile
	@for i in ${SRC}; do\
	    cc ${DEFINES} -MM $$i ; done >> Makefile
	@echo '' >>Makefile
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >>Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >>Makefile
	@echo '# see make depend above' >>Makefile

.c.o:
	$(E) "  CC      " $@
	$(Q) ${CC} ${CFLAGS} ${DEFINES} -c $*.c

# DO NOT DELETE THIS LINE -- make depend uses it

basic.o: basic.c basic.h buffer.h crypt.h line.h retcode.h utf8.h \
 estruct.h input.h bind.h mlout.h random.h terminal.h defines.h window.h
bind.o: bind.c bind.h estruct.h bindable.h buffer.h crypt.h line.h \
 retcode.h utf8.h display.h ebind.h exec.h file.h flook.h input.h names.h \
 window.h defines.h
bindable.o: bindable.c bindable.h defines.h buffer.h crypt.h line.h \
 retcode.h utf8.h display.h estruct.h file.h input.h bind.h lock.h \
 mlout.h terminal.h
buffer.o: buffer.c buffer.h crypt.h line.h retcode.h utf8.h defines.h \
 estruct.h file.h input.h bind.h mlout.h window.h
crypt.o: crypt.c crypt.h
display.o: display.c display.h estruct.h buffer.h crypt.h line.h \
 retcode.h utf8.h input.h bind.h termio.h terminal.h defines.h version.h \
 wrapper.h window.h
ebind.o: ebind.c ebind.h basic.h bind.h estruct.h bindable.h buffer.h \
 crypt.h line.h retcode.h utf8.h eval.h exec.h file.h isearch.h random.h \
 region.h search.h spawn.h window.h defines.h word.h
eval.o: eval.c eval.h basic.h bind.h buffer.h crypt.h line.h retcode.h \
 utf8.h display.h estruct.h exec.h execute.h flook.h input.h random.h \
 search.h terminal.h defines.h termio.h version.h window.h
exec.o: exec.c exec.h retcode.h buffer.h crypt.h line.h utf8.h bind.h \
 display.h estruct.h eval.h file.h flook.h input.h random.h window.h \
 defines.h
execute.o: execute.c execute.h estruct.h bind.h random.h retcode.h \
 display.h file.h buffer.h crypt.h line.h utf8.h input.h mlout.h search.h \
 terminal.h defines.h window.h
file.o: file.c file.h buffer.h crypt.h line.h retcode.h utf8.h defines.h \
 display.h estruct.h execute.h fileio.h input.h bind.h lock.h mlout.h \
 window.h
fileio.o: fileio.c fileio.h crypt.h retcode.h defines.h utf8.h
flook.o: flook.c flook.h retcode.h defines.h fileio.h crypt.h
input.o: input.c input.h bind.h estruct.h bindable.h display.h exec.h \
 retcode.h names.h terminal.h defines.h utf8.h wrapper.h
isearch.o: isearch.c isearch.h basic.h buffer.h crypt.h line.h retcode.h \
 utf8.h display.h estruct.h exec.h input.h bind.h search.h terminal.h \
 defines.h window.h
line.o: line.c line.h retcode.h utf8.h buffer.h crypt.h estruct.h mlout.h \
 window.h defines.h
lock.o: lock.c estruct.h lock.h
main.o: main.c estruct.h basic.h bind.h bindable.h buffer.h crypt.h \
 line.h retcode.h utf8.h display.h eval.h execute.h file.h lock.h mlout.h \
 random.h search.h terminal.h defines.h termio.h version.h window.h
mingw32.o: mingw32.c
mlout.o: mlout.c mlout.h
names.o: names.c names.h basic.h bind.h bindable.h buffer.h crypt.h \
 line.h retcode.h utf8.h display.h estruct.h eval.h exec.h file.h \
 isearch.h region.h random.h search.h spawn.h window.h defines.h word.h
pklock.o: pklock.c estruct.h pklock.h
posix.o: posix.c
random.o: random.c random.h retcode.h basic.h buffer.h crypt.h line.h \
 utf8.h display.h estruct.h execute.h input.h bind.h search.h terminal.h \
 defines.h window.h
region.o: region.c region.h line.h retcode.h utf8.h buffer.h crypt.h \
 estruct.h mlout.h random.h window.h defines.h
search.o: search.c search.h line.h retcode.h utf8.h basic.h buffer.h \
 crypt.h display.h estruct.h input.h bind.h mlout.h terminal.h defines.h \
 window.h
spawn.o: spawn.c spawn.h defines.h buffer.h crypt.h line.h retcode.h \
 utf8.h display.h estruct.h exec.h file.h flook.h input.h bind.h \
 terminal.h window.h
tcap.o: tcap.c terminal.h defines.h retcode.h display.h estruct.h \
 termio.h
termio.o: termio.c termio.h estruct.h retcode.h utf8.h
utf8.o: utf8.c utf8.h
window.o: window.c window.h defines.h buffer.h crypt.h line.h retcode.h \
 utf8.h basic.h display.h estruct.h execute.h terminal.h wrapper.h
word.o: word.c word.h basic.h buffer.h crypt.h line.h retcode.h utf8.h \
 estruct.h mlout.h random.h region.h window.h defines.h
wrapper.o: wrapper.c wrapper.h
wscreen.o: wscreen.c wscreen.h

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
