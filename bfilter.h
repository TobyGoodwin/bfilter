/*

    Copyright (c) 2003 - 2004 Chris Lightfoot. All rights reserved.
    Copyright (c) 2015 - 2016 Toby Goodwin.
    toby@paccrat.org
    https://github.com/TobyGoodwin/bfilter

    This file is part of bfilter.

    Bfilter is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Bfilter is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with bfilter.  If not, see <http://www.gnu.org/licenses/>.

*/

#include "skiplist.h"

/* Current database version, and lowest version that this code can use */
#define VERSION 3
#define MIN_VERSION 3

/* Global variables are bad, m'kay? */
_Bool flagb;
char *flagD;

/* token_list is the list of tokens we find; each key is associated with a
 * struct wordcount which stores nemail, the highest-numbered email in which
 * this word was found, and n, the total number of emails in which this word
 * has been found during this session. */
int nemails, ntokens_submitted;
size_t nbytesrd, termlength;
skiplist token_list;
