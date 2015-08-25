#
# Makefile:
# Makefile for bfilter
#
# Copyright (c) 2003 Chris Lightfoot. All rights reserved.
# Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
#
# $Id: Makefile,v 1.5 2003/08/19 11:03:21 chris Exp $
#

# Edit these until it compiles.
CFLAGS += -g -Wall
LDFLAGS += -g
LDLIBS += -lgdbm #-lefence

# No user-serviceable parts below this point.
VERSION = 0.2

TXTS = README COPYING bfilter.1 CHANGES
SRCS = bfilter.c pool.c skiplist.c util.c
HDRS = pool.h skiplist.h util.h
OBJS = $(SRCS:.c=.o)

bfilter: $(OBJS) depend
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

clean:
	rm -f $(OBJS) bfilter core *~ depend

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
