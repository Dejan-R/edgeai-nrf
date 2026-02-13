#include "mpu6050.h"
#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>

#define MPU_ADDR 0x68

#define REG_PWR_MGMT_1   0x6B
#define REG_WHO_AM_I     0x75
#define REG_CONFIG       0x1A
#define REG_ACCEL_CFG    0x1C
#define REG_ACCEL_XOUT_H 0x3B

static const struct device *i2c_dev;

static bool write_reg(uint8_t reg, uint8_t val)
{
    uint8_t data[2] = { reg, val };
    return i2c_write(i2c_dev, data, 2, MPU_ADDR) == 0;
}

static bool read_reg(uint8_t reg, uint8_t *val)
{
    return i2c_write_read(i2c_dev, MPU_ADDR, &reg, 1, val, 1) == 0;
}
bool mpu6050_init(void)
{
    i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c1));
    if (!device_is_ready(i2c_dev))
        return false;
    // WHO_AM_I sensor check (HW validation)
    uint8_t who = 0;
    if (!read_reg(REG_WHO_AM_I, &who) || who != 0x68)
        return false;
    //Wake up
    if (!write_reg(REG_PWR_MGMT_1, 0x00))
        return false;
    // Digital low pass filter (44 Hz)
    if (!write_reg(REG_CONFIG, 0x03))
        return false;
    // ±2g (max resolution)
    if (!write_reg(REG_ACCEL_CFG, 0x00))
        return false;
    return true;
}

bool mpu6050_read(float *x, float *y, float *z)
{
    if (!i2c_dev || !x || !y || !z)
        return false;
    uint8_t reg = REG_ACCEL_XOUT_H;
    uint8_t raw[6];
    if (i2c_write_read(i2c_dev, MPU_ADDR, &reg, 1, raw, 6) != 0)
        return false;
    int16_t xi = (raw[0] << 8) | raw[1];
    int16_t yi = (raw[2] << 8) | raw[3];
    int16_t zi = (raw[4] << 8) | raw[5];
    const float scale = 16384.0f;
    *x = xi / scale;
    *y = yi / scale;
    *z = zi / scale;
    return true;
}