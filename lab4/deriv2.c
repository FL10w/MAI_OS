#include <math.h>
#include "contract.h"

float cos_derivative(float a, float dx) {
    return (cosf(a + dx) - cosf(a - dx)) / (2.0f * dx);
}