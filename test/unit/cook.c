/*
depends test/ucook
tap
*/

#include <stdio.h>

int main(void) {
    char buf[10];

    printf("1..1\n");
    _test_utf8_encode(buf, 'x');
    if (buf[0] != 0x78) printf("not ");
    print ("ok 1 utf8 encode 1 byte");
}
