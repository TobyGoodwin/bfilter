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
