2015-10-03
==========

So I'm not finding this playing around with tests terribly enlightening.
One key point is that many of the spam regressions (when increasing
MAX_TRAIN_TOKENS) are very heavy on CSS terms. In fact, these are often
the *only* significant terms! Sometimes there is actually a stylesheet,
but often inline stlye attributes are used, but the HTML skipper fails
because quoted-printable is in use.

I think understanding q-p, or at the very least, eliding "=\n"
sequences, could produce a worthwhile improvement. Baseline first,
though, I currently have::

    #define MAX_TRAIN_TOKENS 5000
    #define MAX_TEST_TOKENS 500
    #define SIGNIFICANT_TERMS 23

    ham: 93.80% correct, spam: 85.30% correct
    -rw-------. 1 toby toby 2162688 Oct  3 09:19 /tmp/tmp.lV1plPO3pI
    67.21user 9.44system 1:16.65elapsed 100%CPU (6164maxresident)k

2015-10-02
==========

I've written some scripts to help with testing. If you create "A" and
"B" versions of bfilter, and call them ``bfilter-a`` and ``bfilter-b``,
then ``ab-test`` runs the corpus test on both, ``ab-check spam`` reports
the 10 most significant spam regressions. And if ``bfilter`` is a
version that dumps the probability list, ``ab-prob <message>`` diffs the
output from the "A" and "B" databases. (Hmm... so that last bit isn't
too useful actually. I think I need to add debug flags to print this
stuff, so I can use the *actual* "A" and "B" versions.)

Anyway, looking at regressions when bumping up max_tokens when
training... I don't think there's anything very much to conclude, the
differences just look like not enough input.

One thing that does strike me is that, with the token chains, we almost
certainly want to bump up nsig. In the (still small) training set that I
am using, and with the higher training token count, the phrase "You are
receiving this because" is strongly associated with spam. One of the
regressions features this::

    +receiving%this%because => 0.990000, 0.025000 => 0.980319
    +are%receiving%this => 0.990000, 0.025000 => 0.980319
    +You%are%receiving => 0.990000, 0.025000 => 0.980319
    +receiving%this => 0.990000, 0.025000 => 0.980319
    +are%receiving => 0.990000, 0.025000 => 0.980319
    +this%because => 0.990000, 0.025000 => 0.980319

So that one phrase has contributed 6 significant tokens, which is
unfortunate. Let's just quickly try with ``SIGNIFICANT_TERMS = 50``::

    ham: 90.40% correct, spam: 88.60% correct
    -rw-------. 1 toby toby 6606848 Oct  2 22:55 /tmp/tmp.hopdGCAYZm
    112.65user 8.46system 2:01.22elapsed 99%CPU (0avgtext+0avgdata
    9440maxresident)k
    88inputs+0outputs (1major+712841minor)pagefaults 0swaps

2015-09-30
==========

I'm just going to see if ``_Bool`` vs ``int`` is the reason for that
speedup. No, it's not that.

A minor snag with trying to work out why a tweak affects the results
(specifically, why it leads us to detecting fewer spams) is that there
are two ways it might cause the effect: training and testing. I don't
know if I might at some stage have to try and tease these apart. Anyway,
to begin with let's identify some particular messages that are
classified differently before and after.

Ah, OK. So these are HTML-heavy messages, that were being detected on
the basis of features of the HTML. Now we're just looking at the message
text, they're slipping through. I don't think there's much I can do
about that: further training should be able to spot them. The effect
isn't too serious, anyway.

Quick bodge to avoid discarding link targets: if I see ``'<'`` and the
next character is ``'a'`` or ``'A'``, then don't go into ``bra_ket``
mode. (That sadly misses ``<img src="...">``.) 

Random thought: what happens if we bump up MAX_TOKENS when training?
Hmm... usual story. Multiply by 10, and we go from 92.10 / 84.70 results
below to::

    ham: 94.30% correct, spam: 83.70% correct
    -rw-------. 1 toby toby 6606848 Sep 30 20:54 /tmp/tmp.UwhAMk5TXl
    105.63user 8.40system 1:53.83elapsed 100%CPU (9424maxresident)k

Useful extra 2% right on the hams. Why have the spams dropped this time?
Obviously it's a training problem, but maybe looking at some differently
classified messages can give us a clue.

Probably I should split this into two settings, MAX_TRAIN_TOKENS and
MAX_TEST_TOKENS or similar. Or possibly there should be no limit when
training.

2015-09-29
==========

Just starting to play with tokenization. First discovery, an input of
``don't`` gives rise to the token ``don`` (and, presumably, ``t`` which
is then discarded as too short). That's simple to fix.

Now, I want to skip any text in angle brackets. Except that skips email
addresses, so only skip if we're not in a header line (I renamed
``underscores`` to ``header``, as that describes what it means better.
I'm not sure I really care about underscores though.) This probably
obviates the test for HTML comments. On the other hand, I probably
*don't* really want to skip *all* text in angle brackets, as I really
need to include link targets, unless I can defer that to the vapourware
urlfilter.

So how does that do? ::

    ham: 92.10% correct, spam: 84.70% correct
    -rw-------. 1 toby toby 2162688 Sep 29 22:17 /tmp/tmp.AwAqbB2lKr
    28.13user 6.44system 0:34.39elapsed 100%CPU (5284maxresident)k

Hmm. Better on hams, not so good on spams. I wonder why?

Just for fun, I pushed it out to 3000 tokens::

    ham: 98.60% correct, spam: 80.20% correct
    -rw-------. 1 toby toby 6606848 Sep 29 22:21 /tmp/tmp.4wwmWX056e
    217.06user 10.60system 3:47.72elapsed 99%CPU (9316maxresident)k

Very similar story: usefully better on hams, mysteriously worse on
spams. I suppose I'll need to examine some spams that were previously
detected but no longer are, and see what tweaks are needed. Anyway, the
other odd thing about that result is that we are now *substantially*
faster. I have no idea why.  Could it possibly be the use of ``_Bool``?

2015-09-28
==========

Further cleanups and refactorings performed. There may still be some
small tweaks, but I think most of the code is now in the right files.

Now, what is a good value for MAX_TOKENS? Let's try a few different
ones, see how the time and accuracy measure up::

    _300 -   23s, 83.3 / 89.8
     500 -   39s, 87.1 / 88.5
    1000 - 1m27s, 93.3 / 81.8
    1500 - 2m31s, 95.9 / 75.7
    2000 - 3m38s, 96.1 / 79.7
    3000 - 5m16s, 97.7 / 83.4
    5000 - 7m05s, 97.4 / 84.7

Which is all sadly uninformative. Unsurprisingly, the fewer tokens we
ignore, the better the ham results. I have no idea why the spam figures
sometimes go the other way.

Anyway, I think I shall fix on 500 for testing purposes, as it keeps the
runtime reasonable, and is less likely to go awry than a smaller number.
I think for actual production use, one would want a rather higher
figure. So my baseline result is::

    ham: 87.10% correct, spam: 88.50% correct
    -rw-------. 1 toby toby 2162688 Sep 28 21:45 /tmp/tmp.TH4Ax507b2
    30.23user 6.80system 0:36.85elapsed 100%CPU (5280maxresident)k

First thing to try: what happens if we stop folding case, as Graham
recommends in *Better*? ::

    ham: 87.60% correct, spam: 89.40% correct
    -rw-------. 1 toby toby 2162688 Sep 28 21:47 /tmp/tmp.9scDkeVhU5
    30.37user 6.80system 0:37.00elapsed 100%CPU (5280maxresident)k

Well, it's not any worse. What about at 3000 tokens? ::

    ham: 97.00% correct, spam: 81.10% correct
    -rw-------. 1 toby toby 6606848 Sep 28 21:56 /tmp/tmp.8mxFDwCqSX
    314.08user 12.40system 5:27.15elapsed 99%CPU (9860maxresident)k

Which is, ever so slightly, worse. Still, I think we can leave case
folding turned off. Apart from anything else, it's a very parochial sort
of folding that was going on.

2015-09-27
==========

My *rank* idea is along the right lines, but not quite there. New
insight to try comes from the idea that we are examining significance
along two dimensions, which we need to combine.

Calculate p(spam) as currently (I'm going to fasten onto Graham's
clamps, till I have reason to do otherwise). Now calculate p(present),
which is simply the total number of messages containing this term over
the total number of messages. Let x = p(spam) * 2 - 1, so that more
significant probabilities are further from 0. And y = p(present). Now
just calculate r = sqrt(x^2 + y^2), and this is the measure of
significance.

This may be brilliant, but anyway, let's see it in action::

    ham: 97.70% correct, spam: 83.40% correct
    -rw-------. 1 toby toby 6606848 Sep 27 12:05 /tmp/tmp.0SMnNAJlyN
    292.97user 12.64system 5:16.00elapsed 96%CPU (9744maxresident)k

This is great! And it's much less arbitrary than just saying "5 or
more". A real breakthrough!

So, next, need to carve up bayes.c even more, and generate more test
cases. Then I can get back to the interesting job of improving
tokenization. (At present, snippets of HTML and CSS feature far too
strongly.)

Just by way of comparison, here's the starting point: Oggie's final
version, with MAX_TOKENS 3000, on the train-100 corpus::

    ham: 95.40% correct, spam: 80.70% correct
    -rw-------. 1 toby toby 6606848 Sep 28 08:09 /tmp/tmp.TPEoOz9AWP
    323.44user 12.88system 5:37.65elapsed 99%CPU (9980maxresident)k

I actually have no idea why I'm running faster. Perhaps
``termprob_compare()`` is quicker that ``compare_by_probability()``? But
the important point is that I am now definitely better at categorizing
emails. Further improvements will come from better token selection, I
hope.

2015-09-26
==========

Oh you silly man! The probability list uses a custom comparison
function, ``compare_by_probability()``, which does indeed pick out most
significant (furthest from 0.5) probabilities.

So at this stage I've more or less convinced myself that most of the
maths is as suggested by Graham. Two things I still want to play with:
first, Graham clamps the probability range to (0.01,0.99), while Oggie
uses a dodgy looking float comparison to clamp to (0.00001,0.99999).
Secondly, I think we should use doubles throughout.

(Graham uses ``(min 1 (/ b nbad))`` which has no equivalent in Oggie's
code. Since ``b <= nbad``, the only time I can see that making any
difference is if ``nbad == 0``, in which case we avoid the division by
zero. I'm not sure how Oggie avoids division by zero here, but at some
point I intend to declare that p == 0 unless you've trained at least *n*
reals and spams.)

On that subject, I think ``corpus-test`` needs to train rather more
messages if its results are to be meaningful. If I bump ``ntrain`` up to
50, and sticking to 3000 ``MAX_TOKENS``, I get:

    ham: 95.20% correct, spam: 81.10% correct
    -rw-------. 1 toby toby 6606848 Sep 26 09:37 /tmp/tmp.JkxAf33sAU
    276.65user 11.57system 4:48.21elapsed 100%CPU (9732maxresident)k

OK. Now, change ``float`` to ``double`` and...

    ham: 19.20% correct, spam: 98.50% correct
    -rw-------. 1 toby toby 6606848 Sep 26 10:09 /tmp/tmp.2vErSShMmb
    275.40user 11.69system 4:47.05elapsed 100%CPU (9768maxresident)k

What!?!

Looking at some examples, it seems that all the chosen terms are spam
ones.  With this, still relatively small, training corpus, almost all
the significant terms have been clamped. I need to refactor and write
some tests, but presumably ``compare_by_probability()`` in the
``double`` version always finds 0.99999 is (very fractionally) more
significant than 0.00001. And, presumably, in the ``float()`` version
they're the same, so we get an arbitrary choice.

I wrote ``problist_dump()`` to examine the situation, and yeah, that's
basically true. (The choice is not quite arbitrary, but depends on the
length of the term.)

Now, all this got me thinking. Particularly with the rather small
training sets that I'm currently using, just about every significant
term will be clamped, because it will either appear only in spams or
only in reals. Look at what happens if all the terms are clamped, first
to Oggie's 99.999%::

    00 1.000000
    ...
    06 1.000000
    07 0.999990
    08 0.000010
    09 0.000000
    10 0.000000
    ...
    15 0.000000

And if we use Graham's 99%, that doesn't help much::

    00 1.000000
    ...
    05 1.000000
    06 0.999999
    07 0.990000
    08 0.010000
    09 0.000001
    10 0.000000
    ...
    15 0.000000

Consider a message which has 20 clamped terms, 10 near 0 and 10 near 1.
We should assign p=0.5, as we have absolutely no idea whether or not
this is spam.  But in fact we will pronounce with near certainty that it
is either spam or real; the choice will be arbitrary and fragile.

Graham mitigates this problem by insisting that a term has been seen at
least 5 times in the training corpus (otherwise we'll just assign its
occurrence in the message the standard 0.4, which is likely to knock it
off the top 15 list).

I have a more sneaky idea. What if we look at the total number of
occurences of a term, ``nspam + nreal``. Fold this down in some way,
such as ``floor(log(nspam + nreal))``, and call this ``rank``. Now, sort
first by rank, then the current criteria (modified to consider
probabilities within a delta to be equal). Let's try that...

OK, so the highest ranked terms are all short common words, "of",
"have", etc. I can't see this working out too well, but who knows?
We're still training 100 messages, with 3000 tokens::

    ham: 99.40% correct, spam: 11.50% correct
    -rw-------. 1 toby toby 6606848 Sep 26 22:38 /tmp/tmp.ebqR2rJGGU
    286.03user 11.75system 4:58.13elapsed 99%CPU (9892maxresident)k

So this looks like a classic case of estimating p too low. Or is it that
the threshold of 0.9 is too high? ::

    X-Spam-Words: 3002 terms
     significant: on (0.4154) br (0.5606) href (0.5500) the (0.4524) at (0.4531) in (0.4595)
    X-Spam-Probability: NO (p=0.676646, |log p|=0.390607)

Suppose the threshold were 0.5, rather than 0.9? ::

    ham: 98.50% correct, spam: 22.70% correct
    -rw-------. 1 toby toby 6606848 Sep 26 22:57 /tmp/tmp.MLCk8gxap3
    288.09user 11.97system 5:00.34elapsed 99%CPU (9896maxresident)k

OK, well I think the rank idea is basically a good one, but needs more
work. The fundamental problem at this stage is I have 2 different
dimensions of *significant*, and I need a more subtle way of combining
them. Or not... how about just ignoring any probs in (0.4 - 0.6)? That's
where all the high-ranking but neutral words seem to end up::

    ham: 84.90% correct, spam: 76.80% correct
    -rw-------. 1 toby toby 6606848 Sep 26 23:18 /tmp/tmp.RCuF9qXLz4
    309.98user 12.67system 5:22.90elapsed 99%CPU (9892maxresident)k

Well, those are the most promising results I've had in a while (and that
was with the threshold still at 0.5).

2015-09-25
==========

The refactoring continues. I've started pulling out the code that
actually calculates the probability, and as far as I can tell it only
considers the 15 terms (``nsig``) with the lowest probability. This
seems extraordinary. What happens if we bump it up?

With MAX_TOKENS 300, and nsig 30:

    ham: 87.80% correct, spam: 62.90% correct
    -rw-------. 1 toby toby 561152 Sep 25 22:19 /tmp/tmp.DXQoavDWBe
    11.34user 5.25system 0:16.33elapsed 101%CPU (3480maxresident)k

That's rather better on hams, but much worse on spams, which I can't
immediately account for. What if we consider *all* the terms?

    ham: 99.90% correct, spam: 4.40% correct
    -rw-------. 1 toby toby 561152 Sep 25 22:22 /tmp/tmp.pK2ICNFKIe
    11.31user 5.28system 0:16.33elapsed 101%CPU (3516maxresident)k

Oh. It's just getting the sums wrong. Which makes me think that the
bayes calculation is actually bogus, because it surely shouldn't matter?
Ah, hmm. I think that's because we assign 0.4 to never-seen tokens.

OK. `Graham says`_ "I only use the 15 most significant [tokens]". But,
as far as I can tell, Oggie is using the 15 tokens with the lowest
probability. That's surely not the same thing as significant? Indeed
not...

    "Another effect of a larger vocabulary is that when you look at an
    incoming mail you find more interesting tokens, meaning those with
    probabilities far from .5. I use the 15 most interesting to decide
    if mail is spam."

.. _graham says: http://www.paulgraham.com/better.html

Go back to 15 terms (this is barmey at this stage, but oh well) and
throw in the ideas of doubling the counts for good emails, and needing
the count to be > 5 before we do anything.

2015-09-22
==========

OK, the test suite is coming along. Next, I think I need to completely
automate the corpus tester. Obviously I won't be checking my entire
corpus into the bfilter git repo, but I want to get to the stage where I
can point it at a directory containing ``ham`` and ``spam`` subdirs, and
it churns away till it produces some numbers.

Done. Oh, I also want to report the size of the database. Observation:
my corpus is way too big for this sort of thing.

First results:

    ham: 98.28% correct, spam: 62.60% correct

Which at least has a very low rate of false positives.

Another way to arrange the corpus test would be to take messages in date
order, mixed, classify each one, then train mistakes. (Hmm... ultimately
I want to end up with UNSURE as well as YES and NO.) But let's not worry
about that now.

Right, I've cut my corpus down to 1000 each (pretty much at random, not
reviewed). Now I can classify 40 messages and test 2000 in reasonable
time.

First results, with MAX_TOKENS 300:

  ham: 81.00% correct, spam: 76.80% correct
  -rw-------. 1 toby toby 561152 Sep 23 22:13 /tmp/tmp.CVxtp72ShT
  11.35user 5.17system 0:16.22elapsed 101%CPU (3764maxresident)k

And with MAX_TOKENS 3000:

  ham: 84.10% correct, spam: 84.30% correct
  -rw-------. 1 toby toby 3379200 Sep 23 21:59 /tmp/tmp.C47usqoJTU
  93.03user 9.34system 1:42.12elapsed 100%CPU (6648maxresident)k

So, that's roughly 6x slower, and 6x more data, for a useful improvement
in accuracy.

One random thought that's occurred to me is that bfilter is perhaps too
picky about what's allowed in a token, and will have a hard time with
the modern trend for masking words like "c0ck".

Another random thought: I could use Oggie's rather splendid state
machine (non)-parser to build something that looks for urls in email
messages. As both the URL blocklist idea and the "fresh bread" (is it?)
idea are really rather good. Obviously this would be a separate tool to
bfilter.

On that note, I need to continue the job of splitting things off and
writing test frameworks for them (and ultimately making them into a
library). There's skiplist which is already independent, just needs the
testery. And there's the calculation of the probability itself. I'm
currently suspecting that this may not be quite right, as it seems to
clamp very close to 0 or 1 a lot of the item. (However, most times that
I've doubted Oggie's code, I've been wrong, and the code right.)

2015-09-20
==========

I'm gradually carving this thing up "at the joints". For example, I've
finally managed to extract the function that actually adds a token to
the skiplist. (I think this had suffered when the token history feature
was added.)

It's occurred to me that I can (and should) have both unit tests and
integration tests. For example I can test the ``compose()`` function in
isolation, and as part of the ``read_email()`` -> ``tokenize()`` ->
``compose()`` chain. The only tricky part is getting the makefile to put
everything together in the right order.

2015-09-16
==========

It's all very well to carve out the tokenizer, and pass it a pointer to
the function it should call for each token. But next I want to add tests
for the ``read_email`` function (which calls the tokenizer).

Would this approach work? The function that ``tokenize`` calls is always
called ``submit`` (say), which has a declaration in ``submit.h``, and a
definition in ``submit.c``. So ``token.c`` includes ``submit.h``.  Now
for bfilter, we link ``token.o`` and ``submit.o``, but for the test case
``unit/token.c`` can provide its own definition of ``submit()``, and the
linker sorts it all out.

Yes, of course that works, and will be much simpler to deal with.

2015-09-13
==========

Before I can make much progress with this program, it needs a test
suite. For example, I want to tweak the tokenizer, but basically I've
now become completely dependent on TDD. (Even if I hadn't, we need a
test suite.)

But before I can do *that*, I need to refactor the code somewhat. At
present, the tokenizer is in ``bfilter.c``, which also contains
``main()``. Let's see if I can mend that. Yes, nothing too painful.

2015-09-11
==========

I have a new version which Oggie developed but never published. The key
difference seems to be that it considers strings of tokens, such as "the
contents of". I'm unclear exactly what the rules are at the moment. (Of
course, dspam does this, with bells on, and Paul Graham recommends it.)

It also reports |log p| which helps to distinguish very low scores. For
example::

  X-Spam-Probability: NO (p=0.000000, |log p|=80.595810)

  X-Spam-Probability: NO (p=0.000000, |log p|=126.644783)

(However, this mapping does nothing for numbers close to 1. I think I
shall devise something more symmetric. I think the function I want is::

  map p | p < 0.5 = 1 + 1 / -p * 2
        | otherwise = -1 + 1 / ((1 - p) * 2)

This maps range (0-1) onto the entire number line. So 0.1 => -4, 0.4=>
-0.25, 0.5 => 0, 0.6 => 0.25, 0.9 => 4, 0.95 => 9, 0.99 => 49, etc.)

Some results. Trained on 20 each ham and spam. Correctly identifies
88.1% of ham corpus, and 75.9% of spams.

(Random observations: we still seem to be seeing multipart separators as
tokens. And, there is really no point in having pure numbers as tokens,
e.g.  30, 4.2.2, 166.90. *Particularly* because of the 300 token limit,
this is bad news. On further investigation, such things are discard in
``submit()``, but I shall probably move these tests to ``tokenize()``.)

After training 5 more spams (although probabilities very close to 0), it
is now correct on 89.9%. (As you might expect, training spams does not
help to identify hams: we now get only 77.1% of those right.)

Train another 5 hams, and we're at 84.7% hams, 83.7% spams.

These results are startlingly close to my previous ones. This suggests
that the multi-token approach is buying very little, which I find
surprising.

One simple thing I'd like to try is bumping up the maximum number of
tokens.

(Another thing I'd like to experiment with at some point is
https://karpathy.github.io/2015/05/21/rnn-effectiveness/ - could we
possibly use a neural network instead of bayesian filtering?)

Hmm... it would be nice to have some figures from dspam to compare these
to. I could actually do that rather easily on lithium, just by using a
new user id. Copy up the same corpus, so I'm training exactly the same
messages. Observation: dspam is *really* slow. Haven't timed it
properly, but it's of the order of 1 second to classify a message. Which
means that classifying my whole corpus (~25000 messages) will take all
day.

Oh hey! Another observation: dspam is apparently hosted on sourceforge,
and it says "Last Update: 2014-07-24". That looks like a moribund
project. :-( Surely someone will rescue it?

Also, a lot of messages are "Whitelisted". IIRC, it whitelists a sender
after 10 messages or so, which is not unreasonable. (I'd been thinking
that whitelisting after a single message is wrong.)

Argh! After training 20 of each (846 / 492) dspam is claiming that
everything is innocent. I presume it needs to be trained on some minimum
number of messages before it will commit itself, but I can't immediately
see what that number might be.

OK, let's try 60, magic numbers are 564 and 328. Nope. How about 100, at
338 and 197? Nope, even after training 50 messages of each sort, it
still claims everything is ham! Do I need to run it as root? Oh, now
it's saying (well, logging, which is almost the same thing) "Unable to
determine the destination user".

OK. I'm getting a bit bored of this. Despite having a working dspam
installation to hand, I cannot work out how to train and test a few
messages!  Complaints about dspam's documentation are rife. There is a
reasonable document here_, and the man pages, but it's still
impenetrable. (What, for example, is the difference between
``--classify`` and ``--deliver=summary``?)

.. _here: http://wiki.linuxwall.info/doku.php/en:ressources:dossiers:dspam

Back to bfilter. Bump up the maximum number of tokens to 3000, and
repeat the tests. After training 20 of each, I now get 98.2% hams right,
which looks very promising, but a mere 63.3% of spams. 

2015-08-26
==========

I've been testing bfilter on my spam corpus. The results are impressive.
I trained a random 10 hams and 10 spams. After such modest training,
bfilter then correctly identified 12787 / 15864 ham messages (80.6%). I
looked at a few of the false positives. One was, in fact, previously
misclassified spam. The next few were "near spams", legitimate
advertising messages from businesses that I had previously dealt with.

I trained a couple of these near spams, and now bfilter correctly
identified 13436 (84.7%) of the hams. At this point, I looked at my spam
corpus, and bfilter correctly identified 7288 / 9729 (74.9%) of them.
Again, I trained two more messages, and the hit rate rose to 8744
(89.9%).

So these initial results look promising. The number of false positives
is a bit worrying; as Paul Graham points out, we should avoid these at
all costs. Probably we just need to always say NO till a minimum number
of messages have been trained, where the minimum might be around 50.

Also, bfilter is finding more infelicities in my corpus. It complains of
a few (supposedly ham) messages: `failed to read email (no system
error)`. On investigation, the messages in question all look like this:

    Received: from 46.235.225.115 [95.70.92.180] by mx.flare.email
      with SMTP; 15 Apr 2015 18:22:15 -0000
    Message-ID: <6[10
    Date: 15 Apr 2015 18:22:15 -0000

I would really like to know where such a thing came from, but bfilter is
right that it shouldn't be in my corpus.

Bfilter treats its input as mbox format, which means it goes wrong on
maildir messages that contain /^From /.

I repeated the test with 20 hams and 20 spams. Incidentally, the runes
to do this are to count the messages in the corpus, divide by 20 (or
whatever), then:

    less `{ls | awk 'NR % 486 == 0 { print }'} # manually check first
    for (m in `{ls | awk 'NR % 486 == 0 { print }'}) sed 's/^From />From /' $m | bfilter isspam

First run of the whole corpus after this training gets 87.4% correct on
the hams, and 73.6% of the spams. This seems a bit disappointing, as it
the results with 40 messages trained don't seem much better than with 20
messages. But presumably the problem is that we're training
uninteresting messages.

I've now trained an additional 5 spam messages, each of which had *p=0*.
Those extra 5 spams give me 90.7% correct on the spams, and 74.1% hams.
Not a vast improvement. 

Hmm... on reflection maybe I should be training messages wrongly
classified at *high* probability... too late now, but note that the
entire state of the filter lives in a single file, so it would be
trivial to copy that to compare. (Yay to bfilter! Boo to dspam and its
postgresql database! Boo to crm114 and its homegrown multi-file stuff!)

Noticed something odd: bfilter appears, at least sometimes, to be
annotating the inner parts of multipart MIME messages. Which:
1. means that all my counts and percentages are likely wrong; and
2. demonstrates that bfilter is buggy.

First item on the todo list will be to add a "whole message" flag. I
never want to treat the input as an mbox, although I don't suppose I
should remove that functionality.
