#
# Makefile:
# Makefile for bfilter
#
# Copyright (c) 2003 Chris Lightfoot. All rights reserved.
# Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
#
# $Id: Makefile,v 1.12 2005/06/07 16:41:42 chris Exp $
#

# Edit these until it compiles.
CC = gcc -std=c99
CFLAGS += -g -Wall -I/software/include -I.
LDFLAGS += -g -L/software/lib
LDLIBS += -lsqlite3 -lm #-lefence

# No user-serviceable parts below this point.
VERSION = 0.4

TXTS = README COPYING bfilter.1 CHANGES tokeniser-states.dot migrate-0.2-to-0.3
SRCS = bayes.c class.c compose.c cook.c count.c db.c db-count.c db-term.c error.c fdump.c line.c main.c pool.c \
       read.c skiplist.c submit.c token.c train.c utf8.c util.c
HDRS = bayes.h class.h compose.h cook.h count.h db.h db-count.h db-term.h error.h fdump.h line.h pool.h \
       read.h skiplist.h submit.h token.h train.h utf8.h util.h
OBJS = $(SRCS:.c=.o)

CFLAGS += -DBFILTER_VERSION=\"$(VERSION)\"

bfilter: $(OBJS) depend
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

# integration tests
test/icook: test/cook/cook.o cook.o line.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/ipass: test/pass/pass.o cook.o error.o fdump.o line.o read.o util.o utf8.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/iread: test/read/read.o cook.o error.o line.o read.o utf8.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/itoken: test/token/main.o cook.o error.o line.o read.o token.o utf8.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# unit tests
test/ubayes: test/unit/bayes.o bayes.o error.o line.o pool.o skiplist.o submit.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/uclass: test/unit/class.o class.o db.o error.o line.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/ucompose: test/unit/compose.o compose.o error.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/ucount: test/unit/count.o count.o db.o error.o line.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/udb_intlist: test/unit/db_intlist.o db.o error.o line.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/uline: test/unit/line.o error.o line.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/uskiplist0: test/unit/skiplist0.o error.o pool.o skiplist.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/uskiplist1: test/unit/skiplist1.o error.o pool.o skiplist.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/uskiplist: test/unit/skiplist.o error.o pool.o skiplist.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/utoken: test/unit/token.o error.o token.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/uutf8: test/unit/utf8.o utf8.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

check:
	cd test; ./run.sh

clean:
	rm -f $(OBJS) bfilter core *~ depend test/*/*.o

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
