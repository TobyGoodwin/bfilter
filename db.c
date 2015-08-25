/*
 * db.c:
 * Database for bfilter.
 *
 * Copyright (c) 2004 Chris Lightfoot. All rights reserved.
 * Email: chris@ex-parrot.com; WWW: http://www.ex-parrot.com/~chris/
 *
 */

static const char rcsid[] = "$Id: db.c,v 1.2 2004/04/11 17:32:10 chris Exp $";

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

#include "util.h"

static TDB_CONTEXT *filterdb;

/* HASHLEN
 * Length, in bytes, of the hash of a term we use as a key in the database. We
 * use the first HASHLEN bytes of the MD5 checksum of the term. */
#define HASHLEN 8

/* make_hash TERM LENGTH HASH
 * Save in HASH the hash of the LENGTH-byte TERM. */
static void make_hash(const char *term, const size_t len, unsigned char hash[8]) {
    unsigned char md5[16];
    MD5(term, len, md5);
    memcpy(hash, md5, HASHLEN);
}

/* db_open
 * Open the filter database, which lives in ~/.bfildb. Returns 1 on success
 * or 0 on failure. Because we need to save timestamps, the database is opened
 * read/write in all cases. */
int db_open(void) {
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
        name = malloc(strlen(home) + 10);
        sprintf(name, "%s/.bfildb2", home);
    }

    /* Wait at most about a minute for the database. */
    if (!(filterdb = tdb_open(name, 0, 0, O_CREAT | O_RDWR, 0666)))
        fprintf(stderr, "bfilter: %s: %s\n", name, strerror(errno));

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
void db_set_pair(const unsigned char *name, unsigned int a, unsigned int b) {
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
int db_get_pair(const unsigned char *name, unsigned int *a, unsigned int *b) {
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

/* db_clean DAYS
 * Delete from the database elements which have not been used in the past
 * number of DAYS. Returns the number of discarded terms. */
unsigned int db_clean(int ndays) {
    TDB_DATA k;
    time_t t0;
    unsigned int n = 0, kept = 0, lost = 0;
    
    if (ndays == -1)
        ndays = 28;

    time(&t0);
    t0 -= ndays * 24 * 3600;

    /* Iterate over keys. */
    for (k = tdb_firstkey(filterdb); k.dptr; ) {
        TDB_DATA knext, d;
        knext = tdb_nextkey(filterdb, k);
        d = tdb_fetch(filterdb, k);

        if (d.dsize == 8) {
            /* No time stored; assume that this is older than the threshold. */
            tdb_delete(filterdb, k);
            ++lost;
        } else {
            uint32_t u;
            time_t t;
            memcpy(&u, d.dptr + 8, 4);
            t = (time_t)ntohl(u);

            if (t < t0) {
                tdb_delete(filterdb, k);
                ++lost;
            } else
                ++kept;
        }

        xfree(k.dptr);
        xfree(d.dptr);

        if ((++n) % 100 == 0 && isatty(1))
            fprintf(stderr, "Cleaning: kept %8u / discarded %8u\r", kept, lost);

        k = knext;
    }
    
    if (isatty(1))
        fprintf(stderr, "Cleaning: kept %8u / discarded %8u\n", kept, lost);
    
    return lost;
}
