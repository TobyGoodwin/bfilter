#    Copyright (c) 2003, 2004 Chris Lightfoot. All rights reserved.
#    Copyright (c) 2015 - 2016 Toby Goodwin.
#    toby@paccrat.org
#    https://github.com/TobyGoodwin/bfilter
#
#    This file is part of bfilter.
#
#    Bfilter is free software: you can redistribute it and/or modify
#    it under the terms of the GNU General Public License as published by
#    the Free Software Foundation, either version 3 of the License, or
#    (at your option) any later version.
#
#    Bfilter is distributed in the hope that it will be useful,
#    but WITHOUT ANY WARRANTY; without even the implied warranty of
#    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#    GNU General Public License for more details.
#
#    You should have received a copy of the GNU General Public License
#    along with bfilter.  If not, see <http://www.gnu.org/licenses/>.


VERSION = 1.0

# Edit these until it compiles.
CC = gcc -std=c99
CFLAGS += -g -Wall -I.
LDFLAGS += -g
LDLIBS += -lsqlite3 -lm

# No user-serviceable parts below this point.

TXTS = Changes.rst COPYING INSTALL README.rst bfilter.1
SRCS = bayes.c class.c compose.c cook.c db.c db-count.c db-term.c \
       error.c fdump.c line.c main.c pool.c read.c skiplist.c submit.c \
       token.c train.c utf8.c util.c
HDRS = bayes.h bfilter.h class.h compose.h cook.h db.h db-count.h db-term.h \
       error.h fdump.h line.h pool.h read.h skiplist.h submit.h \
       token.h train.h utf8.h util.h
OBJS = $(SRCS:.c=.o)

bfilter: $(OBJS) depend
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

main.o: CFLAGS += -DVERSION=\"$(VERSION)\"

# integration tests
test/icook: test/cook/cook.o cook.o line.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/ipass: test/pass/pass.o cook.o error.o fdump.o line.o read.o util.o utf8.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/iread: test/read/read.o cook.o error.o line.o read.o utf8.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/itoken: test/token/main.o cook.o error.o line.o read.o \
             token.o utf8.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

# unit tests
test/ubayes: test/unit/bayes.o bayes.o class.o db.o db-term.o error.o \
             line.o pool.o skiplist.o submit.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/uclass: test/unit/class.o class.o db.o error.o line.o util.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/ucompose: test/unit/compose.o compose.o error.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

test/ucount: test/unit/count.o db.o db-count.o error.o line.o util.o
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

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c -o $@ $<

dist: depend $(SRCS) $(HDRS) $(TXTS)
	mkdir bfilter-$(VERSION)
	set -e ; for i in Makefile depend $(SRCS) $(HDRS) $(TXTS) ; do cp $$i bfilter-$(VERSION)/$$i ; done
	tar cvf - bfilter-$(VERSION) | gzip --best > bfilter-$(VERSION).tar.gz
	rm -rf bfilter-$(VERSION)

depend: $(SRCS)
	$(CPP) $(CFLAGS) -MM $(SRCS) > depend

nodepend:
	rm -f depend

include depend
