/*
 * Author: Brendan Le Foll <brendan.le.foll@intel.com>
 * Copyright (c) 2014 Intel Corporation.
 *
 * Code based on LSM303DLH sample by Jim Lindblom SparkFun Electronics
 * and the CompensatedCompass.ino by Frankie Chu from SeedStudio
 *
 * Further modifications to port to the LSM303d by <bruce.j.beare@intel.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <iostream>
#include <string>
#include <stdexcept>
#include <unistd.h>
#include <stdlib.h>

#include "lsm303d.h"

using namespace upm;

LSM303d::LSM303d(int bus, int addr, int accScale) : m_i2c(bus)
{
    m_addr = addr;

    uint8_t afs_bits = 0;       // LM303D_SCALE_2G - see the data sheet
    const uint8_t abw_bits = 3; // 50hz Anti-alias filter bandwidth - see the data sheet

    // 0x27 is the 'normal' mode with X/Y/Z enable
    // Data is available at 100HZ.
    // See the data sheet for higher data rates.
    setRegisterSafe(m_addr, CTRL_REG1, 0x67);

    // scale can be 2, 4 or 8
    switch (accScale) {
    case LM303D_SCALE_2G:
      afs_bits = 0;
      break;
    case LM303D_SCALE_4G:
      afs_bits = 1;
      break;
    case LM303D_SCALE_6G:
      afs_bits = 2;
      break;
    case LM303D_SCALE_8G:
      afs_bits = 3;
      break;
    case LM303D_SCALE_16G:
      afs_bits = 4;
      break;
    default:
      throw std::invalid_argument(std::string(__FUNCTION__) +
                                    ": failed to specify scaling correctly");
      break;
    }
    setRegisterSafe(m_addr, CTRL_REG2, (abw_bits<<6)|(afs_bits<<3));

    // Enable Mag.
    const uint8_t mag_resolution_bits = 3 << 5; // high resolution
    const uint8_t mag_data_rate_bits = 4 << 2;  // 50 hz
    const uint8_t mag_sensor_mode = 0;          // continuous conversion

    setRegisterSafe(m_addr, CTRL_REG5, mag_resolution_bits|mag_data_rate_bits);
    setRegisterSafe(m_addr, CTRL_REG7, mag_sensor_mode);
}

float
LSM303d::getHeading()
{
    if (getCoordinates() != mraa::SUCCESS) {
        return -1.0;
    }

    float heading = 180.0 * atan2(double(coor[Y]), double(coor[X]))/M_PI;

    if (heading < 0.0)
        heading += 360.0;

    return heading;
}

int16_t*
LSM303d::getRawAccelData()
{
    return &accel[0];
}

int16_t*
LSM303d::getRawCoorData()
{
    return &coor[0];
}

int16_t
LSM303d::getAccelX()
{
  return accel[X];
}

int16_t
LSM303d::getAccelY()
{
  return accel[Y];
}

int16_t
LSM303d::getAccelZ()
{
  return accel[Z];
}

mraa::Result
LSM303d::getCoordinates()
{
    mraa::Result ret = mraa::SUCCESS;
    uint8_t status = writeThenRead(STATUS_M);

    coor[X] = (int16_t(writeThenRead(OUT_X_H_M)) << 8)
             |  int16_t(writeThenRead(OUT_X_L_M));
    coor[Y] = (int16_t(writeThenRead(OUT_Y_H_M)) << 8)
             |  int16_t(writeThenRead(OUT_Y_L_M));
    coor[Z] = (int16_t(writeThenRead(OUT_Z_H_M)) << 8)
             |  int16_t(writeThenRead(OUT_Z_L_M));
    //printf("status=0x%x, X=%d, Y=%d, Z=%d\n", status, coor[X], coor[Y], coor[Z]);

    return ret;
}


int16_t
LSM303d::getCoorX() {
  return coor[X];
}

int16_t
LSM303d::getCoorY() {
  return coor[Y];
}

int16_t
LSM303d::getCoorZ() {
  return coor[Z];
}

// helper function that writes a value to the acc and then reads
int
LSM303d::writeThenRead(uint8_t reg)
{
    m_i2c.address(m_addr);
    m_i2c.writeByte(reg);
    m_i2c.address(m_addr);
    return (int) m_i2c.readByte();
}

mraa::Result
LSM303d::getAcceleration()
{
    mraa::Result ret = mraa::SUCCESS;

    accel[X] = (int16_t(writeThenRead(OUT_X_H_A)) << 8)
             |  int16_t(writeThenRead(OUT_X_L_A));
    accel[Y] = (int16_t(writeThenRead(OUT_Y_H_A)) << 8)
             |  int16_t(writeThenRead(OUT_Y_L_A));
    accel[Z] = (int16_t(writeThenRead(OUT_Z_H_A)) << 8)
             |  int16_t(writeThenRead(OUT_Z_L_A));
    //printf("X=%x, Y=%x, Z=%x\n", accel[X], accel[Y], accel[Z]);

    return ret;
}

// helper function that sets a register and then checks the set was succesful
mraa::Result
LSM303d::setRegisterSafe(uint8_t slave, uint8_t sregister, uint8_t data)
{
    buf[0] = sregister;
    buf[1] = data;

    if (m_i2c.address(slave) != mraa::SUCCESS) {
        throw std::invalid_argument(std::string(__FUNCTION__) +
                                    ": mraa_i2c_address() failed");
        return mraa::ERROR_INVALID_HANDLE;
    }
    if (m_i2c.write(buf, 2) != mraa::SUCCESS) {
        throw std::invalid_argument(std::string(__FUNCTION__) +
                                    ": mraa_i2c_write() failed");
        return mraa::ERROR_INVALID_HANDLE;
    }
    uint8_t val = m_i2c.readReg(sregister);
    if (val != data) {
        throw std::invalid_argument(std::string(__FUNCTION__) +
                                    ": failed to set register correctly");
        return mraa::ERROR_UNSPECIFIED;
    }
    return mraa::SUCCESS;
}
