# Makefile -- uEMACS
# Copyright (c) 2014-2021 Renaud Fivet

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
 DEFINES += -DPOSIX -DUSG
else ifeq ($(uname_S),CYGWIN)
 DEFINES += -DCYGWIN -DSYSV
else ifeq ($(uname_S),MSYS)
 DEFINES += -DCYGWIN -DSYSV
else ifeq ($(uname_S),NetBSD)
 DEFINES += -DPOSIX -DBSD=1
else
 $(error $(uname_S) needs configuration)
endif

#ifeq ($(uname_S),FreeBSD)
# DEFINES=-DAUTOCONF -DPOSIX -DSYSV -D_FREEBSD_C_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=600
#endif
#ifeq ($(uname_S),Darwin)
# DEFINES=-DAUTOCONF -DPOSIX -DSYSV -D_DARWIN_C_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_XOPEN_SOURCE=600
#endif

#LIBS=-ltermcap			# BSD
#LIBS=-lcurses			# SYSV
#LIBS=-ltermlib
#LIBS=-L/usr/lib/termcap -ltermcap
LFLAGS=-hbx
BINDIR=/usr/bin
LIBDIR=/usr/lib

SRCS = $(sort $(wildcard *.c))
OBJS = $(SRCS:.c=.o)

$(PROGRAM): $(OBJS)
	$(E) "  LINK    " $@
	$(Q) $(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

SPARSE=sparse
SPARSE_FLAGS=-D__LITTLE_ENDIAN__ -D__x86_64__ -D__linux__ -D__unix__

sparse:
	$(SPARSE) $(SPARSE_FLAGS) $(DEFINES) $(SRCS)

clean:
	$(E) "  CLEAN"
	$(Q) rm -f $(PROGRAM) core lintout makeout tags *.o

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
	splint -weak $(DEFINES) $(SRCS) -booltype boolean -booltrue TRUE -boolfalse FALSE +posixlib +matchanyintegral

errs:
	@rm -f makeout
	make $(PROGRAM) >makeout 2>&1

tags:	${SRC}
	@rm -f tags
	ctags ${SRC}

.c.o:
	$(E) "  CC      " $@
	$(Q) ${CC} ${CFLAGS} ${DEFINES} -c $*.c

depend.mak: $(SRCS)
	$(E) "  DEPEND"
	$(Q) $(CC) $(DEFINES) -MM $+ > depend.mak

include depend.mak

# end of Makefile
