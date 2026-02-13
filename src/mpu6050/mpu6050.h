#pragma once
#include <stdbool.h>
bool mpu6050_init(void);
bool mpu6050_read(float *x, float *y, float *z);
