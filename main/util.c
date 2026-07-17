#include "util.h"

int clamp_int(int value, int min, int max)
{
    if (value < min) return min;
    if (value > max) return max;
    return value;
}

int abs_int(int a) 
{
    if (a<0) a=-a;
    return a;
}