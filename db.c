/*
 * db.c:
 * Database for bfilter.
 *
 * Copyright (c) 2004 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: db.c,v 1.7 2005/06/07 16:42:04 chris Exp $";

#include <sys/types.h>

#include <errno.h>
#include <pwd.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <tdb.h>
#include <time.h>
#include <unistd.h>

#include <openssl/md5.h>
#include <netinet/in.h>
#include <sys/fcntl.h>
#include <sys/stat.h>

#include "class.h"
#include "line.h"
#include "util.h"

static TDB_CONTEXT *filterdb;
static size_t dbsize;

/* HASHLEN
 * Length, in bytes, of the hash of a term we use as a key in the database. We
 * use the first HASHLEN bytes of the MD5 checksum of the term. */
#define HASHLEN 8

/* make_hash TERM LENGTH HASH
 * Save in HASH the hash of the LENGTH-byte TERM. */
static void make_hash(const uint8_t *term, const size_t len, uint8_t hash[8]) {
    uint8_t md5[16];
    MD5(term, len, md5);
    memcpy(hash, md5, HASHLEN);
}

static char *dbfilename(const char *suffix) {
    char *name, *home;
    if ((home = getenv("BFILTER_DB")))
        name = xstrdup(home);
    else {
        home = getenv("HOME");
        if (!home) {
            struct passwd *P;
            if (!(P = getpwuid(getuid())))
                return 0;
            home = P->pw_dir;
        }
        name = xmalloc(strlen(home) + 10);
        sprintf(name, "%s/.bfildb2", home);
    }
    if (suffix) {
        name = xrealloc(name, strlen(name) + strlen(suffix) + 1);
        strcat(name, suffix);
    }
    return name;
}

/* db_open
 * Open the filter database, which lives in ~/.bfildb. Returns 1 on success
 * or 0 on failure. Because we need to save timestamps, the database is opened
 * read/write in all cases. */
int db_open(void) {
    char *name;
    struct stat st;

    name = dbfilename(NULL);

    if (!(filterdb = tdb_open(name, 0, 0, O_CREAT | O_RDWR, 0666)))
        fprintf(stderr, "bfilter: %s: %s\n", name, strerror(errno));

    if (0 == stat(name, &st))
        dbsize = st.st_size;
    
    xfree(name);

    if (filterdb)
        return 1;
    else
        return 0;
}

/* db_close
 * Close the filter database. */
void db_close(void) {
    tdb_close(filterdb);
}

/* db_set_pair NAME A B
 * Save under NAME the pair A, B, also setting the timestamp. */
void db_set_pair(const char *name, unsigned int a, unsigned int b) {
    TDB_DATA k, d;
    uint32_t u;
    unsigned char key[HASHLEN], data[12];     /* a, b, time of last use. */

    make_hash(name, strlen(name), key);
    k.dptr = key;
    k.dsize = HASHLEN;

    u = htonl(a);
    memcpy(data, &u, 4);
    u = htonl(b);
    memcpy(data + 4, &u, 4);
    u = htonl((uint32_t)time(NULL));     /* XXX 2038 bug */
    memcpy(data + 8, &u, 4);
    d.dptr = data;
    d.dsize = 12;

    tdb_store(filterdb, k, d, TDB_REPLACE);
}

/* db_get_pair NAME A B
 * Save in *A and *B the elements of the pair identified by NAME, returning 1
 * and updating the timestamp if the pair is found, else returning 0. */
int db_get_pair(const char *name, unsigned int *a, unsigned int *b) {
    TDB_DATA k, d;
    unsigned char key[HASHLEN];
    
    make_hash(name, strlen(name), key);
    k.dptr = key;
    k.dsize = HASHLEN;
    
    d = tdb_fetch(filterdb, k);
    if (!d.dptr || (d.dsize != 8 && d.dsize != 12))
        return 0;
    else {
        uint32_t u;
        
        memcpy(&u, d.dptr, 4);
        *a = ntohl(u);
        memcpy(&u, d.dptr + 4, 4);
        *b = ntohl(u);
        xfree(d.dptr);

        /* Update record with current time. */
        db_set_pair(name, *a, *b);
        
        return 1;
    }
}

/* store an integer list x of size n under key k of size k_sz */
void db_set_intlist(uint8_t *k, size_t k_sz, uint32_t *x, unsigned int n) {
    TDB_DATA key, d;
    unsigned char h[HASHLEN];

    make_hash(k, k_sz, h);
    key.dptr = h;
    key.dsize = HASHLEN;

    d.dptr = (void *)x;
    d.dsize = 4 * n;

    tdb_store(filterdb, key, d, 0);
}

/* retrieve an integer list of size n from key k of size k_sz */
uint32_t *db_get_intlist(uint8_t *k, size_t k_sz, unsigned int *n) {
    TDB_DATA key, d;
    unsigned char h[HASHLEN];

    make_hash(k, k_sz, h);
    key.dptr = h;
    key.dsize = HASHLEN;

    d = tdb_fetch(filterdb, key);
    if (!d.dptr || d.dsize % sizeof(uint32_t) != 0)
        return 0;
    *n = d.dsize / sizeof(uint32_t);
    return (uint32_t *)d.dptr;
}

#define CLASSES_KEY ((uint8_t *)"__classes__")

struct class *db_get_classes(void) {
    TDB_DATA k, v;
    uint8_t key[HASHLEN], *p;
    struct class *cs = 0;
    int csa = 0, csn = 0;

    make_hash(CLASSES_KEY, sizeof(CLASSES_KEY) - 1, key);
    k.dptr = key;
    k.dsize = HASHLEN;

    v = tdb_fetch(filterdb, k);
    if (v.dptr) {
        /* note that the cs array we build here contains pointers into the data
         * v returned from the database; tdb specifies that the caller is
         * responsible for freeing this data, so obviously in this case we
         * don't */
        for (p = v.dptr; p < v.dptr + v.dsize; ++csn) {
            uint32_t nc;
            if (csn == csa)
                cs = xrealloc(cs, (csa = csa * 2 + 1) * sizeof *cs);
            cs[csn].name = p;
            while (*p++)
                ;
            memcpy(&nc, p, sizeof nc);
            p += sizeof nc;
            cs[csn].code = ntohl(nc);
        }
    } else
        csa = csn = 0;
        
    /* add a sentinel */
    if (csn == csa)
        cs = xrealloc(cs, (csa = csa * 2 + 1) * sizeof *cs);
    cs[csn].name = 0;
    cs[csn].code = 0;

    return cs;
}

void db_set_classes(struct class *cs) {
    TDB_DATA k, v;
    struct class *p;
    struct line csl = { 0 };
    uint8_t key[HASHLEN];

    make_hash(CLASSES_KEY, sizeof(CLASSES_KEY) - 1, key);
    k.dptr = key;
    k.dsize = HASHLEN;

    for (p = cs; p->code; ++p) {
        struct line c = { 0 };
        uint32_t nc;

        c.x = p->name;
        c.l = strlen((char *)p->name) + 1; /* including \0 */
        line_cat(&csl, &c);
        nc = htonl(p->code);
        c.x = (uint8_t *)&nc;
        c.l = 4;
        line_cat(&csl, &c);
    }
    v.dptr = csl.x; v.dsize = csl.l;
    tdb_store(filterdb, k, v, 0); /* XXX return value? */
}

struct cleanparam {
    TDB_CONTEXT *cp_db;
    time_t cp_maxage;
    unsigned int cp_nkept, cp_ndiscarded, cp_ntotal;
};

/* copy_items DB KEY VALUE P
 * Copy data from old to new database, if it is new enough. */
static int copy_items(TDB_CONTEXT *db, TDB_DATA key, TDB_DATA val, void *p) {
    static unsigned char h[HASHLEN];
    static int have_h;
    static time_t now;
    uint32_t u;
    struct cleanparam *c;
    time_t when;
    
    if (!have_h) {
        make_hash("__emails__", sizeof("__emails__") - 1, h);
        have_h = 1;
        time(&now);
    }
    
    c = (struct cleanparam*)p;

    if (key.dsize != 8 || val.dsize != 12)
        return 0;

    memcpy(&u, val.dptr + 8, 4);
    when = (time_t)ntohl(u);

    if (when > now - c->cp_maxage || 0 == memcmp(h, key.dptr, HASHLEN)) {
        tdb_store(c->cp_db, key, val, TDB_REPLACE);
        ++c->cp_nkept;
    } else
        ++c->cp_ndiscarded;

    if (isatty(2) && (0 == ((c->cp_nkept + c->cp_ndiscarded) % 1000) || c->cp_nkept + c->cp_ndiscarded == c->cp_ntotal)) {
        fprintf(stderr, "\rCleaning: kept %8u / discarded %8u; done %5.1f%%",
                c->cp_nkept, c->cp_ndiscarded, 100. * (c->cp_nkept + c->cp_ndiscarded) / (float)c->cp_ntotal);
    }

    return 0;
}

/* db_clean DAYS
 * Delete from the database elements which have not been used in the past
 * number of DAYS. Returns the number of discarded terms. */
void db_clean(int ndays) {
    struct cleanparam c = {0};
    char *newname = NULL, *name;
    
    name = dbfilename(NULL);
    srand(time(NULL));
    
    while (!c.cp_db) {
        char suffix[32];
        xfree(newname);
        sprintf(suffix, ".new.%8u.%8u.%8u", (unsigned)rand(), (unsigned)rand(), (unsigned)time(NULL));
        newname = dbfilename(suffix);
        if (!(c.cp_db = tdb_open(newname, 0, 0, O_CREAT | O_RDWR | O_EXCL, 0666)) && errno != EEXIST) {
            fprintf(stderr, "bfilter: %s: %s\n", newname, strerror(errno));
            return;
        }
    }

    c.cp_maxage = ndays * 24 * 3600;
    c.cp_ntotal = tdb_traverse(filterdb, NULL, NULL);

    tdb_traverse(filterdb, copy_items, &c);
    printf("\n");

    tdb_close(c.cp_db);
    tdb_close(filterdb);

    if (-1 == rename(newname, name)) {
        fprintf(stderr, "bfilter: replace %s: %s\n", name, strerror(errno));
    }
    
    db_open();

    xfree(newname);
    xfree(name);
    
    return;
}

#define NHISTDAYS       28
#define NPROBCLASSES    25

struct dbstats {
    unsigned int ds_nterms, ds_nbogons;
    unsigned int ds_nterms_by_age[NHISTDAYS + 1];   /* ...[NHISTDAYS] is `older terms' */
    unsigned int ds_nterms_by_prob[NPROBCLASSES];
    unsigned int ds_nspamtotal, ds_nrealtotal;      /* # emails in db */
};

/* gather_stats DB KEY VALUE P
 * Callback for gathering statistics about the database. */
static int gather_stats(TDB_CONTEXT *db, TDB_DATA key, TDB_DATA val, void *p) {
    static unsigned char h[HASHLEN];
    static int have_h;
    static time_t now;
    struct dbstats *S;
    uint32_t u;
    unsigned int nspam, nreal;
    time_t when;
    int ndays;
    float prob;
    
    S = (struct dbstats*)p;
    if (!have_h) {
        make_hash("__emails__", sizeof("__emails__") - 1, h);
        have_h = 1;
        time(&now);
    }
    
    if (key.dsize != HASHLEN) {
        ++S->ds_nbogons;
        return 0;
    } else if (val.dsize != 12) {
        ++S->ds_nbogons;
        return 0;
    }
    if (0 == memcmp(key.dptr, h, HASHLEN))
        return 0;
    
    ++S->ds_nterms;

    memcpy(&u, val.dptr, 4);
    nspam = ntohl(u);
    memcpy(&u, val.dptr + 4, 4);
    nreal = ntohl(u);
    memcpy(&u, val.dptr + 8, 4);
    when = (time_t)ntohl(u);        /* XXX 2038 bug */
    
    /* Compute age in days, update histogram. */
    ndays = (now - when) / (24 * 3600);
    if (ndays < 0)
        ++S->ds_nbogons;
    else if (ndays < NHISTDAYS)
        ++S->ds_nterms_by_age[ndays];
    else
        ++S->ds_nterms_by_age[NHISTDAYS];

    /* Compute probability. */
    prob = ((float)nspam / (float)S->ds_nspamtotal) / ((float)nspam / (float)S->ds_nspamtotal + (float)nreal / (float)S->ds_nrealtotal);
    ++S->ds_nterms_by_prob[(int)(prob * (NPROBCLASSES - 1))];

    return 0;
}

/* db_print_stats
 * Print some statistics about the database. */
void db_print_stats(void) {
    struct dbstats S = {0};
    size_t ncols = 0;
    char *c;
    int i;
    unsigned int max;
    char *bar;
    
    if ((c = getenv("COLUMNS")))
        ncols = (size_t)atoi(c);
    
    if (!ncols) ncols = 80;
    if (ncols > 1024) ncols = 1024;
    if (ncols < 40) ncols = 40;
    bar = xmalloc(1024);
    for (i = 0; i < 1024; ++i) bar[i] = '#';
    
    if (!(db_get_pair("__emails__", &S.ds_nspamtotal, &S.ds_nrealtotal))) {
        fprintf(stderr, "bfilter: database seems to be empty; add some spam and nonspam\n");
        return;
    }
    tdb_traverse(filterdb, gather_stats, (void*)&S);

    printf("\n");

    if (dbsize)
        printf("Total database size:    %8.1f MB (%5.1f%% of data length)\n", (float)dbsize / 1048576., (100. * dbsize) / (20. * S.ds_nterms + 1));
    else
        printf("Total database size:    unknown\n");
            
    
    printf("Spams in corpus:        %8u\n"
           "Real emails in corpus:  %8u\n"
           "Terms in corpus:        %8u\n"
           "Bogus database entries: %8u\n",
           S.ds_nspamtotal, S.ds_nrealtotal, S.ds_nterms, S.ds_nbogons);

    /* Draw histogram of terms by age. */
    printf("\nNumber of terms, by time since last seen:\n\n");

    for (i = 0, max = 0; i <= NHISTDAYS; ++i)
        if (S.ds_nterms_by_age[i] > max)
            max = S.ds_nterms_by_age[i];
    

    /*      xxxxxxxxxxx xxxxxxxxx   */
    printf("Age (days)  Number of terms\n"
           "----------- ---------------\n");
    printf("     <1     %8u %.*s\n", S.ds_nterms_by_age[0], (int)((ncols - 21.) * (float)S.ds_nterms_by_age[0] / (float)max), bar);
    for (i = 1; i < NHISTDAYS; ++i)
        printf("  %3d-%-3d   %8u %.*s\n", i, i + 1, S.ds_nterms_by_age[i], (int)((ncols - 21.) * (float)S.ds_nterms_by_age[i] / (float)max), bar);
    printf("   >=%3d    %8u %.*s\n", NHISTDAYS, S.ds_nterms_by_age[NHISTDAYS], (int)((ncols - 21.) * (float)S.ds_nterms_by_age[NHISTDAYS] / (float)max), bar);

    /* Draw histogram of terms by spam probability. */
    printf("\nNumber of terms, by `spam probability':\n\n");
    
    for (i = 0, max = 0; i < NPROBCLASSES; ++i)
        if (S.ds_nterms_by_prob[i] > max)
            max = S.ds_nterms_by_prob[i];

    printf("Probability Number of terms\n"
           "----------- ---------------\n");
    for (i = 0; i < NPROBCLASSES; ++i) {
        float p;
        p = (i + 0.5) / (float)NPROBCLASSES;
        printf("   %.2f     %8u %.*s\n", p, S.ds_nterms_by_prob[i], (int)((ncols - 21.) * (float)S.ds_nterms_by_prob[i] / (float)max), bar);
    }

    printf("\n");
}
