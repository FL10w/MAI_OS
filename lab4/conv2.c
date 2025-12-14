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
    int v = neg ? -x : x;

    char buf[25];
    int i = sizeof(buf) - 1;
    buf[i] = '\0';

    while (v) {
        int r = v % 3;
        buf[--i] = '0' + r;
        v /= 3;
    }
    if (neg) buf[--i] = '-';

    char* res = malloc(sizeof(buf) - i);
    strcpy(res, buf + i);
    return res;
}