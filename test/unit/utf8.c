/*
depends test/uutf8
tap
*/

#include <stdio.h>
#include <string.h>

#include "utf8.h"

#define BUF 10

int main(void) {
    unsigned char buf[BUF];

    printf("1..12\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0) != 1 ||
            buf[0] != 0 || buf[1] != 0x55) printf("not ");
    printf("ok 1 utf8 encode 1 byte min\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 'x') != 1 ||
            buf[0] != 0x78 || buf[1] != 0x55) printf("not ");
    printf("ok 2 utf8 encode 1 byte\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0x7f) != 1 ||
            buf[0] != 0x7f || buf[1] != 0x55) printf("not ");
    printf("ok 3 utf8 encode 1 byte max\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0x80) != 2 ||
            buf[0] != 0xc2 || buf[1] != 0x80 || buf[2] != 0x55) printf("not ");
    printf("ok 4 utf8 encode 2 bytes min\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0x61f) != 2 ||
            buf[0] != 0xd8 || buf[1] != 0x9f || buf[2] != 0x55) printf("not ");
    printf("ok 5 utf8 encode 2 bytes\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0x7ff) != 2 ||
            buf[0] != 0xdf || buf[1] != 0xbf || buf[2] != 0x55) printf("not ");
    printf("ok 6 utf8 encode 2 bytes max\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0x800) != 3 ||
            buf[0] != 0xe0 || buf[1] != 0xa0 ||
            buf[2] != 0x80 || buf[3] != 0x55) printf("not ");
    printf("ok 7 utf8 encode 3 bytes min\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0x2189) != 3 ||
            buf[0] != 0xe2 || buf[1] != 0x86 ||
            buf[2] != 0x89 || buf[3] != 0x55) printf("not ");
    printf("ok 8 utf8 encode 3 bytes\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0xffff) != 3 ||
            buf[0] != 0xef || buf[1] != 0xbf ||
            buf[2] != 0xbf || buf[3] != 0x55) printf("not ");
    printf("ok 9 utf8 encode 3 bytes max\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0x10000) != 4 ||
            buf[0] != 0xf0 || buf[1] != 0x90 ||
            buf[2] != 0x80 || buf[3] != 0x80 || buf[4] != 0x55) printf("not ");
    printf("ok 10 utf8 encode 4 bytes min\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0x850bb) != 4 ||
            buf[0] != 0xf2 || buf[1] != 0x85 ||
            buf[2] != 0x82 || buf[3] != 0xbb || buf[4] != 0x55) printf("not ");
    printf("ok 11 utf8 encode 4 bytes\n");

    memset(buf, 0x55, BUF);
    if (utf8_encode(buf, 0x10ffff) != 4 ||
            buf[0] != 0xf4 || buf[1] != 0x8f ||
            buf[2] != 0xbf || buf[3] != 0xbf || buf[4] != 0x55) printf("not ");
    printf("ok 12 utf8 encode 4 bytes max\n");

}
