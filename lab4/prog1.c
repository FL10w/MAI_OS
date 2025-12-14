#include <stdio.h>
#include <stdlib.h>
#include "contract.h"
#include <string.h>

int main() {
    printf("Программа №1\n");
    printf("Команды:\n  0 — информация\n  1 a dx — производная\n  2 x — перевод в систему\n  exit — выход\n");

    char line[128];
    while (fgets(line, sizeof(line), stdin)) {
        if (strncmp(line, "exit", 4) == 0) break;

        int cmd;
        if (sscanf(line, "%d", &cmd) != 1) continue;

        if (cmd == 0) {
            printf("Используются реализации:\n");
            printf("  cos_derivative: (f(a+dx)-f(a))/dx\n");
            printf("  convert: двоичная система\n");
        } else if (cmd == 1) {
            float a, dx;
            if (sscanf(line, "%*d %f %f", &a, &dx) == 2) {
                float r = cos_derivative(a, dx);
                printf("cos'(%g) при dx=%g = %.8f\n", a, dx, r);
            } else {
                printf("Ошибка: требуется '1 a dx'\n");
            }
        } else if (cmd == 2) {
            int x;
            if (sscanf(line, "%*d %d", &x) == 1) {
                char* s = convert(x);
                printf("convert(%d) = %s\n", x, s);
                free(s);
            } else {
                printf("Ошибка: требуется '2 x'\n");
            }
        } else {
            printf("Неизвестная команда\n");
        }
    }
    return 0;
}