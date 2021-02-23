# Makefile for uEMACS, updated 03 Nov 2020 10:22:37

SRC=basic.c bind.c bindable.c buffer.c display.c ebind.c eval.c exec.c execute.c file.c fileio.c flook.c input.c isearch.c line.c lock.c main.c mlout.c names.c pklock.c posix.c random.c region.c search.c spawn.c tcap.c termio.c utf8.c util.c window.c word.c wrapper.c
OBJ=basic.o bind.o bindable.o buffer.o display.o ebind.o eval.o exec.o execute.o file.o fileio.o flook.o input.o isearch.o line.o lock.o main.o mlout.o names.o pklock.o posix.o random.o region.o search.o spawn.o tcap.o termio.o utf8.o util.o window.o word.o wrapper.o
HDR=basic.h bind.h bindable.h buffer.h defines.h display.h ebind.h estruct.h eval.h exec.h execute.h file.h fileio.h flook.h input.h isa.h isearch.h line.h lock.h mlout.h names.h pklock.h random.h region.h retcode.h search.h spawn.h terminal.h termio.h utf8.h util.h version.h window.h word.h wrapper.h

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
# CYGWIN_NT-6.1, CYGWIN_NT-6.1-WOW, CYGWIN_NT-6.1-WOW64, MSYS_NT-10.0-19042
uname_S := $(shell sh -c 'echo $(uname_S) | sed s/_.*$$//')

PROGRAM=ue

CC=gcc
WARNINGS=-pedantic -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter
CFLAGS=-O2 $(WARNINGS)
LDFLAGS=-s
LIBS=-lcurses
DEFINES=-DAUTOCONF -DPROGRAM=$(PROGRAM)
ifeq ($(uname_S),Linux)
 DEFINES:= $(DEFINES) -DPOSIX -DUSG
else ifeq ($(uname_S),CYGWIN)
 DEFINES:= $(DEFINES) -DCYGWIN -DSYSV
else ifeq ($(uname_S),MSYS)
 DEFINES:= $(DEFINES) -DCYGWIN -DSYSV
else ifeq ($(uname_S),NetBSD)
 DEFINES:= $(DEFINES) -DPOSIX -DBSD=1
else
 $(error $(uname_S) needs configuration)
endif

#ifeq ($(uname_S),FreeBSD)
# DEFINES=-DAUTOCONF -DPOSIX -DSYSV -D_FREEBSD_C_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=600
#endif
#ifeq ($(uname_S),Darwin)
# DEFINES=-DAUTOCONF -DPOSIX -DSYSV -D_DARWIN_C_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=600
#endif

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
	$(Q) $(CC) $(LDFLAGS) -o $@ $(OBJ) $(LIBS)

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
	make $(PROGRAM) >makeout 2>&1

tags:	${SRC}
	@rm -f tags
	ctags ${SRC}

source:
	@mv Makefile Makefile.bak
	@echo "# Makefile for uEMACS, updated `date`" >Makefile
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
	@sed -n -e '/^# DO NOT ADD OR MODIFY/,$$p' Makefile.bak >> Makefile

depend: ${SRC}
	@mv Makefile Makefile.bak
	@sed -n -e '1,/^# DO NOT DELETE THIS LINE/p' Makefile.bak > Makefile
	@echo "# Updated `date`" >> Makefile
	@echo >> Makefile
	@for i in ${SRC}; do\
	    $(CC) ${DEFINES} -MM $$i ; done >> Makefile
	@echo '' >>Makefile
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >>Makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >>Makefile
	@echo '# see make depend above' >>Makefile

.c.o:
	$(E) "  CC      " $@
	$(Q) ${CC} ${CFLAGS} ${DEFINES} -c $*.c

# DO NOT DELETE THIS LINE -- make depend uses it
# Updated 03 Nov 2020 10:29:47

basic.o: basic.c basic.h retcode.h input.h bind.h mlout.h random.h \
 terminal.h defines.h utf8.h window.h buffer.h line.h
bind.o: bind.c bind.h estruct.h bindable.h buffer.h line.h retcode.h \
 utf8.h display.h ebind.h exec.h file.h flook.h input.h names.h util.h \
 window.h defines.h
bindable.o: bindable.c bindable.h defines.h buffer.h line.h retcode.h \
 utf8.h display.h estruct.h file.h input.h bind.h lock.h mlout.h \
 terminal.h
buffer.o: buffer.c buffer.h line.h retcode.h utf8.h defines.h estruct.h \
 file.h input.h bind.h mlout.h util.h window.h
display.o: display.c display.h estruct.h utf8.h buffer.h line.h retcode.h \
 input.h bind.h termio.h terminal.h defines.h version.h wrapper.h \
 window.h
ebind.o: ebind.c ebind.h basic.h retcode.h bind.h estruct.h bindable.h \
 buffer.h line.h utf8.h eval.h exec.h file.h isearch.h random.h region.h \
 search.h spawn.h window.h defines.h word.h
eval.o: eval.c eval.h basic.h retcode.h bind.h buffer.h line.h utf8.h \
 display.h estruct.h exec.h execute.h flook.h input.h random.h search.h \
 terminal.h defines.h termio.h util.h version.h window.h
exec.o: exec.c exec.h retcode.h buffer.h line.h utf8.h bind.h display.h \
 estruct.h eval.h file.h flook.h input.h random.h util.h window.h \
 defines.h
execute.o: execute.c execute.h estruct.h bind.h random.h retcode.h \
 display.h utf8.h file.h buffer.h line.h input.h mlout.h search.h \
 terminal.h defines.h window.h
file.o: file.c file.h buffer.h line.h retcode.h utf8.h defines.h \
 display.h estruct.h execute.h fileio.h input.h bind.h lock.h mlout.h \
 util.h window.h
fileio.o: fileio.c fileio.h defines.h retcode.h utf8.h
flook.o: flook.c flook.h retcode.h defines.h fileio.h
input.o: input.c input.h bind.h estruct.h bindable.h display.h utf8.h \
 exec.h retcode.h isa.h names.h terminal.h defines.h wrapper.h
isearch.o: isearch.c isearch.h basic.h retcode.h buffer.h line.h utf8.h \
 display.h estruct.h exec.h input.h bind.h search.h terminal.h defines.h \
 util.h window.h
line.o: line.c line.h retcode.h utf8.h buffer.h estruct.h mlout.h \
 window.h defines.h
lock.o: lock.c estruct.h lock.h
main.o: main.c estruct.h basic.h retcode.h bind.h bindable.h buffer.h \
 line.h utf8.h display.h eval.h execute.h file.h lock.h mlout.h random.h \
 search.h terminal.h defines.h termio.h util.h version.h window.h
mlout.o: mlout.c mlout.h
names.o: names.c names.h basic.h retcode.h bind.h bindable.h buffer.h \
 line.h utf8.h display.h estruct.h eval.h exec.h file.h isearch.h \
 region.h random.h search.h spawn.h window.h defines.h word.h
pklock.o: pklock.c estruct.h pklock.h
posix.o: posix.c
random.o: random.c random.h retcode.h basic.h buffer.h line.h utf8.h \
 display.h estruct.h execute.h input.h bind.h search.h terminal.h \
 defines.h window.h
region.o: region.c region.h line.h retcode.h utf8.h buffer.h estruct.h \
 mlout.h random.h window.h defines.h
search.o: search.c search.h line.h retcode.h utf8.h basic.h buffer.h \
 display.h estruct.h input.h bind.h isa.h mlout.h terminal.h defines.h \
 util.h window.h
spawn.o: spawn.c spawn.h defines.h buffer.h line.h retcode.h utf8.h \
 display.h estruct.h exec.h file.h flook.h input.h bind.h terminal.h \
 window.h
tcap.o: tcap.c terminal.h defines.h retcode.h utf8.h display.h estruct.h \
 termio.h
termio.o: termio.c termio.h utf8.h estruct.h retcode.h
utf8.o: utf8.c utf8.h
util.o: util.c util.h
window.o: window.c window.h defines.h buffer.h line.h retcode.h utf8.h \
 basic.h display.h estruct.h execute.h terminal.h wrapper.h
word.o: word.c word.h basic.h retcode.h buffer.h line.h utf8.h estruct.h \
 isa.h mlout.h random.h region.h window.h defines.h
wrapper.o: wrapper.c wrapper.h

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
