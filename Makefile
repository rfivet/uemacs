# Makefile -- µEMACS
# Copyright © 2013-2022 Renaud Fivet

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

CC=cc
WARNINGS=-pedantic -Wall -Wextra -Wstrict-prototypes -Wno-unused-parameter
CFLAGS=-O2 $(WARNINGS)
LDFLAGS=-s
LIBS=-lcurses
DEFINES=-DPROGRAM=$(PROGRAM) -D_GNU_SOURCE # -DNDEBUG

BINDIR=/usr/bin
LIBDIR=/usr/lib

SRCS = $(sort $(wildcard *.c))

$(PROGRAM): $(SRCS:.c=.o)
	$(E) "  LINK  " $@
	$(Q) $(CC) $(LDFLAGS) -o $@ $+ $(LIBS)

clean:
	$(E) "  CLEAN"
	$(Q) rm -f $(PROGRAM) *.dep *.o

install: $(PROGRAM)
	strip $(PROGRAM)
	cp $(PROGRAM) ${BINDIR}
	cp emacs.hlp ${LIBDIR}
	cp emacs.rc ${LIBDIR}/.emacsrc
	chmod 755 ${BINDIR}/$(PROGRAM)
	chmod 644 ${LIBDIR}/emacs.hlp ${LIBDIR}/.emacsrc

.c.o:
	$(E) "  CC    " $@
	$(Q) $(CC) $(CFLAGS) $(DEFINES) -c $*.c

%.dep: %.c
	$(E) "  DEPEND" $@
	$(Q) $(CC) $(DEFINES) -MM $< > $@

ifneq ($(MAKECMDGOALS),clean)
include $(SRCS:.c=.dep)
endif

# end of Makefile
