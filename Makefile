# makefile for emacs, updated Thu, Sep 19, 2013 12:14:27 PM

SRC=ansi.c basic.c bind.c bindable.c buffer.c crypt.c display.c ebind.c eval.c exec.c execute.c file.c fileio.c flook.c globals.c ibmpc.c input.c isearch.c line.c lock.c log.c main.c names.c pklock.c posix.c random.c region.c search.c spawn.c tcap.c termio.c utf8.c vmsvt.c vt52.c window.c word.c wrapper.c
OBJ=ansi.o basic.o bind.o bindable.o buffer.o crypt.o display.o ebind.o eval.o exec.o execute.o file.o fileio.o flook.o globals.o ibmpc.o input.o isearch.o line.o lock.o log.o main.o names.o pklock.o posix.o random.o region.o search.o spawn.o tcap.o termio.o utf8.o vmsvt.o vt52.o window.o word.o wrapper.o
HDR=basic.h bind.h bindable.h buffer.h crypt.h display.h ebind.h edef.h efunc.h estruct.h eval.h exec.h execute.h file.h fileio.h flook.h input.h isearch.h line.h lock.h log.h names.h pklock.h random.h region.h retcode.h search.h spawn.h termio.h utf8.h version.h window.h word.h wrapper.h

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

PROGRAM=ue

CC=gcc
WARNINGS=-Wall -Wstrict-prototypes
CFLAGS=-O2 $(WARNINGS)
#CC=c89 +O3			# HP
#CFLAGS= -D_HPUX_SOURCE -DSYSV
#CFLAGS=-O4 -DSVR4		# Sun
#CFLAGS=-O -qchars=signed	# RS/6000
ifeq ($(uname_S),Linux)
 DEFINES=-DAUTOCONF -DPOSIX -DUSG -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=600
endif
ifeq ($(uname_S),FreeBSD)
 DEFINES=-DAUTOCONF -DPOSIX -DSYSV -D_FREEBSD_C_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=600
endif
ifeq ($(uname_S),Darwin)
 DEFINES=-DAUTOCONF -DPOSIX -DSYSV -D_DARWIN_C_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=600
endif
ifeq ($(uname_S),CYGWIN_NT-6.1-WOW64)
 DEFINES=-DAUTOCONF -DCYGWIN -DPROGRAM=$(PROGRAM)
endif
ifeq ($(uname_S),CYGWIN_NT-6.1)
 DEFINES=-DAUTOCONF -DCYGWIN -DPROGRAM=$(PROGRAM)
endif
#DEFINES=-DAUTOCONF
#LIBS=-ltermcap			# BSD
LIBS=-lcurses			# SYSV
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
	$(Q) rm -f $(PROGRAM) core lintout makeout tags makefile.bak *.o

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

errs:
	@rm -f makeout
	make $(PROGRAM) >makeout

tags:	${SRC}
	@rm -f tags
	ctags ${SRC}

source:
	@mv makefile makefile.bak
	@echo "# makefile for emacs, updated `date`" >makefile
	@echo '' >>makefile
	@echo SRC=`ls *.c` >>makefile
	@echo OBJ=`ls *.c | sed s/c$$/o/` >>makefile
	@echo HDR=`ls *.h` >>makefile
	@echo '' >>makefile
	@sed -n -e '/^# DO NOT ADD OR MODIFY/,$$p' <makefile.bak >>makefile

depend: ${SRC}
	@for i in ${SRC}; do\
	    cc ${DEFINES} -MM $$i ; done >makedep
	@echo '/^# DO NOT DELETE THIS LINE/+2,$$d' >eddep
	@echo '$$r ./makedep' >>eddep
	@echo 'w' >>eddep
	@cp makefile makefile.bak
	@ed - makefile <eddep
	@rm eddep makedep
	@echo '' >>makefile
	@echo '# DEPENDENCIES MUST END AT END OF FILE' >>makefile
	@echo '# IF YOU PUT STUFF HERE IT WILL GO AWAY' >>makefile
	@echo '# see make depend above' >>makefile

#	@for i in ${SRC}; do\
#	    cc ${DEFINES} -M $$i | sed -e 's, \./, ,' | grep -v '/usr/include' | \
#	    awk '{ if ($$1 != prev) { if (rec != "") print rec; \
#		rec = $$0; prev = $$1; } \
#		else { if (length(rec $$2) > 78) { print rec; rec = $$0; } \
#		else rec = rec " " $$2 } } \
#		END { print rec }'; done >makedep

.c.o:
	$(E) "  CC      " $@
	$(Q) ${CC} ${CFLAGS} ${DEFINES} -c $*.c

# DO NOT DELETE THIS LINE -- make depend uses it

ansi.o: ansi.c estruct.h line.h utf8.h retcode.h edef.h
basic.o: basic.c basic.h display.h estruct.h line.h utf8.h retcode.h \
 edef.h input.h random.h word.h
bind.o: bind.c bind.h edef.h estruct.h line.h utf8.h retcode.h bindable.h \
 buffer.h display.h ebind.h exec.h file.h flook.h input.h names.h \
 window.h
bindable.o: bindable.c bindable.h buffer.h estruct.h line.h utf8.h \
 retcode.h display.h edef.h file.h input.h
buffer.o: buffer.c buffer.h estruct.h line.h utf8.h retcode.h display.h \
 edef.h file.h input.h window.h
crypt.o: crypt.c crypt.h estruct.h line.h utf8.h retcode.h
display.o: display.c display.h estruct.h line.h utf8.h retcode.h edef.h \
 termio.h version.h wrapper.h window.h
ebind.o: ebind.c ebind.h basic.h bind.h edef.h estruct.h line.h utf8.h \
 retcode.h bindable.h buffer.h eval.h exec.h file.h isearch.h random.h \
 region.h search.h spawn.h window.h word.h
eval.o: eval.c eval.h estruct.h line.h utf8.h retcode.h basic.h bind.h \
 edef.h buffer.h display.h exec.h flook.h input.h random.h search.h \
 termio.h version.h window.h
exec.o: exec.c exec.h estruct.h line.h utf8.h retcode.h buffer.h bind.h \
 edef.h display.h eval.h file.h flook.h input.h
execute.o: execute.c edef.h estruct.h line.h utf8.h retcode.h bind.h \
 random.h display.h file.h
file.o: file.c file.h buffer.h estruct.h line.h utf8.h retcode.h crypt.h \
 edef.h execute.h fileio.h input.h lock.h log.h window.h
fileio.o: fileio.c fileio.h retcode.h estruct.h line.h utf8.h crypt.h
flook.o: flook.c flook.h retcode.h estruct.h line.h utf8.h fileio.h
globals.o: globals.c estruct.h line.h utf8.h retcode.h edef.h
ibmpc.o: ibmpc.c estruct.h line.h utf8.h retcode.h edef.h
input.o: input.c input.h edef.h estruct.h line.h utf8.h retcode.h bind.h \
 bindable.h display.h exec.h names.h wrapper.h
isearch.o: isearch.c isearch.h basic.h display.h estruct.h line.h utf8.h \
 retcode.h edef.h input.h search.h
line.o: line.c line.h utf8.h estruct.h retcode.h edef.h log.h
lock.o: lock.c lock.h estruct.h line.h utf8.h retcode.h display.h edef.h \
 input.h
log.o: log.c log.h retcode.h
main.o: main.c basic.h bind.h edef.h estruct.h line.h utf8.h retcode.h \
 bindable.h buffer.h display.h eval.h execute.h file.h input.h lock.h \
 log.h random.h search.h termio.h version.h
names.o: names.c names.h basic.h bind.h edef.h estruct.h line.h utf8.h \
 retcode.h bindable.h buffer.h display.h eval.h exec.h file.h isearch.h \
 region.h random.h search.h spawn.h window.h word.h
pklock.o: pklock.c pklock.h estruct.h line.h utf8.h retcode.h edef.h
posix.o: posix.c termio.h
random.o: random.c random.h basic.h display.h estruct.h line.h utf8.h \
 retcode.h edef.h execute.h input.h log.h search.h
region.o: region.c region.h estruct.h line.h utf8.h retcode.h edef.h \
 log.h
search.o: search.c search.h estruct.h line.h utf8.h retcode.h basic.h \
 display.h edef.h input.h log.h
spawn.o: spawn.c spawn.h buffer.h estruct.h line.h utf8.h retcode.h \
 display.h edef.h file.h flook.h input.h log.h window.h
tcap.o: tcap.c display.h estruct.h line.h utf8.h retcode.h edef.h \
 termio.h
termio.o: termio.c termio.h estruct.h line.h utf8.h retcode.h edef.h
utf8.o: utf8.c utf8.h
vmsvt.o: vmsvt.c estruct.h line.h utf8.h retcode.h edef.h
vt52.o: vt52.c estruct.h line.h utf8.h retcode.h edef.h
window.o: window.c window.h estruct.h line.h utf8.h retcode.h basic.h \
 display.h edef.h execute.h wrapper.h
word.o: word.c word.h basic.h estruct.h line.h utf8.h retcode.h edef.h \
 log.h random.h region.h
wrapper.o: wrapper.c wrapper.h

# DEPENDENCIES MUST END AT END OF FILE
# IF YOU PUT STUFF HERE IT WILL GO AWAY
# see make depend above
