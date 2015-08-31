/*
 * skiplist.c:
 * Implementation of skiplists.
 *
 * Copyright (c) 2003 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: skiplist.c,v 1.3 2003/02/18 15:06:16 chris Exp $";

#include <sys/types.h>

#include <stdlib.h>
#include <string.h>

#include "pool.h"
#include "skiplist.h"
#include "util.h"


/*
 * Description: a skiplist is a collection of linked lists in which each node
 * contains a number of forward links which are used to skip over intermediate
 * nodes. The nodes are filled and connected randomly. Picture:
 *
 * height
 *        .------.           forward (`more') links           .------.
 *   2    |      | -----------------------------------------> |      |
 *        |      |                  .------.                  |      |
 *   1    |      | ---------------> |      | ---------------> |      |
 *        |      |     .------.     |      |     .------.     |      |
 *   0    | -inf | --> |  aa  | --> |  bb  | --> |  cc  | --> | +inf |
 *        `------'     `------'     `------'     `------'     `------' 
 *              <---------'  <---------'  <---------'  <---------'
 *                      backward (`less') links
 * 
 * Searching is easy. We start on the left hand end of the top list and
 * compare the key of the end of each `more' link to the search key. Move
 * right if the linked node is less than the search key, down if it is
 * greater, and return it if it is equal. We abort the search if we ever
 * try to descend below the bottom of the list.
 *
 * Insertion proceeds by searching for the node immediately smaller than
 * the new key. We then insert a new node having a random height; the height
 * is chosen with probability like 1/2**h. Then we link up the new key to
 * all its friends. If the chosen height is greater than the largest height
 * of the whole list, then we add lists to the top until we have one list with
 * only the two +/-inf members right at the top.
 *
 * Deletion is also simple. Again we ensure that there is only one two-member
 * list at the top.
 */


/* 
 * Definitions of the structures we use.
 */


/* struct skiplist_node
 * Each node contains a list of `more' pointers and a single `less' pointer,
 * in addition to its key and value. */
struct skiplist_node {
    void *k, *v;
    size_t klen;
    struct skiplist_node **more, *less;
    size_t height; /* height is the index of the highest list, not the
                      number of lists. */
};

/* struct tag_skiplist
 * Underlying implementation of a skiplist. */
struct tag_skiplist {
    int (*compare)(const void *k1, const size_t k1len, const void *k2, const size_t k2len);
    struct skiplist_node *min, *max;
    size_t nmemb, height;
    pool P;
};

/* xxpi, xxmi, PLUSINF, MINUSINF
 * Pointers used as placeholders for the special-case keys +\inf and -\inf. */
static int xxpi, xxmi;
#define PLUSINF     ((void*)&xxpi)
#define MINUSINF    ((void*)&xxmi)

/*
 * Various private utility functions.
 */

/* generic_compare BUF1 LEN1 BUF2 LEN2
 * Compare two buffers in the manner of strcmp. */
static int generic_compare(const void *k1, const size_t k1len, const void *k2, const size_t k2len) {
    int r;
    size_t n;
    n = k1len < k2len ? k1len : k2len;
    r = memcmp(k1, k2, n);
    if (r == 0) {
        if (k1len > k2len)
            return 1;
        else if (k1len < k2len)
            return -1;
        else
            return 0;
    } else
        return r;
}

/* node_new SKIPLIST KEY KLEN VALUE HEIGHT
 * Allocate a new node of the given HEIGHT for SKIPLIST, filling it with the
 * given KLEN-byte KEY (which may be either MINUSINF or PLUSINF, in which case
 * KLEN is ignored) and the given VALUE. */
static struct skiplist_node *node_new(skiplist S, const void *key, const size_t klen, const void *val, size_t height) {
    struct skiplist_node *N, nz = {0};
    N = pool_malloc(S->P, sizeof *N);
    *N = nz;
    N->more = pool_malloc(S->P, (1 + height) * sizeof *N->more);
    if (key != MINUSINF && key != PLUSINF) {
        void *k;
        k = pool_malloc(S->P, klen);
        memcpy(k, key, klen);
        N->k = k;
        N->klen = klen;
    } else
        N->k = (void*)key;
    N->v = (void*)val;
    N->height = height;
    return N;
}

/* random_number
 * Return a random integer appropriately distributed. */
static inline int random_number(void) {
    int i = 0;
    while (rand() < RAND_MAX / 2)
        ++i;
    return i;
}

/* do_compare SKIPLIST KEY KLEN NODE
 * Compare the given KLEN-byte KEY against the key of the given NODE according
 * to the comparison semantics of SKIPLIST. */
static inline int do_compare(skiplist S, const void *k, const size_t klen, const struct skiplist_node *n) {
    if (n->k == PLUSINF)
        return -1;
    else if (n->k == MINUSINF)
        return +1;  /* shouldn't happen? */
    else
        return S->compare(k, klen, n->k, n->klen);
}

/* find_node SKIPLIST KEY KLEN EXACT
 * Find the node in SKIPLIST with the given KLEN-byte KEY, returning NULL if
 * the KEY is not present. */
static struct skiplist_node *find_node(skiplist S, const void *k, const size_t klen) {
    struct skiplist_node *N;
    int h;

    /* Start at top node. */
    h = S->height;
    N = S->min;

    while (1) {
        int c;
        c = do_compare(S, k, klen, N->more[h]);
            /* XXX optimise PLUSINF case here? */
        if (c == 0)
            /* Next node has this key. */
            return N->more[h];
        else if (c < 0) {
            /* Next node has larger key. */
            if (h == 0)
                return NULL;
            else
                --h;
        } else
            /* Next node has smaller key. */
            N = N->more[h];
    }
}

/*
void skiplist_dump(skiplist S) {
    struct skiplist_node *ns;
    for (ns = S->min; ns; ns = ns->up) {
        struct skiplist_node *n;
        for (n = ns; n; n = n->more) {
            if (n->k == MINUSINF)
                printf("[ -inf ]");
            else if (n->k == PLUSINF)
                printf("[ +inf ]");
            else
                printf("%08x", (unsigned int)n->k);
            if (n->more) printf(" -> ");
        }
        printf("\n");
    }
}
*/

/*
 * Public interface.
 */


/* skiplist_new COMPAREFUNC
 * Initialise a new skiplist using COMPAREFUNC to compare the values of keys;
 * or, if COMPAREFUNC is NULL, use the generic comparison function. */
skiplist skiplist_new(int(*compare)(const void *, const size_t, const void *, const size_t)) {
    skiplist S;
    alloc_struct(tag_skiplist, S);
    
    S->compare = compare;
    if (!S->compare)
        S->compare = generic_compare;

    S->P = pool_new_size(32);

    /* First pair of nodes. */
    S->min    = node_new(S, MINUSINF, 0, NULL, 0);
    S->max    = node_new(S, PLUSINF, 0, NULL, 0);
    S->min->more[0] = S->max;
    S->max->less    = S->min;

    return S;
}

/* skiplist_delete SKIPLIST
 * Delete SKIPLIST and all associated memory. If all data have been inserted
 * with skiplist_insert_copy, this will free all associated data, too. */
void skiplist_delete(skiplist S) {
    pool_delete(S->P);
    xfree(S);
}

/* skiplist_delete_free SKIPLIST FREEFUNC
 * Iterate over every element of SKIPLIST, calling FREEFUNC with the value of
 * each, and then delete SKIPLIST itself. If FREEFUNC is NULL, use free(3). */
void skiplist_delete_free(skiplist S, void (*freefunc)(void*)) {
    skiplist_iterator itr;
    if (!freefunc) freefunc = xfree;
    for (itr = skiplist_itr_first(S); itr; itr = skiplist_itr_next(S, itr))
        freefunc(skiplist_itr_value(S, itr));
    skiplist_delete(S);
}

/* skiplist_insert_copy SKIPLIST KEY KLEN VALUE VLEN
 * Make a copy of VLEN bytes of VALUE data in SKIPLIST's private pool, and
 * insert a pointer to the copy under the given KLEN-byte KEY. */
int skiplist_insert_copy(skiplist S, const void *key, const size_t klen, const void *val, const size_t vlen) {
    void *v;
    v = pool_malloc(S->P, vlen);
    memcpy(v, val, vlen);
    if (!skiplist_insert(S, key, klen, v)) {
        pool_free(S->P, v); /* XXX */
        return 0;
    } else
        return 1;
}

/* skiplist_insert SKIPLIST KEY KLEN VALUE
 * Insert a new item described by KEY of length KLEN with the given VALUE
 * into SKIPLIST. Returns 1 on success or 0 if the named KEY already
 * exists. */
int skiplist_insert(skiplist S, const void *key, const size_t klen, void *val) {
    size_t newheight, j, h;
    struct skiplist_node *N, *Nnew = NULL;
    static struct skiplist_node **after;
    static size_t nafter;

    /* Choose a height at which to insert the new key. */
    newheight = random_number();

    /* Possibly insert new empty lists. */
    if (newheight > S->height) {
        int j;
        S->min->more = pool_realloc(S->P, S->min->more, (newheight + 1) * sizeof *S->min->more);
        for (j = S->height + 1; j <= newheight; ++j)
            S->min->more[j] = S->max;
        S->min->height = S->height = newheight;
    }

    /* Now we need to walk through the list, finding the nodes immediately
     * after which to insert this node. */
    N = S->min;
    h = S->height;

    if (!after || nafter < newheight + 1)
        after = xrealloc(after, (nafter = (newheight + 1) * 2) * sizeof *after);

    while (1) {
        int c;
        c = do_compare(S, key, klen, N->more[h]);
            /* XXX optimise PLUSINF case here? */
        if (c == 0)
            /* Next node has this key, so abort. */
            return 0;
        else if (c < 0) {
            /* Next node has larger key, so this is where we insert the new
             * node at this level. */
            if (h <= newheight)
                after[h] = N;
            if (h == 0)
                break;
            else
                --h;
        } else
            /* Next node has smaller key. */
            N = N->more[h];
    }

    Nnew = node_new(S, key, klen, val, newheight);
    Nnew->less = N;
    
    for (j = 0; j <= newheight; ++j) {
        Nnew->more[j] = after[j]->more[j];
        Nnew->more[j]->less = Nnew;
        after[j]->more[j] = Nnew;
    }

    /* All done. */
    ++S->nmemb;
    return 1;
}

/* skiplist_size SKIPLIST
 * Give the number of elements in SKIPLIST. */
size_t skiplist_size(skiplist S) {
    return S->nmemb;
}

/* skiplist_contains SKIPLIST KEY KLEN
 * Return nonzero if KLEN-byte KEY is present in SKIPLIST. */
int skiplist_contains(skiplist S, const void *key, const size_t klen) {
    return find_node(S, key, klen) ? 1 : 0;
}

/* skiplist_find SKIPLIST KEY KLEN
 * Returns the value associated with KLEN-byte KEY in SKIPLIST if it is
 * present, or NULL if it is not found. A NULL-valued key may be tested
 * for using skiplist_contains. */
void *skiplist_find(skiplist S, const void *key, const size_t klen) {
    struct skiplist_node *N;
    N = find_node(S, key, klen);
    if (N)
        return N->v;
    else
        return NULL;
}

/* skiplist_remove_copy SKIPLIST KEY KLEN
 * Removes the data referenced by the KLEN-byte KEY from SKIPLIST, and frees
 * the associated value, which is assumed to have been copied by
 * skiplist_insert_copy. Returns 1 if the data was deleted, or 0 if it was
 * not present. */
int skiplist_remove_copy(skiplist S, const void *key, const size_t klen) {
    void *v;
    v = skiplist_remove(S, key, klen);
    if (v) {
        pool_free(S->P, v);
        return 1;
    } else
        return 0;
}

/* skiplist_remove SKIPLIST KEY KLEN
 * Removes the data referenced by the KLEN-byte KEY from SKIPLIST. Returns the
 * value associated with KEY if it was present, or NULL otherwise. */
void *skiplist_remove(skiplist S, const void *key, const size_t klen) {
    struct skiplist_node *N;
    void *v = NULL;
    N = find_node(S, key, klen);
    if (N) {
        int i = 0;

        v = N->v;
        pool_free(S->P, N->k);

        N->more[0]->less = N->less;
        for (i = 0; i <= N->height; ++i)
            N->less->more[i] = N->more[i];

        /* XXX remove empty lists at top? */

        --S->nmemb;
    }

    /* All done. */
    return v;
}

/*
 * Iterators: forward/reverse iteration over sorted contents of list.
 */

void *skiplist_itr_value(skiplist S, skiplist_iterator itr) {
    struct skiplist_node *n;
    n = (struct skiplist_node*)itr;
    return n->v;
}

void *skiplist_itr_key(skiplist S, skiplist_iterator itr, size_t *len) {
    if (len) *len = itr->klen;
    return itr->k;
}

skiplist_iterator skiplist_itr_first(skiplist S) {
    if (S->min->more[0] == S->max)
        return NULL;
    else
        return S->min->more[0];
}

skiplist_iterator skiplist_itr_last(skiplist S) {
    if (S->max->less == S->min)
        return NULL;
    else
        return S->max->less;
}

skiplist_iterator skiplist_itr_next(skiplist S, skiplist_iterator itr) {
    if (itr->more[0] == S->max)
        return NULL;
    else
        return itr->more[0];
}

skiplist_iterator skiplist_itr_prev(skiplist S, skiplist_iterator itr) {
    if (itr->less == S->min)
        return NULL;
    else
        return itr->less;
}

