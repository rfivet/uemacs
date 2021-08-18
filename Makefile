# Makefile -- µEMACS
# Copyright © 2013-2021 Renaud Fivet

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

PROGRAM=ue

CC=gcc
WARNINGS=-pedantic -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter
CFLAGS=-O2 $(WARNINGS)
LDFLAGS=-s
LIBS=-lcurses
DEFINES=-DPROGRAM=$(PROGRAM) -D_GNU_SOURCE # -DNDEBUG

BINDIR=/usr/bin
LIBDIR=/usr/lib

SRCS = $(sort $(wildcard *.c))
OBJS = $(SRCS:.c=.o)

$(PROGRAM): $(OBJS)
	$(E) "  LINK    " $@
	$(Q) $(CC) $(LDFLAGS) -o $@ $(OBJS) $(LIBS)

clean:
	$(E) "  CLEAN"
	$(Q) rm -f $(PROGRAM) depend.mak *.o

install: $(PROGRAM)
	strip $(PROGRAM)
	cp $(PROGRAM) ${BINDIR}
	cp emacs.hlp ${LIBDIR}
	cp emacs.rc ${LIBDIR}/.emacsrc
	chmod 755 ${BINDIR}/$(PROGRAM)
	chmod 644 ${LIBDIR}/emacs.hlp ${LIBDIR}/.emacsrc

.c.o:
	$(E) "  CC      " $@
	$(Q) ${CC} ${CFLAGS} ${DEFINES} -c $*.c

depend.mak: $(wildcard *.h)
	$(E) "  DEPEND"
	$(Q) $(CC) $(DEFINES) -MM $(SRCS) > depend.mak

include depend.mak

# end of Makefile
