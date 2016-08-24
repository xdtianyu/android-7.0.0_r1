/*
 * Author: Brendan Le Foll<brendan.le.foll@intel.com>
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
#pragma once

#include <string.h>
#include <mraa/i2c.hpp>
#include <math.h>

namespace upm {
/**
 * @brief LSM303d Accelerometer/Compass library
 * @defgroup lsm303d libupm-lsm303d
 * @ingroup seeed adafruit i2c accelerometer compass
 */

/**
 * @library lsm303d
 * @sensor lsm303d
 * @comname LSM303d Accelerometer & Compass
 * @altname Grove 6-Axis Accelerometer & Compass
 * @type accelerometer compass
 * @man seeed adafruit
 * @web http://www.seeedstudio.com/wiki/Grove_-_6-Axis_Accelerometer%26Compass
 * @con i2c
 *
 * @brief API for the LSM303d Accelerometer & Compass
 *
 * This module defines the LSM303d 3-axis magnetometer/3-axis accelerometer.
 * This module was tested with the Seeed Studio* Grove 6-Axis Accelerometer & Compass
 * version 2.0 module used over I2C. The magnometer and acceleromter are accessed
 * at two seperate I2C addresses.
 *
 * @image html lsm303d.jpeg
 * @snippet lsm303d.cxx Interesting
 */
class LSM303d {
  public:

    /* Address definitions for the grove 6DOF v2.0 */
    typedef enum {
        LSM303d_ADDR = 0x1E
    } GROVE_6DOF_ADDRS_T;

    typedef enum {
        LM303D_SCALE_2G  = 2,
        LM303D_SCALE_4G  = 4,
        LM303D_SCALE_6G  = 6,
        LM303D_SCALE_8G  = 8,
        LM303D_SCALE_16G = 16
    } LSM303D_SCALE_T;

    typedef enum {
        X = 0,
        Y = 1,
        Z = 2
    } XYZ_T;

    /**
    * Instantiates an LSM303d object
    *
    * @param i2c bus
    * @param addr Magnetometer
    * @param addr Accelerometer
    */
   LSM303d (int bus,
           int addr=LSM303d_ADDR,
           int accScale=LM303D_SCALE_4G);

   /**
    * LSM303d object destructor
    * where is no more need for this here - I2c connection will be stopped
    * automatically when m_i2c variable will go out of scope
    * ~LSM303d ();
    **/

   /**
    * Gets the current heading; headings <0 indicate an error has occurred
    *
    * @return float
    */
   float getHeading();

   /**
    * Gets the coordinates in the XYZ order
    */
   mraa::Result getCoordinates();

   /**
    * Gets accelerometer values
    * Should be called before other "get" functions for acceleration
    */
   mraa::Result getAcceleration();

   /**
    * Gets raw coordinate data; it is updated when getCoordinates() is called
    */
   int16_t* getRawCoorData();

   /**
    * Gets the X component of the coordinates data
    */
   int16_t getCoorX();

   /**
    * Gets the Y component of the coordinates data
    */
   int16_t getCoorY();

   /**
    * Gets the Z component of the coordinates data
    */
   int16_t getCoorZ();

   /**
    * Gets raw accelerometer data; it is updated when getAcceleration() is called
    */
   int16_t* getRawAccelData();

   /**
    * Gets the X component of the acceleration data
    */
   int16_t getAccelX();

   /**
    * Gets the Y component of the acceleration data
    */
   int16_t getAccelY();

   /**
    * Gets the Z component of the acceleration data
    */
   int16_t getAccelZ();

  private:

    /* LSM303d Register definitions */
    typedef enum {
        STATUS_M     = 0x7,
        OUT_X_L_M    = 0x8,
        OUT_X_H_M    = 0x9,
        OUT_Y_L_M    = 0xA,
        OUT_Y_H_M    = 0xB,
        OUT_Z_L_M    = 0xC,
        OUT_Z_H_M    = 0xD,

        CTRL_REG0    = 0x1f,
        CTRL_REG1    = 0x20,
        CTRL_REG2    = 0x21,
        CTRL_REG3    = 0x22,
        CTRL_REG4    = 0x23,
        CTRL_REG5    = 0x24,
        CTRL_REG6    = 0x25,
        CTRL_REG7    = 0x26,

        STATUS_REGA  = 0x27,

        OUT_X_L_A    = 0x28,
        OUT_X_H_A    = 0x29,
        OUT_Y_L_A    = 0x2A,
        OUT_Y_H_A    = 0x2B,
        OUT_Z_L_A    = 0x2C,
        OUT_Z_H_A    = 0x2D,

        FIFO_CTRL    = 0x2E,
        FIFO_SRC     = 0x2F,

        IG_CFG1      = 0x30,
        IG_SRC1      = 0x31,
        IG_THS1      = 0x32,
        IG_DUR1      = 0x33,

        IG_CFG2      = 0x34,
        IG_SRC2      = 0x35,
        IG_THS2      = 0x36,
        IG_DUR2      = 0x37,

        CLICK_CFG    = 0x38,
        CLICK_SRC    = 0x39,
        CLICK_THS    = 0x3A,

        TIME_LIMIT   = 0x3B,
        TIME_LATEN   = 0x3C,
        TIME_WINDO   = 0x3D,

        ACT_THS      = 0x3E,
        ACT_DUR      = 0x3F,
    } LSM303d_REGS_T;

   int writeThenRead(uint8_t reg);
   mraa::Result setRegisterSafe(uint8_t slave, uint8_t sregister, uint8_t data);

   mraa::I2c m_i2c;
   int m_addr;

   uint8_t buf[6];
   int16_t coor[3];
   int16_t accel[3];
};

}
