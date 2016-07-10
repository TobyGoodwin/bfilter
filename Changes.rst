Change log for bfilter
======================

Toby Goodwin
------------

1.0
~~~

* implement ``untrain`` and ``retrain``
* implement ``move``
* convert from tdb to sqlite
* add the confidence estimate to classification outputs
* don't hash tokens
* remove timestamps from tokens
* introduce a database version id
* generalize to many arbitrary classes, not just *spam* vs *real* 
* handle html numeric entities, such as ``&#100;``
* do not split base64-encoded words at end of (encoded) line
* handle quoted-printable, including softeol
* replace Graham's algorithm with a more standard NBC
* remove (english-centric) case folding
* refactoring
* test suite added
* add ``-b`` to treat input as Berkeley mbox format
* do not treat input as mbox format by default

Chris Lightfoot
---------------

Fixed a crash bug in cleandb. Record log of "spam probaility" in
X-Spam-Probability header.

0.4
~~~

Fixed typos in man page. The help message now prints the bfilter
version. Added token history code, so that ``Buy Cheap Viagra`` now
gives rise to the tokens ``buy%cheap``, ``cheap%viagra`` and
``buy%cheap%viagra``, in addition to the single-word ones used
previously. Added a ``stats`` option for printing information about the
contents of the database. Changed ``cleandb`` to work by copying the
retained items to a new database file, then renaming it over the old
one; this is faster than the old method and should ensure that disk
space is reclaimed properly.

0.3
~~~

Fixed cleandb code. More error indications. Added tokeniser state machine
diagram, mostly as an experiment in graphviz. Uses TDB rather than GDBM, for
better concurrency. Does not use _ as a token separator when parsing
X-Spam-Status: headers, so bfilter can make better use of test results added
by Spam Assassin.

0.2
~~~

Added timestamps to database keys. Added cleandb option. Added X-Spam-Status:
(from Spam Assassin) to the list of headers to inspect. Changed the base
all-spam / all-real probabilities to 0.00001 / 0.99999.

0.1
~~~

Initial public release.
