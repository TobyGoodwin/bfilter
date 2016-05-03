
bfilter - a Bayesian spam filter
--------------------------------

Everyone and their dog is writing Bayesian spam filters these days. I'm no
exception. The first version of this was a trivial perl script, but it was slow
and crap, so I rewrote it as an enormous C program. This was also an excuse to
implement a skiplist data type, though inevitably my feeling of cleverness
there was punctured by a computer-scientist friend who told me that my
implementation was lousy and shamed me into redoing it. Such is life (and
that's the last time I trust lecture notes downloaded from the web...).

About the only significant thing which this program does which is not available
in Dog Brand(TM) mail filters is a moderately intelligent attempt to recognise
and decode base64 data in emails. Note that bfilter doesn't actually parse
emails properly (according to the rules of RFC[2]822, 1521 and so forth) but
instead applies some heuristics to find chunks of base64 encoded text which
extend over more than a few lines. I haven't attempted to analyse whether this
is worthwhile.

bfilter uses a TDB database to store the list of terms and term occurences
which is used to estimate the probability that a given email is spam. The sole
thing to be said for this approach is that it was easy to implement. TDB
databases are big and not blindingly fast. But disk space is cheap and
performance is acceptable.

NOTE TO UPGRADERS: version 0.3 uses a different database type -- Andrew
Tridgell's TDB, rather than GDBM -- in an attempt to save disk space and
improve concurrency (TDB doesn't have exclusive-lock-on-write). There is a perl
script, migrate-0.2-to-0.3, which performs this conversion for you. You will
need the TDB_File module from CPAN.


Installation
------------

Edit the Makefile and type make. Fix any errors which are reported. Copy the
resulting bfilter executable to Where You Want It; similarly for the man page,
bfilter.1.


Use
---

Assemble a corpus of spam and of non-spam email. The latter should include a
bunch of email you've sent. Then do

    bfilter isspam < spam-emails
    bfilter isreal < non-spam-emails

-- assuming that the two corpora are in the form of Berkeley mail folders (with
emails separated by a blank line and a line starting `From '). The word counts
are stored in the GDBM database ~/.bfildb.

Now you can pass a new email through

    bfilter test

and it will add on X-Spam-Words: and X-Spam-Probability: headers based on the
terms it finds in the emails. Any existing such header is replaced. You can
then use procmail or whatever to filter on the results, like this:

    :0 fw
    | bfilter test

    :0:
    * ^X-Spam-Probability: YES
    spam

or whatever.

From time to time you should run bfilter cleandb, which will discard from the
database terms which haven't been used in the past four weeks. This will save
some disk space.

bfilter understands the headers added by SpamAssassin and can filter on them;
I find this more useful than SpamAssassin's built-in scoring.


CVS
---

You can access the public CVS repository for bfilter:

  $ CVSROOT=:pserver:anonymous@sphinx.mythic-beasts.com:/home/chris/vcvs/repos
  $ export CVSROOT
  $ cvs login
  CVS password:       # password is `anonymous'
  $ cvs co bfilter


License, Disclaimer and Other Legal Stuff
-----------------------------------------

This program is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free Software
Foundation; either version 2 of the License, or (at your option) any later
version.

This program is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE.  See the GNU General Public License for more details.

bfilter is copyright (c) 2003 Chris Lightfoot <chris@ex-parrot.com>
See also: http://ex-parrot.com/~chris/software.html#bfilter

$Id: README,v 1.10 2004/04/18 13:56:02 chris Exp $
