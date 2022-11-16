#include <stdbool.h>
#include <math.h>
#include "v3math.h"

void v3_from_points(float *dst, float *a, float *b)
{
    dst[0] = b[0] - a[0];
    dst[1] = b[1] - a[1];
    dst[2] = b[2] - a[2];
}

void v3_add(float *dst, float *a, float *b)
{
    dst[0] = a[0] + b[0];
    dst[1] = a[1] + b[1];
    dst[2] = a[2] + b[2];
}

void v3_subtract(float *dst, float *a, float *b)
{
    dst[0] = a[0] - b[0];
    dst[1] = a[1] - b[1];
    dst[2] = a[2] - b[2];
}

float v3_dot_product(float *a, float *b)
{
    return (a[0] * b[0]) + (a[1] * b[1]) + (a[2] * b[2]);
}

void v3_cross_product(float *dst, float *a, float *b)
{
    dst[0] = (a[1] * b[2]) - (a[2] * b[1]);
    dst[1] = (a[2] * b[0]) - (a[0] * b[2]);
    dst[2] = (a[0] * b[1]) - (a[1] * b[0]);
}

void v3_scale(float *dst, float s)
{
    dst[0] = dst[0] * s;
    dst[1] = dst[1] * s;
    dst[2] = dst[2] * s;
}

float v3_angle(float *a, float *b)
{
    return (float)acos(v3_angle_quick(a,b));
} 

float v3_angle_quick(float *a, float *b)
{
    float numerator,denominator,result;

    numerator = v3_dot_product(a,b);

    denominator = (v3_length(a))*(v3_length(b));

    result = (numerator/denominator);

    return result;

}

void v3_reflect(float *dst, float *v, float *n)
{
    // n is functioning as 2b after following line
    v3_scale(n, (-2 * (v3_dot_product(n, v))));

    v3_add(dst, v, n);
}

float v3_length(float *a)
{
    double result;

    result = (a[0]*a[0]) + (a[1]*a[1]) + (a[2]*a[2]);
    result = sqrt(result);

    return (float)result;
}

void v3_normalize(float *dst, float *a)
{
    // denominator cant be zero
    float denominator;

    denominator = v3_length(a);

    dst[0] = a[0] / denominator;
    dst[1] = a[1] / denominator;
    dst[2] = a[2] / denominator;
}

void v3_copy(float *dst, float *a) {
    dst[0] = a[0];
    dst[1] = a[1];
    dst[2] = a[2];
}

bool v3_equals(float *a, float *b, float tolerance)
{
    return ((float)(fabs((double)(a[0] - b[0]))) < tolerance &&
        (float)(fabs((double)(a[1] - b[1]))) < tolerance &&
        (float)(fabs((double)(a[2] - b[2]))) < tolerance);
}