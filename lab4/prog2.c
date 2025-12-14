#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <string.h>
#include "contract.h"


static float (*cos_derivative_ptr)(float, float) = NULL;
static char* (*convert_ptr)(int) = NULL;

static void *deriv_lib = NULL;
static void *conv_lib = NULL;

const char *deriv_libs[2] = {"./libderiv1.so", "./libderiv2.so"};
const char *conv_libs[2] = {"./libconv1.so", "./libconv2.so"};

int load_version(int version) {
    if (deriv_lib) dlclose(deriv_lib);
    if (conv_lib) dlclose(conv_lib);

    deriv_lib = dlopen(deriv_libs[version], RTLD_LAZY);
    if (!deriv_lib) {
        fprintf(stderr, "Ошибка загрузки %s: %s\n", deriv_libs[version], dlerror());
        return -1;
    }

    conv_lib = dlopen(conv_libs[version], RTLD_LAZY);
    if (!conv_lib) {
        fprintf(stderr, "Ошибка загрузки %s: %s\n", conv_libs[version], dlerror());
        dlclose(deriv_lib);
        deriv_lib = NULL;
        return -1;
    }

    cos_derivative_ptr = (float (*)(float, float)) dlsym(deriv_lib, "cos_derivative");
    convert_ptr = (char* (*)(int)) dlsym(conv_lib, "convert");

    char *err = dlerror();
    if (err) {
        fprintf(stderr, "Ошибка dlsym: %s\n", err);
        dlclose(deriv_lib); dlclose(conv_lib);
        deriv_lib = conv_lib = NULL;
        return -1;
    }

    printf("Переключено на реализацию №%d\n", version + 1);
    printf("   cos_derivative: %s\n", version == 0 ?
        "(f(a+dx)-f(a))/dx" : "(f(a+dx)-f(a-dx))/(2dx)");
    printf("   convert: %s\n", version == 0 ? "двоичная" : "троичная");
    return 0;
}

int main() {
    printf("Программа №2\n");
    printf("Команды:\n");
    printf("  0            — переключить реализацию (1 | 2)\n");
    printf("  1 a dx       — производная cos(x) в точке a с шагом dx\n");
    printf("  2 x          — перевод числа x в другую систему\n");
    printf("  exit         — выход\n");

    int current_version = 0; 
    if (load_version(current_version) != 0) {
        return 1;
    }

    char line[256];
    while (fgets(line, sizeof(line), stdin)) {
        line[strcspn(line, "\n")] = '\0';

        if (strcmp(line, "exit") == 0) {
            break;
        }

        if (strlen(line) == 0) continue;

        int cmd;
        if (sscanf(line, "%d", &cmd) != 1) {
            printf("Ошибка: введите команду (0, 1, 2, exit)\n");
            continue;
        }

        if (cmd == 0) {
            current_version = 1 - current_version;
            if (load_version(current_version) != 0) {
                printf("Не удалось переключиться на реализацию %d\n", current_version + 1);
            }
        } else if (cmd == 1) {
            if (!cos_derivative_ptr) {
                printf("Ошибка: cos_derivative не загружена\n");
                continue;
            }

            float a, dx;
            if (sscanf(line, "%*d %f %f", &a, &dx) == 2) {
                float res = cos_derivative_ptr(a, dx);
                printf("cos'(%g) при dx=%g = %.8f (реализация №%d)\n", a, dx, res, current_version + 1);
            } else {
                printf("Ошибка: требуется '1 a dx'\n");
            }
        } else if (cmd == 2) {
            if (!convert_ptr) {
                printf("Ошибка: convert не загружена\n");
                continue;
            }

            int x;
            if (sscanf(line, "%*d %d", &x) == 1) {
                char *s = convert_ptr(x);
                if (s) {
                    printf("convert(%d) = %s (реализация №%d)\n", x, s, current_version + 1);
                    free(s);
                } else {
                    printf("convert(%d) = (NULL)\n", x);
                }
            } else {
                printf("Ошибка: требуется '2 x'\n");
            }
        } else {
            printf("Неизвестная команда. Используйте 0, 1, 2 или exit.\n");
        }
    }

    if (deriv_lib) dlclose(deriv_lib);
    if (conv_lib) dlclose(conv_lib);

    return 0;
}