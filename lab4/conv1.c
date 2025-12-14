#include <stdlib.h>
#include <string.h>
#include "contract.h"

char* convert(int x) {
    if (x == 0) {
        char* res = malloc(2);
        strcpy(res, "0");
        return res;
    }

    int neg = (x < 0);
    unsigned int v = neg ? -(unsigned int)x : (unsigned int)x;

    char buf[34];
    int i = sizeof(buf) - 1;
    buf[i] = '\0';

    while (v) {
        buf[--i] = '0' + (v & 1);
        v >>= 1;
    }
    if (neg) buf[--i] = '-';

    char* res = malloc(sizeof(buf) - i);
    strcpy(res, buf + i);
    return res;
}