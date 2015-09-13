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
this is bad news.)

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
