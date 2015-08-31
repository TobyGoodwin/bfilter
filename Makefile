#
# Makefile:
# Makefile for bfilter
#
# Copyright (c) 2003 Chris Lightfoot. All rights reserved.
# Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
#
# $Id: Makefile,v 1.9 2004/04/22 19:13:15 chris Exp $
#

# Edit these until it compiles.
CC = gcc -std=c99
CFLAGS += -g -Wall -I/software/include
LDFLAGS += -g -L/software/lib
LDLIBS += -ltdb -lcrypto #-lefence

# No user-serviceable parts below this point.
VERSION = 0.3

TXTS = README COPYING bfilter.1 CHANGES tokeniser-states.dot migrate-0.2-to-0.3
SRCS = bfilter.c pool.c skiplist.c util.c db.c
HDRS = pool.h skiplist.h util.h db.h
OBJS = $(SRCS:.c=.o)

bfilter: $(OBJS) depend
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -f $(OBJS) bfilter core *~ depend

tokeniser-states.ps: tokeniser-states.dot
	dot -Tps tokeniser-states.dot > tokeniser-states.ps

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

tarball: depend $(SRCS) $(HDRS) $(TXTS)
	mkdir bfilter-$(VERSION)
	set -e ; for i in Makefile depend $(SRCS) $(HDRS) $(TXTS) ; do cp $$i bfilter-$(VERSION)/$$i ; done
	tar cvf - bfilter-$(VERSION) | gzip --best > bfilter-$(VERSION).tar.gz
	rm -rf bfilter-$(VERSION)

depend: $(SRCS)
	$(CPP) $(CFLAGS) -MM $(SRCS) > depend

nodepend:
	rm -f depend

include depend
