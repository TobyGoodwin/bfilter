2015-11-01
==========

Inspiration! Make bfilter use scaffold.

Here we currently are::

    ham: 99.30% correct, .40% unsure; spam: 47.80% correct, 11.80% unsure
    -rw-------. 1 toby toby 561152 Nov  1 22:33 /tmp/tmp.ECWZEcQb5H
    9.53user 4.98system 0:14.36elapsed 101%CPU (4516maxresident)k

What happens if we reject 2-character tokens? This::

    ham: 99.30% correct, .40% unsure; spam: 47.80% correct, 11.80% unsure
    -rw-------. 1 toby toby 561152 Nov  1 22:34 /tmp/tmp.zAf01i273R
    9.39user 5.06system 0:14.29elapsed 101%CPU (4364maxresident)k

So that's fine. And what about rejecting tokens that are all digits? ::

    ham: 99.20% correct, .50% unsure; spam: 48.60% correct, 12.20% unsure
    -rw-------. 1 toby toby 606208 Nov  2 19:40 /tmp/tmp.Hb9fEBX3Lq
    9.51user 5.56system 0:15.36elapsed 98%CPU (4604maxresident)k

Wow. That deserves an A/B test.

2015-10-29
==========

Right. Major cleanup time.

I need to revise the database formats quite a lot. I think I want to
reintroduce the timestamp on terms, as I think it's a useful feature
(but 28 days is *far* too short a time; maybe a year).

More importantly, we need a single ``__classes__`` record that holds
codes, counts of terms, and counts of emails. (Yes, that means it will
need to be rewritten every time we train a new message.)

OK, that's up and running.

Mainly fiddling at the moment. I simplified ``token.c`` right down, and
removed the ``any isalpha`` test, as it seemed counterproductive
(especially with the rather limited idea we have of what an alphabetic
character is in the Unicode world). But I do want to reintroduce a test
for ``all isdigit``, since there really is no point in scoring tokens
like ``55`` and ``179``.

However, I need to actually get the thing working again before I can do
that...

Ah. Hmm. The classes interface is unpleasant. The problem is that
``class_lookup()`` may need to add a new class, which means changing the
list of classes. So it needs an interface like ``struct class
*class_lookup(struct class **classes)``. The other alternative I can see
is for ``class_fetch()`` to always ensure that there are *two*
sentinels, so we can use one for our new class. That's actually less
code...

Bleargh, horrid memory bug. Now fixed. Looks like some of the tokens
we're generating are still bogus, but at least we're running the
corpus-test to completion again.

2015-10-27
==========

I think my next thing should be to cleanup, and integrate with flare, so
I can see how it goes in "real life". I was just quickly playing with
adding a new class. Although it does look like it may take a fair bit of
training before the filter gets it right, the Frank&B suggestion doesn't
seem to help much.

Hmm. I think I could invent my own normalization (0f604f0)::

    ham: 99.90% correct, spam: 40.50% correct
    -rw-------. 1 toby toby 561152 Oct 27 22:38 /tmp/tmp.f6T7itJzZE
    10.05user 5.56system 0:15.77elapsed 98%CPU (4564maxresident)k

Huh. Those numbers seem familiar. So basically we take ``(1 + Tct) /
t_class``, and that gives the same results as ``(1 + Tct) / (t_class +
n_class)``, where n_class is the number of documents in th class (so +1
for each document). It seems strange that those produce the same
results.

Anyway, there's absolutely no point in normalizing when we're getting
99.9% of the smaller class right, since normalizing can only pull more
documents into the smaller class, which has already got all the docs it
should have. Darn.

Another sort of normalizing: what about normalizing so we get
probabilities that sum to one? If do that, we need the logprobs to sum
to 0... no, that would make the probabilities *multiply* to one. Sum to
-1/n (where n is the number of logprobs)? I kindof plucked that out of
the air, is it right? No, no kindof addition in log space is going to
work.

So we could make the logprobs sum to 0, by adding the arithmetic mean of the
logs to each one. Then exponentiate, then, errm, now we want to scale
geometrically to make the sum one. Oh yeah, multiply by 1/sum(probs).

Or I could try and find out what the P(d) that F&B talk about is.
(Suspect it's just a fudge factor like the one I'm sketching, but still.)

Hmm. So the problem here is that we sometimes end up with large
differences in the logprobs, which can lead to overflow when we convert
back to linear space. Most of the time, the final answers are just 0.0
and 1.0 anyway.

Essentially, all the interest happens when the range of logprobs < 5
(d756ae4)::

    lognorm: -978.824557 => -1.734412
    lognorm: -975.355733 => 1.734412
    linnorm: -1.734412 => 0.176504
    linnorm: 1.734412 => 5.665595
    score(spam): 0.030212
    score(real): 0.969788
    
    mean probability = -442.980259
    lognorm: -443.752219 => -0.771960
    lognorm: -442.208299 => 0.771960
    linnorm: -0.771960 => 0.462107
    linnorm: 0.771960 => 2.164003
    score(spam): 0.175966
    score(real): 0.824034

So... if we look for a range of < 5, and declare that "unsure", we get
180 unsure cases in the test corpus. With a range < 3, 122 cases. Let's
go with that (97a2d76).

Tidying up a bit, that looks quite promising. For spams, I get
right / wrong / unsure of 478 / 404 / 118, and for hams 993 / 3 / 4. So
the unsure ones are almost all the spams. Of the hams, 2 of the unsure
ones were previously wrong (ce5f149)::

    right 478
    wrong 404
    unsure 118
    right 993
    wrong 3
    unsure 4
     
    ham: 99.30% correct, spam: 47.80% correct
    -rw-------. 1 toby toby 561152 Oct 28 22:56 /tmp/tmp.Iby3mae1is
    9.52user 5.44system 0:15.01elapsed 99%CPU (4500maxresident)k

2015-10-26
==========

I should probably get into the habit of committing whenever I'm about to record
some results. At one point I had 98.4 / 64.0, which is better than I've got
now, but I'm not sure how I did that exactly.

OK. Here's where we currently are (32eb323)::

    ham: 99.90% correct, spam: 40.50% correct
    -rw-------. 1 toby toby 561152 Oct 26 21:14 /tmp/tmp.2JqhwhzfM8
    9.30user 4.99system 0:14.12elapsed 101%CPU (4384maxresident)k

Getting rid of normalization, I get this (0dc8b73)::

    ham: 99.50% correct, spam: 53.90% correct
    -rw-------. 1 toby toby 561152 Oct 26 21:16 /tmp/tmp.Z3jZL9X044
    9.45user 4.92system 0:14.18elapsed 101%CPU (4368maxresident)k

Cleaning up alpha, and going back to normalizing (eb8ae65)::

    ham: 99.90% correct, spam: 40.50% correct
    -rw-------. 1 toby toby 561152 Oct 26 21:29 /tmp/tmp.etgLvgImyD
    9.87user 5.37system 0:15.22elapsed 100%CPU (4352maxresident)k

So. ISTM that we had much better results when we just used a ``t_total``
instead of ``t_class + t_total`` compared to normalization. What is it
that normalization is doing that is different from that? Oh, it uses
``t_class``.  Change that to ``t_total``, and I get (74b6bea)::

    ham: 87.10% correct, spam: 88.90% correct
    -rw-------. 1 toby toby 561152 Oct 26 21:32 /tmp/tmp.YzxAj7CD1A
    9.93user 5.56system 0:15.66elapsed 98%CPU (4388maxresident)k

Which makes me think I've completely misunderstood this normalization
step. Let me peer at that paper *again*.

OK, well there's this::

    -            norm = alpha * (1. + Tct) / t_total;
    +            norm = alpha * (1. + Tct / t_class);

Make any difference? (20dec18)::

    ham: 0% correct, spam: 100.00% correct
    -rw-------. 1 toby toby 561152 Oct 26 21:47 /tmp/tmp.6m3AK1iXXs
    9.72user 5.25system 0:15.47elapsed 96%CPU (4392maxresident)k

Um. Possibly an int / double issue. Yes, fixing that I get (0eef8c2)::

    ham: 98.00% correct, spam: 20.50% correct
    -rw-------. 1 toby toby 561152 Oct 26 21:56 /tmp/tmp.5D1vSxivSz
    9.93user 5.47system 0:15.52elapsed 99%CPU (4400maxresident)k

And rearranging according to the comment "we have effectively replaced
the standard initial word count of one by the class- specific initial
word count ...", I get the same answers (0888615)::

    ham: 98.00% correct, spam: 20.50% correct
    -rw-------. 1 toby toby 561152 Oct 26 22:12 /tmp/tmp.wo0Jk87cid
    9.33user 4.94system 0:14.10elapsed 101%CPU (4324maxresident)k

One other thing. I'm seeing "ham bias" (perhaps), but hams are the
*smaller* class (22904 / 33877 at present). So this is the opposite
problem to the one Frank and Bouckaert are solving.

So it seems that 99.5 / 53.9 is about the best I can do. I *may* run
into the Frank and Bouckaert problem when I start doing more general
classification... but then again I may not. 

One thing I did mean to experiment with was Graham's idea of tagging
terms with the header line they come from (as in ``subject*free``).

2015-10-25
==========

OK, so currently I'm seeing a "ham bias" in my classifier. I can think
of two possibilities. First, I have implemented the wrong algorithm (or
implemented the right algorithm wrongly). Secondly, the data are somehow
messing things up.

For the first point, I need to find another description of the NB
algorithm and compare with what I'm doing. For the second, I suspect
that the problem I noticed yesterday with some binaries being
"tokenized" may be significant. After all, the data suggest that spams
are about 50% larger than spams, which seem improbable.

Well now, the odd thing here is that I'm *not* seeing loadsa binary keys
in the database (hashing is turned off at the moment). What I *am*
seeing is lots of snippets of base64::

    LjJQNbbvylmaiu
    HFauLmrtygBWh/L3yAFF4coSw3NU2W0x
    DNdfIARrFSeAoN5

and it does appear that these are largely in spam messages. Easy enough
to find a test case, and yes, hunks of b64 are failing to be decoded.

I'll investigate that in a second, but there's a quick hack that should
almost completely mitigate the damage caused by that problem. Just to
recap where we are now (this is with HISTORY = 1, which I'm going to
stick to for the time being)::

    ham: 99.70% correct, spam: 51.00% correct
    -rw-------. 1 toby toby 704512 Oct 25 15:40 /tmp/tmp.UVcwCupTHh
    10.00user 5.37system 0:15.27elapsed 100%CPU (4572maxresident)k

    t_spam = 34178, t_total = 11508
    t_real = 22294, t_total = 11508

Hmm, quick hack didn't help as much as I would have liked, although it
did help::

    ham: 99.50% correct, spam: 53.10% correct
    -rw-------. 1 toby toby 704512 Oct 25 15:50 /tmp/tmp.WKBUK1PF4k
    9.78user 5.30system 0:14.98elapsed 100%CPU (4592maxresident)k

    t_spam = 33277, t_total = 10617
    t_real = 22290, t_total = 10617

I'd thought by adding ``+`` to token "dot" characters, lines of b64
would turn into single tokens which would then be rejected because
they're too long. But, no, the code *truncates* overlong terms, instead
of rejecting them. If we do reject::

    ham: 99.50% correct, spam: 54.10% correct
    -rw-------. 1 toby toby 561152 Oct 25 15:57 /tmp/tmp.Xa6ORCKmaC
    9.56user 5.20system 0:14.64elapsed 100%CPU (4588maxresident)k

    t_spam = 32272, t_total = 9538
    t_real = 22004, t_total = 9538

So. Meh. Why are we not spotting these hunks of b64? So as far as I can
see they all emanate from a single message which has a b64 block not
preceded by a blank line. (It is preceded by a line containing a single
tab character.) So this is basically nonsense, and I think the right way
to deal with it is to reject, not truncate, too-long tokens. (This
occurs before compose, so it's fine if a history composition produces a
token longer than 32 characters.)

Looking at token.c, I think most of the tests here are wrong. Let's
really simplify it::

    ham: 99.50% correct, spam: 53.90% correct
    -rw-------. 1 toby toby 561152 Oct 25 16:36 /tmp/tmp.xVXAOrhGYM
    9.65user 5.29system 0:14.82elapsed 100%CPU (4660maxresident)k

    t_spam = 33877, t_total = 10116
    t_real = 22904, t_total = 10116

OK. Well, the tokens I'm seeing in the database all look pretty
reasonable now. So. Let's look at that maths again.

I seem to have stumbled into `this problem`_.

.. _this problem: http://www.cs.waikato.ac.nz/~eibe/pubs/FrankAndBouckaertPKDD06new.pdf

So, next, let's see if we can implement MNB/PCN (Mulinomial Naive Bayes
with Per-Class Normalization). First results::

    ham: 99.90% correct, spam: 40.50% correct
    -rw-------. 1 toby toby 561152 Oct 25 21:49 /tmp/tmp.x3gJVLO97a
    9.33user 4.92system 0:14.06elapsed 101%CPU (4444maxresident)k

Now, that looks like we've still got ham bias, but is that actually so?
Is it just that the training set is too small? If I train 250 messages,
then I get::

    ham: 99.80% correct, spam: 77.20% correct
    -rw-------. 1 toby toby 1728512 Oct 25 21:50 /tmp/tmp.c1GeH8iGgv
    16.98user 6.41system 0:23.21elapsed 100%CPU (5356maxresident)k

which looks more promising.

For comparison, without normalization, 50 training emails gives me
this::

    ham: 99.50% correct, spam: 54.10% correct
    -rw-------. 1 toby toby 561152 Oct 25 21:57 /tmp/tmp.igX0o2nC4m
    9.41user 5.18system 0:14.47elapsed 100%CPU (4596maxresident)k

and 250 this::

    ham: 99.80% correct, spam: 78.40% correct
    -rw-------. 1 toby toby 1728512 Oct 25 21:58 /tmp/tmp.01asVKYnXb
    16.77user 6.66system 0:23.31elapsed 100%CPU (5400maxresident)k

Which all tends to suggest that normalization isn't helping much here.
(But it may do when I introduce additional classes. And the "ham bias" I
thought I was seeing is bogus - I think what's actually going on here is
that the real email corpus is much more predictable than the spam
corpus.)

Anyway. If I'm convinced that what's going on here is *not* ham bias,
then I'm getting damn fine results for hams. And although the spam
results are a bit disappointing, they're not really much worse than
anything I was getting with Graham's sums.

Let's put NTRAIN back to 50, and HISTORY_LEN back to 3::

    ham: 99.90% correct, spam: 44.10% correct
    -rw-------. 1 toby toby 5283840 Oct 25 22:05 /tmp/tmp.QmA2F9eRGb
    167.97user 9.93system 2:57.90elapsed 100%CPU (8028maxresident)k

Oh. Well. Hmm. (Note that this compares with the 99.9% / 40.5% result,
so it *is* an improvement, but modest.)

I'm failing to understand ``alpha``. Fiddling with it seems to make no
difference at all.

2015-10-24
==========

Right, well I've more or less got the MNBC implemented. It's pretty
grody, but I can clean it up once it works. At present, it doesn't work,
and it's starting to look like I've found a skiplist bug: it looks like
removing a key doesn't do what you'd expect.

However, it's just occurred to me that I can cheat. I can just increment
the data that is stored in the skiplist.

Yay! I'm now getting the right numbers for the worked example.

The message spam/1399905162.7935.hydrogen.mv6.co.uk in my corpus
produces a lot of bogus tokens. It contains a base64 encoded PDF, which
apparently isn't discarded by the istext test.

Anyway. Here are the very first results::

    ham: 99.90% correct, spam: 36.10% correct
    -rw-------. 1 toby toby 704512 Oct 24 22:20 /tmp/tmp.S9R5XLO90t
    11.55user 5.49system 0:16.90elapsed 100%CPU (4484maxresident)k

Obviously we're finding way way way too many hams, I don't know why.
Also, it seems to be embarrassingly quick. I was worried that it would
be too slow, but if it's actually doing as much work is it's supposed to
it's unbelievably faster. Hmm.

That was with HISTORY_LEN of 1. Let's put that back to 3 and see what
happens::

    ham: 99.90% correct, spam: 29.30% correct
    -rw-------. 1 toby toby 5283840 Oct 24 22:45 /tmp/tmp.ePtFvOxvoj
    80.20user 7.61system 1:27.81elapsed 100%CPU (8148maxresident)k

OK, well, that's more reasonable for time.

Now, I sort of see what's happening. For terms that aren't in the
training vocabulary (the vast majority of course), we get::

    condprob[spam][16n] = 6.09333e-06
    condprob[real][16n] = 7.09829e-06

Why's that? Oh, we shouldn't be counting these terms at all. OK. So that
helps::

    ham: 98.40% correct, spam: 64.00% correct
    -rw-------. 1 toby toby 5685248 Oct 24 23:28 /tmp/tmp.o1y61wcK5Y
    82.68user 7.77system 1:30.52elapsed 99%CPU (7932maxresident)k

Hmm... why has the database changed size suddenly? Oh, well, no actually
the surprising thing is that it seemed to be exactly the same size
before. We're storing rather different data now. Meh.

Anyway, I still don't understand why we seem to have a bias for hams. (I
changed the order in which we train, and - as expected - that made no
difference.) Is it something to do with termsperclass?

Yes, I think so, inasmuch as if we equalize that, we get this::

    ham: 91.50% correct, spam: 82.90% correct
    -rw-------. 1 toby toby 5685248 Oct 24 23:39 /tmp/tmp.TxEaldLSgO
    78.59user 7.67system 1:26.29elapsed 99%CPU (8120maxresident)k

Which looks like the bias is gone. But surely the algorithm should work
without that? Is it because we're not actually considering all the
tokens? No, that doesn't help. Bother, this is the point where it
becomes clear (yet again) that I don't really understand this
probability stuff.

2015-10-21
==========

As predicted, it's a tedious lot of bit twiddling to get these more
complicated data structures into the database, but I've done the
trickier one.

*Both* my earlier ideas are wrong. Under ``__classes__``, we store the
names and codes. Then under every other key, we store a list of pairs:
code, and count. There's a special key ``__emails__`` that holds the
number of emails in each class, using the same list of pairs.

To get actual probabilities, I also need somewhere to store the total
number of terms (the vocabulary), and the total number of terms in each
class. Hmm.

In fact, let's not store a list of pairs, but simply a list of
``uint32_t``\s.  That makes for very simple code (currently I'm not
storing Oggie's timestamps either). It also means that we can use the
same store and fetch routines for the vocabulary total.

2015-10-20
==========

Oh! I've just had the most wonderful idea! Let's make bfilter a
*generic* classifier. Not just *real* or *spam*, but any classification
you care to train. This would require some changes to the database
format (but I don't care about backwards compatibility), and otherwise
just a few tweaks to the actual filter that I was going to make anyway.

Then, we can make flare zing!

OK, so what's the new interface look like? I think we just replace
``isreal`` and ``isspam`` with ``train CLASS``. For ``test``, we simply
report the class. For ``annotate``, we will generate a header something
like this::

  X-Bfilter-Class: spam (confidence 95%)

As far as the database goes, we'll need a key ``__classes__``. This will
consist of a pair of integers, followed by the nul-terminated class
name. The first integer is the count of documents in this class. The
second is the code of the class.

No. ``__classes__`` can just be a list of the class names. Then for each
class there's a key ``__class_NAME__`` holding the code and the count.
Then under each (hashed) term, we need to store a list of pairs: (code,
count) for each class where we've seen the token. Hmm... that's a nasty
lot of structure to put in the database.

Still, let's start writing some test cases.

2015-10-19
==========

I was thinking about the idea of recoding text. It goes like this.
1. Examine the text and decide if it is utf-8 encoded or not (this can
   be done with considerable confidence).
2. If not, then encode each 8-bit character to utf-8; effectively this
   assumes the encoding is iso-8859-1.

Suppose we don't do this? Then somebody such as myself, who sees a lot
of utf-8, some latin-1, and almost no other encodings, will suffer
slightly because a trigger word will have two possible encodings. So
recoding will help me, a bit, as it will bring together such words.

But for another user, let's say one who sees a mixture of utf-8 and
latin-5, recoding fails to bring together the same word encoded both
ways. On the other hand, it doesn't actually make things any *worse* -
there are still two possible encodings for each word, plain ol' utf-8,
and this new, bizarre thing. The bizarre thing wouldn't be at all
readable by humans, but it will still end up with the same set of bits
for the same word, which is all we care about.

So, I suppose from the above we should recode. But to be honest I'm a
bit bored of bit twiddling at the moment, and I'm sceptical it will make
much difference.

Back to A/B tests. As usual, some messages we earlier identified as spam
we now claim are ham. The first one on this list, there's *one single*
change in the 23 significant terms: we have added ``language%in`` with a
probability of 0.01. (Yes, this term does appear in the 2047-encoded
subject line.) And because we have a fine balance of 0.99 and 0.01
terms, this one change completely reverses the decision on this
message.)

Not much other change, actually. Anyway, I think I'm now at the point
where I'm interpreting the message as much as I want to, in other words
``read.c`` is just about done. I may tweak the
tokenization, composition etc.

And, more than any of those, ``bayes.c``. I'm still very unhappy with
the way this is working, particularly with regard to clamping. I've
found a `useful link`_ that I will need to study.

.. _useful link: http://nlp.stanford.edu/IR-book/html/htmledition/naive-bayes-text-classification-1.html

Note that I invented "Laplace smoothing" independently. :-) I turned it
off again, because it didn't seem to help, but let me try it again just
now::

    ham: 95.70% correct, spam: 68.80% correct
    -rw-------. 1 toby toby 5283840 Oct 19 22:11 /tmp/tmp.caBNccYYW9
    62.15user 7.39system 1:09.20elapsed 100%CPU (8296maxresident)k

Now, that's a fair bit better at hams... much worse at spams! But is
that because the threshold is too high? (Are we actually generating some
sane probabilities?) Now I have the A/B test to be able to tell easily.

No, it's not as simple as that. We still get polarized probabilities.
But the selection of significant terms is coming up *completely*
different. A few very common words, "of", "to", make it to the top
because they occur so frequently, even though they are close to neutral.

Maybe we just need to look at more terms? With SIGNIFICANT_TERMS 53::

    ham: 92.80% correct, spam: 76.30% correct
    -rw-------. 1 toby toby 5283840 Oct 19 22:32 /tmp/tmp.Xu2Bdtx3Kb
    62.25user 7.30system 1:09.10elapsed 100%CPU (8240maxresident)k

No. Time to go read that link carefully, I think.

2015-10-18
==========

Right. I think the last decoding I need to implement is MIME headers.
I'm not planning to handle arbitrary character sets, just utf-8 and
iso-8859-1. The latter is the only case we've had so far where a coding
produces a longer output than input, and is pretty horrid.

Also, we have to identify all the elements of ``=?...?...?...?=``,
because otherwise we go wrong if the qp data starts with ``=``.

Well, we have the most modest of improvements::

    ham: 92.00% correct, spam: 87.30% correct
    -rw-------. 1 toby toby 5283840 Oct 18 22:36 /tmp/tmp.1KDXFUQWtK
    64.26user 7.62system 1:11.50elapsed 100%CPU (8240maxresident)k


2015-10-17
==========

Numeric entity decoding implemented. *However*, I think I've run into a
problem with ``char`` versus ``unsigned char``. Hmm. Yes, it does appear
that plain ``char`` is signed. That means that all the stripping out of
``unsigned`` that I did a long time ago was totally mistaken. Bother.
Wonder if I can use ``<stdint.h>`` to make this less painful?
Specifically ``uint8_t``. Let's try it.

Hmm. Well, that compiles without warnings, but there are still some uses
of ``char`` that should be fixed. Aha! So ``token.c`` doesn't include
``token.h``. That's naughty. OK, I can believe the ``uint8_t`` changes
have percolated through the code now.

My current baseline, I think, is this::

    ham: 91.00% correct, spam: 88.20% correct
    -rw-------. 1 toby toby 5283840 Oct 11 22:52 /tmp/tmp.g2qZkHjBeT
    82.73user 8.53system 1:38.67elapsed 92%CPU (9188maxresident)k

And now we decode numeric entities::

    ham: 91.50% correct, spam: 87.60% correct
    -rw-------. 1 toby toby 5283840 Oct 17 22:06 /tmp/tmp.i7GrcOTORV
    62.77user 8.02system 1:19.14elapsed 89%CPU (8144maxresident)k

I have no idea why it's quicker. (Oh, well, maybe all the unsignedness
is good.) Lets look at A/B changes.

Hmm. So the tokenizer is still living in a Latin-1 world, and
considering any byte >= 0xa0 to be a valid token character. Since we're
still encoded as UTF-8 at this point, the only sane thing is to allow
any byte >= 0x80, so all UTF-8 encoded characters may be included. This
change actualy helps, ever so slightly::

    ham: 92.00% correct, spam: 87.30% correct
    -rw-------. 1 toby toby 5283840 Oct 17 22:24 /tmp/tmp.kaVGdZKOFE
    62.47user 7.56system 1:09.62elapsed 100%CPU (8196maxresident)k

Now, look at this, from the probabilities diff (not that these tokens
have actually changed between A and B)::

    MIME-Version%Content-Transfer-Encoding%quoted-printable => 0.990000, 0.010000 => 0.980051
    utf-8%MIME-Version%Content-Transfer-Encoding => 0.010000, 0.020000 => 0.980204

But first, why are they coming out in this order, when they're supposed
to be ordered by the radius descending? Oh, ok, because they're within
epsilon of each other. Bang epsilon down a bit. No, dammit, that makes
things worse!?!

And is it *really* the case that the first token has only appeared in a
single training message? (That happened to be a spam.) And the radius
stuff really ought to ensure that terms that have only appeared in a
single message are not significant. Let's double p_present (this kind of
makes sense, as we take ``p_spam * 2 - 1``, rather than ``p_spam -
0.5``). Now, if I also drop the threshold to 0.8, I get this::

    ham: 95.30% correct, spam: 67.60% correct
    -rw-------. 1 toby toby 5283840 Oct 17 22:40 /tmp/tmp.7AuXGObRbo
    63.38user 7.67system 1:10.64elapsed 100%CPU (8384maxresident)k

But that's disappointing too. Doubling p_present doesn't seem to be an
improvement. It occurs to me that perhaps I ought to consider the
threshold fixed at 0.5 for the time being, and tweak this at the very
end. Not that I think it matters a lot for now.

Now, OK, I think I've broken something here. For some reason, an input
that included ``#outlook`` would previously generate the token
``outlook``, but it no longer seems to. I'm a bit baffled by this. I
think it's a whole new class of integration tests.

(I'm also wondering about the future of tokenizing. It's still currently
rather ASCII orented, but teaching it about Unicode (and utf-8) would be
too much. What about going the other way, and making only the obvious
white space characters separate tokens?)

Right, got there in the end. It turns out that ``max_tokens`` is really
``max_terms``: the 3 tokens ``To view the`` turn into the 7 terms
``To``, ``view``, ``view``, ``To%view``, ``the``, ``view%the``,
``To%view%the``. Now that we decode HTML entities, we're generating more
tokens (such as, in this example, ``✓`` and ``£55``). These turn into
even more terms, which pushes some of the terms that were indicating
this message as a spam past the 500 limit.

If we increase ``MAX_TEST_TERMS`` to 1000, then, happy to say, that is
an all round improvement (except for speed)::

    ham: 95.20% correct, spam: 87.00% correct
    -rw-------. 1 toby toby 5283840 Oct 18 12:02 /tmp/tmp.T98cxfCiwz
    91.37user 8.37system 1:39.43elapsed 100%CPU (8228maxresident)k

2015-10-12
==========

Binary detection implemented. Makes no difference to the spam score. It
does remove ``ff`` from the words found in that Google email (but we
still judge it to be spam).

2015-10-10
==========

Added the -Dp flag, which makes ab-prob that much more useful. And now
add -Dt too. (I really ought to refactor bayes.c some more.)

I think I'll look at quoted-printable next. Should be easy. If we have a
``bdy`` line (but *not* ``bdy_b64``), then call ``cookqp()``, which
simply looks for ``=`` followed by 2 hex digits and replaces them
inplace. Done, and almost no movement (ham rate is up from 91.4%)::

    ham: 91.70% correct, spam: 85.20% correct
    -rw-------. 1 toby toby 2162688 Oct 10 22:52 /tmp/tmp.bOvqJuymUR
    46.74user 8.37system 0:54.62elapsed 100%CPU (7148maxresident)k

In fact, 6 messages have (incorrectly) changed from ham to spam, and at
least 10 the other way round. Tweaked ``ab-diff`` (was ``ab-prob``) to
look more closely at this. Aaaand, it turns out that the first ham
message I'm looking at is in fact spam, or at least borderline. It's
great that bfilter is finding these things, but also a bit annoying, as
replacing them is tedious (and makes previous statistics slightly
wrong).

Looking further, we're definitely picking out better tokens now:
nonsense terms like ``quoted-printable%3D`` and ``circular%economy%E2``
are gone. Ham->spam #2 just seems to be unfortunate.

In ham->spam #3, we have this, which I don't like::

    +wish%to%receive => 0.990000, 0.030000 => 0.980459
    +longer%wish%to => 0.990000, 0.030000 => 0.980459
    +no%longer%wish => 0.990000, 0.030000 => 0.980459
    +you%no%longer => 0.990000, 0.030000 => 0.980459
    +receive%this => 0.990000, 0.030000 => 0.980459

It just seems wrong that the single phrase "if you no longer wish to
receive this ..." contributes so much to the spam score. And now here's
something worrying. I trained that message, and (as expected) bfilter
now reports that it's real *but* the probability on ``wish%to%receive``
is still clamped at 0.99. How can that be?

Aha! I had TEST and TRAIN the wrong way round! That should put the cat
amongst the pigeons::

    ham: 91.00% correct, spam: 88.20% correct
    -rw-------. 1 toby toby 5283840 Oct 11 22:52 /tmp/tmp.g2qZkHjBeT
    82.73user 8.53system 1:38.67elapsed 92%CPU (9188maxresident)k

It's a fair bit slower, and slightly better at picking out spams. Um,
let's rewind to before qp::

    ham: 92.30% correct, spam: 88.80% correct
    -rw-------. 1 toby toby 5283840 Oct 11 22:59 /tmp/tmp.UFUqa7FiXf
    82.74user 7.94system 1:30.34elapsed 100%CPU (9260maxresident)k

Changes mainly seem to be noise, although it has picked out another
borderline message. I wonder if I'm just not training enough messages?
Suppose we train 250 each messages (25% of the test corpus)::

    ham: 98.40% correct, spam: 92.70% correct
    -rw-------. 1 toby toby 20185088 Oct 11 23:17 /tmp/tmp.9DITAEF7Xs
    495.74user 15.58system 8:32.36elapsed 99%CPU (23372maxresident)k

The extreme slowdown there is a touch disappointing. Obviously it's good
news that we're up to 98.4%, although that seems a bit low under the
circumstances. Actually, no, it's pretty good: of the 17 ham messages
marked as spam, 1 really is. About half are from the White House, not
quite sure why these are coming up as spam. About a quarter are from
Oxfam, purely due to their use of MessageFocusMailer (or some such). And
there's a tiny sprinkling of random ones (one Haskell cafe message
includes a long disclaimer with several spam key words).

I dunno. I guess I should press on with better tokenization:

* HTML entities;
* reject base64 that doesn't look like text;
* latin-1 => utf-8.

See where that gets me to. Then it will be a case of trying, once again,
to get my head round the probability stuff.

2015-10-08
==========

The rewritten ``read.c`` now handles base64 too. The code is cleaner,
more comprehensible, and more concise than the first version (I'll work
out some numbers in a minute for how much more concise). Not only that,
but Oggie's bas64 decoder worked a line at a time, so split words. Mine
avoids this flaw.

(But introduces a new one, which we may have to do something about: we
will actually construct any and all attachments, and feed them to the
tokenizer. While this shouldn't cause any problems (almost everything
will be discarded as too long), it's a lot of work that accomplishes
nothing.)

Now, there are still a few things that Oggie's state machine does and
mine doesn't. One is to discard any incoming ``X-Spam-Probability:``
header, which I will need to do. Another is to handle Berkeley mbox
``From_`` separators, which I suppose I need to do. Evil little corner
cases, the lot of 'em (especially Berkeley mbox).

Hmph. Actually, counting semicolons, the old ``read.c`` was 102 LoC, and
the new ``read.c`` + ``line.c`` + ``cook.c`` is 107 LoC. I'm struck by
how close those 2 numbers are. Still, I believe the new code to be
cleaner and clearer. (Hell, it's not full of "functions" inlined with
``#define`` and carefully placed so that all the variables they need are
in scope. (I'll optimize later. (If I need to.))) Oh, plus I handle
softeol.

So, it's time to see if soft eol, and also not breaking b64 words
randomly, actually helps to detect spams or not.

OK, so we have some seg faults. First thing is that the base64 decoder
assumes that it's being given a sensible number (== 0 modulo 4) of input
bytes. Second thing is that we do actually want to check that we have a
sensible number of bytes. If not, it presumably wasn't b64 after all.

In the particular case I looked at, the string "Vasya" occurred on a
line on its own.

So the last stats I had were::

    ham: 93.80% correct, spam: 85.30% correct
    -rw-------. 1 toby toby 2162688 Oct  3 09:19 /tmp/tmp.lV1plPO3pI
    67.21user 9.44system 1:16.65elapsed 100%CPU (6164maxresident)k

And now I'm seeing::

    ham: 91.40% correct, spam: 85.50% correct
    -rw-------. 1 toby toby 2162688 Oct  8 23:17 /tmp/tmp.3fTd5FQkZ6
    46.02user 8.26system 0:56.66elapsed 95%CPU (7124maxresident)k

Well, the first thing of note is that all that hard work trying to make
things quicker by contorting the syntax with ``#define`` was apparently
entirely wasted! My cleaner code, despite making a lot more function
calls, appears to be significantly faster.

Unfortunately, we're producing worse results faster. Must be A/B time...

OK. So the first 3 or 4 "most differing" results are emails from Quidco,
which are pretty close to spam. I looked closely at the "least spammy"
of the top 10 (it was actually a "new login from device blah" email from
Google). As far as I can tell, it's pure chance that we scored this as a
ham initially. Here are the 5 most significant terms::

    margin-top => 0.010000, 0.080000 => 0.983260
    sans-serif%font-size%10px => 0.990000, 0.030000 => 0.980459
    ght => 0.010000, 0.050000 => 0.981275
    tex%t-decoration%none => 0.010000, 0.010000 => 0.980051
    t-decoration%none => 0.010000, 0.010000 => 0.980051

Note that 3 of these involve word fragments. And they are all chunks of
CSS, which I'm not convinced is a terribly reliable indicator of spam.
In the new regime, we seem to be doing much better at choosing actual
words::

    ff => 0.990000, 0.050000 => 0.981275
    image/jpeg%name => 0.990000, 0.010000 => 0.980051
    in%your%account => 0.990000, 0.010000 => 0.980051
    and%determined => 0.990000, 0.010000 => 0.980051
    the%first%time => 0.990000, 0.010000 => 0.980051

It's just unfortunate that they seem to be very spammy ones. What is
``ff``? Well, this message contains 3 images. As predicted, they don't
seem to cause any serious trouble, but the only occurrence of ``ff``
occurs in a ``.png`` image. I think having decoded some b64, we need to
look at the result and try to guess if it might actually be text or not.
(In this case, and I suspect many others, simply checking for NUL bytes
would do well, although I actually have a test case that includes b64
null bytes... oh! or is that a bug? Yes, it's a bug, now fixed.) Merely
chucking out ``ff`` isn't going to change the classification of this
message though, sadly.

2015-10-07
==========

The rewrite of ``read.c`` is going well, and I'm confident the end
result will be much cleaner and more extensible than previously.

I've been mulling over Graham's comments about headers, and I at least
want to experiment with adding *every* header, prefixed by its name.
This means that we'll generate an awful lot of tokens like
``received*from``, ``received*haskell.org``, etc., and may need to bump
up MAX_TOKENS.

Still, before making any changes, I need to complete the
reimplementation.  That's soft EOL handling now working, and by way of
evidence that the rewrite is effective, I didn't even need to look at
the "engine", just add the new state and make minor tweaks to
``transition()``, ``maybe_save()``, and ``maybe_submit()`` to handle it.

Next will be base64, but that will have to wait till tomorrow.

2015-10-05
==========

I am going to rewrite ``read.c``. There are several things it needs to
do that it doesn't already, and the code is already too messy.

The basic idea is a mild extension and generalization of the existing
code for base64. Basically we will have input buffer, which is written
directly to output in passthrough mode. And there will be a separate
hold space, which may have transformations performed on it, and is
submitted to the tokenizer at appropriate points.

Transformations include:
* base64 decoding
* soft EOL folding
* q-p decoding
* html entity decoding
* interpreting things that can't be utf-8 on the assumption that they're
latin-1 (eek, this came out sounding a bit different from what I'd
hoped).

It would be *possible* to be more clever about character sets. It's
occurred to me that the state machine should be able to do a reasonable
job of spotting mime boundaries, and could then flip back into header
mode (or part-header, or something), and while in header mode it could
watch out for Content-Type: headers, and attempt to extract character
sets from them.

However, suppose we decide that a hunk of text is in fact in iso-8859-7,
what are we going to do with this information? I was thinking we'd have
to throw libicu at it, which I'm really not sure is a good plan. But for
the 8-bit sets at least, it wouldn't be too painful to have lookup
tables. 

Anyway, it's actually pretty easy to look at some text and determine
with high probability whether or not it is UTF-8.

Log of various changes.

* Having the character count (was ``j``, now ``l``) be the number of
non-\n charecters is dangerous. It means we have to use ``feof()`` to
discover the end of the email. More seriously, at that point ``j`` is
``(size_t) -1``, which is not a nice value to have floating around.

* There was both a ``passthrough`` flag, and a pointer to a ``FILE *``,
which both needed to be set for passthrough mode. The flag has now gone.

* The tests in ``test/read`` no longer ever enable passthrough mode, use
the more reliable ``test/pass`` for that.

2015-10-04
==========

Where are we at, then? Time for a todo list.

1. improve ``read.c`` and teach it more about quoted-printable
2. think about non-ASCII characters
3. look at bayes theorem some more
4. consider Graham's "better" ideas
5. add debug flags
6. replace the probability skiplist with a heap

For 1, I'm pretty certain I actually broke some things last night: it's
wrong to set ``end`` the moment we see eof, as we haven't processed the
last line. However, I haven't yet managed to produce a test case that
demonstrates a bug. I'm still vacillating between hack vs rewrite.

Number 2 is a bit worrying. Oggie's only concession to non-ASCII seems
to be that any character with the high-bit set is treated as a word
character. This might, possibly, just about, make things work by virtue
of UTF-8, but it's a bit pants. On the other hand, using UTF-32 for
everything would be a major change, and might just be over-engineering.
Definitely need to do something with RFC 2047-encoded headers.

On the topic of 3, I've been looking at `naive Bayes classification`_,
and I don't think we're doing it quite right.

.. _naive bayes classification: https://en.wikipedia.org/wiki/Naive_Bayes_classifier

By 4, I mean tweaks like using ``subject*foo`` as the token for the word
"foo" occurring in the subject line. These are tweaks, though, and not
worth doing till more substantial changes have occurred.

Adding debug flags is trivial, and will make things like the A/B test
much nicer.

In 6, I'm sure it's a win, but it is a performance hack that can wait
till much later in the day.

Looking at this, the highest priority must be to consider point 2. If
everything's going to shift to UTF-32, that's a *major* change, even the
test suite will need a lot of work. (For example, if we submit UTF-32
tokens, the "fake" ``tokenize()`` will need to convert back to UTF-8.
Well, or the sample outputs could be UTF-32... actually vim seems to
know about UTF-32 pretty well.)

The other option is to keep it all in UTF-8. In truth, that's probably
simpler for my short-term sanity, and frankly most of the mail I care
about *is* mainly ASCII, so -32 would just use more space. Although it
also affords me (and the rest of the english speaking world) the
"opportunity" to be sloppy about character encoding issues.

Gosh and golly gosh. I spent a while beating my head over naive bayes
classifiers, and rewrote ``bayes()`` to calculate something more like
what I was reading about. Initial results::

    ham: 92.00% correct, spam: 87.10% correct
    -rw-------. 1 toby toby 2162688 Oct  4 16:31 /tmp/tmp.z3o3AtgKMG
    50.42user 8.79system 1:06.20elapsed 89%CPU (6172maxresident)k

I really didn't expect anything as decent as that. Whether we're
actually calculating anything very much different, I'm not really sure.
I had been worrying again about the clamping in Graham's method, but
with the more standard NBC that I just implemented, the algorithm simply
tells you which class is the answer, so that's even worse!  I do think
it's optimistic to call the number we calculate *p(spam)*, and I'd
really like some measure of confidence, or way to produce an "unsure,
train me" answer. But I think for now I'll stick to Graham's maths, as I
don't have anything better.

I want another test framework: for the passthrough flag. There are some
tests in ``read/`` that are supposed to exercise this, but they rely on
the ``.out`` file exactly reproducing the ``.in`` file (with any other
output interspersed.) It would obviously make more sense to have
specific tests that ensure the output is byte-for-byte identical with
the input. And, good, this reveals the bug I made last night. (Fixing it
will have to wait till tomorrow.)

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

Argh! First attempt at handling soft eols joined lines together "in
place", which looked reasonable, but would completely break passthrough
mode. We will need a new state, and a separate buffer. (To be honest,
``read_email()`` is already a bit of a mess, and adding extra stuff is
unlikely to make it less messy, but I don't think I have the strength to
rewrite it at the moment.)

Well, that's disappointing::

    ham: 94.00% correct, spam: 76.90% correct
    -rw-------. 1 toby toby 2162688 Oct  3 22:14 /tmp/tmp.s5VYuQNnOq
    57.64user 9.16system 1:06.58elapsed 100%CPU (6164maxresident)k

Let's look more closely... oh, ah, it's bombing out half the time. This
is better::

    ham: 91.90% correct, spam: 87.10% correct
    -rw-------. 1 toby toby 2162688 Oct  3 22:43 /tmp/tmp.tV9kyr3eF4
    48.88user 8.50system 0:57.13elapsed 100%CPU (6116maxresident)k

As expected, we're better at spams, although only marginally. Sad that
hams have dropped though. OK, so there are several hams from quidco in
the top 10, and some other quasi-spams. (Actually, there's one that's
*so* close to being a spam that I'm tempted to replace it in the corpus
with a "better" ham. So that's actually a success of the new code!)

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
