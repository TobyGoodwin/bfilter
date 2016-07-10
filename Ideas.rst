Ideas Are Cheap
===============

* use scaffold
* have a ``cleandb`` that removes tokens with a count of one [1]_
* support multiple sources of classification
* way to remove a classification, temporary and permanent
* option to output full results array 
* add a ``term.context`` column, e.g. ``body``, ``from``, ``subject``

.. [1] Using a query similar to ``select term.term, count.term, sum(count.count) from term join count on term.id = count.term group by count.term having sum(count.count) < 2 limit 50;``
