/*
 * Copyright (C) 2014 Satoshi Noguchi
 * Copyright (C) 2014 Synaptics Inc
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <alloca.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <math.h>

#include "testutil.h"
#include "f54test.h"
#include "rmidevice.h"
#include "display.h"

/* Most recent device status event */
#define RMI_F01_STATUS_CODE(status)		((status) & 0x0f)
/* Indicates that flash programming is enabled (bootloader mode). */
#define RMI_F01_STATUS_BOOTLOADER(status)	(!!((status) & 0x40))

/*
 * Sleep mode controls power management on the device and affects all
 * functions of the device.
 */
#define RMI_F01_CTRL0_SLEEP_MODE_MASK	0x03

#define RMI_SLEEP_MODE_NORMAL		0x00
#define RMI_SLEEP_MODE_SENSOR_SLEEP	0x01
#define RMI_SLEEP_MODE_RESERVED0	0x02
#define RMI_SLEEP_MODE_RESERVED1	0x03

/*
 * This bit disables whatever sleep mode may be selected by the sleep_mode
 * field and forces the device to run at full power without sleeping.
 */
#define RMI_F01_CRTL0_NOSLEEP_BIT	(1 << 2)

F54Test::~F54Test()
{
	if (m_txAssignment != NULL) delete [] m_txAssignment;
	if (m_rxAssignment != NULL) delete [] m_rxAssignment;
}

int F54Test::Prepare(f54_report_types reportType)
{
	int retval;
	unsigned char data;

	retval = FindTestFunctions();
	if (retval != TEST_SUCCESS)
		return retval;

	retval = m_device.QueryBasicProperties();
	if (retval < 0)
		return TEST_FAIL_QUERY_BASIC_PROPERTIES;

	retval = ReadF54Queries();
	if (retval != TEST_SUCCESS)
		return retval;

	retval = SetupF54Controls();
	if (retval != TEST_SUCCESS)
		return retval;

	retval = ReadF55Queries();
	if (retval != TEST_SUCCESS)
		return retval;

	retval = SetF54ReportType(reportType);
	if (retval != TEST_SUCCESS)
		return retval;

	retval = SetF54Interrupt();
	if (retval != TEST_SUCCESS)
		return retval;

	data = (unsigned char)m_reportType;
	retval = m_device.Write(m_f54.GetDataBase(), &data, 1);
	if (retval < 0)
		return retval;

	return TEST_SUCCESS;
}

int F54Test::Run()
{
	int retval;
	unsigned char command;

	command = (unsigned char)COMMAND_GET_REPORT;
	retval = DoF54Command(command);
	if (retval != TEST_SUCCESS)
		return retval;

	retval = ReadF54Report();
	if (retval != TEST_SUCCESS)
		return retval;

	retval = ShowF54Report();
	if (retval != TEST_SUCCESS)
		return retval;

	return TEST_SUCCESS;
}

int F54Test::SetF54ReportType(f54_report_types report_type)
{
	switch (report_type) {
	case F54_8BIT_IMAGE:
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_HIGH_RESISTANCE:
	case F54_TX_TO_TX_SHORTS:
	case F54_RX_TO_RX_SHORTS_1:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP_MIN_MAX:
	case F54_RX_OPENS_1:
	case F54_TX_OPENS:
	case F54_TX_TO_GND_SHORTS:
	case F54_RX_TO_RX_SHORTS_2:
	case F54_RX_OPENS_2:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_NO_RX_COUPLING:
	case F54_SENSOR_SPEED:
	case F54_ADC_RANGE:
	case F54_TRX_OPENS:
	case F54_TRX_TO_GND_SHORTS:
	case F54_TRX_SHORTS:
	case F54_ABS_RAW_CAP:
	case F54_ABS_DELTA_CAP:
		m_reportType = report_type;
		return SetF54ReportSize(report_type);
	default:
		m_reportType = INVALID_REPORT_TYPE;
		m_reportSize = 0;
		return TEST_FAIL_INVALID_PARAMETER;
	}
}

int F54Test::SetF54ReportSize(f54_report_types report_type)
{
	int retval;
	unsigned char tx = m_txAssigned;
	unsigned char rx = m_rxAssigned;

	switch (report_type) {
	case F54_8BIT_IMAGE:
		m_reportSize = tx * rx;
		break;
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_NO_RX_COUPLING:
	case F54_SENSOR_SPEED:
		m_reportSize = 2 * tx * rx;
		break;
	case F54_HIGH_RESISTANCE:
		m_reportSize = HIGH_RESISTANCE_DATA_SIZE;
		break;
	case F54_TX_TO_TX_SHORTS:
	case F54_TX_OPENS:
	case F54_TX_TO_GND_SHORTS:
		m_reportSize = (tx + 7) / 8;
		break;
	case F54_RX_TO_RX_SHORTS_1:
	case F54_RX_OPENS_1:
		if (rx < tx)
			m_reportSize = 2 * rx * rx;
		else
			m_reportSize = 2 * tx * rx;
		break;
	case F54_FULL_RAW_CAP_MIN_MAX:
		m_reportSize = FULL_RAW_CAP_MIN_MAX_DATA_SIZE;
		break;
	case F54_RX_TO_RX_SHORTS_2:
	case F54_RX_OPENS_2:
		if (rx <= tx)
			m_reportSize = 0;
		else
			m_reportSize = 2 * rx * (rx - tx);
		break;
	case F54_ADC_RANGE:
		if (m_f54Query.has_signal_clarity) {

			retval = m_device.Read(m_f54Control.reg_41.address,
					m_f54Control.reg_41.data,
					sizeof(m_f54Control.reg_41.data));
			if (retval < 0) {
				m_reportSize = 0;
				break;
			}
			if (m_f54Control.reg_41.no_signal_clarity) {
				if (tx % 4)
					tx += 4 - (tx % 4);
			}
		}
		m_reportSize = 2 * tx * rx;
		break;
	case F54_TRX_OPENS:
	case F54_TRX_TO_GND_SHORTS:
	case F54_TRX_SHORTS:
		m_reportSize = TRX_OPEN_SHORT_DATA_SIZE;
		break;
	case F54_ABS_RAW_CAP:
	case F54_ABS_DELTA_CAP:
		m_reportSize = 4 * (tx + rx);
		break;
	default:
		m_reportSize = 0;
		return TEST_FAIL_INVALID_PARAMETER;
	}

	return TEST_SUCCESS;
}

int F54Test::FindTestFunctions()
{
	if (0 > m_device.ScanPDT(0x00, 10))
		return TEST_FAIL_SCAN_PDT;

	if (!m_device.GetFunction(m_f01, 0x01))
		return TEST_FAIL_NO_FUNCTION_01;

	if (!m_device.GetFunction(m_f54, 0x54))
		return TEST_FAIL_NO_FUNCTION_54;

	if (!m_device.GetFunction(m_f55, 0x55))
		return TEST_FAIL_NO_FUNCTION_55;

	return TEST_SUCCESS;
}

int F54Test::ReadF54Queries()
{
	int retval;
	unsigned short query_addr = m_f54.GetQueryBase();
	unsigned char offset;

	retval = m_device.Read(query_addr,
			       m_f54Query.data,
			       sizeof(m_f54Query.data));
	if (retval < 0)
		return retval;

	offset = sizeof(m_f54Query.data);

	/* query 12 */
	if (m_f54Query.has_sense_frequency_control == 0)
		offset -= 1;

	/* query 13 */
	if (m_f54Query.has_query13) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_13.data,
				sizeof(m_f54Query_13.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 14 */
	if ((m_f54Query.has_query13) && (m_f54Query_13.has_ctrl87))
		offset += 1;

	/* query 15 */
	if (m_f54Query.has_query15) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_15.data,
				sizeof(m_f54Query_15.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 16 */
	if ((m_f54Query.has_query15) && (m_f54Query_15.has_query16)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_16.data,
				sizeof(m_f54Query_16.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 17 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query16) &&
			(m_f54Query_16.has_query17))
		offset += 1;

	/* query 18 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query16) &&
			(m_f54Query_16.has_ctrl94_query18))
		offset += 1;

	/* query 19 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query16) &&
			(m_f54Query_16.has_ctrl95_query19))
		offset += 1;

	/* query 20 */
	if ((m_f54Query.has_query15) && (m_f54Query_15.has_query20))
		offset += 1;

	/* query 21 */
	if ((m_f54Query.has_query15) && (m_f54Query_15.has_query21)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_21.data,
				sizeof(m_f54Query_21.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 22 */
	if ((m_f54Query.has_query15) && (m_f54Query_15.has_query22)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_22.data,
				sizeof(m_f54Query_22.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 23 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query22) &&
			(m_f54Query_22.has_query23)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_23.data,
				sizeof(m_f54Query_23.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 24 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query21) &&
			(m_f54Query_21.has_query24_data18))
		offset += 1;

	/* query 25 */
	if ((m_f54Query.has_query15) && (m_f54Query_15.has_query25)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_25.data,
				sizeof(m_f54Query_25.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 26 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query22) &&
			(m_f54Query_22.has_ctrl103_query26))
		offset += 1;

	/* query 27 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_27.data,
				sizeof(m_f54Query_27.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 28 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query22) &&
			(m_f54Query_22.has_query28))
		offset += 1;

	/* query 29 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_29.data,
				sizeof(m_f54Query_29.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 30 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_30.data,
				sizeof(m_f54Query_30.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 31 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_ctrl122_query31))
		offset += 1;

	/* query 32 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_32.data,
				sizeof(m_f54Query_32.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 33 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_33.data,
				sizeof(m_f54Query_33.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 34 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query34))
		offset += 1;

	/* query 35 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query35)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_35.data,
				sizeof(m_f54Query_35.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 36 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_36.data,
				sizeof(m_f54Query_36.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	/* query 37 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36) &&
			(m_f54Query_36.has_query37))
		offset += 1;

	/* query 38 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36) &&
			(m_f54Query_36.has_query38)) {
		retval = m_device.Read(query_addr + offset,
				m_f54Query_38.data,
				sizeof(m_f54Query_38.data));
		if (retval < 0)
			return retval;
		offset += 1;
	}

	return TEST_SUCCESS;;
}

int F54Test::SetupF54Controls()
{
	unsigned char length;
	unsigned char num_of_sensing_freqs;
	unsigned short reg_addr = m_f54.GetControlBase();

	num_of_sensing_freqs = m_f54Query.number_of_sensing_frequencies;

	/* control 0 */
	reg_addr += CONTROL_0_SIZE;

	/* control 1 */
	if ((m_f54Query.touch_controller_family == 0) ||
			(m_f54Query.touch_controller_family == 1))
		reg_addr += CONTROL_1_SIZE;

	/* control 2 */
	reg_addr += CONTROL_2_SIZE;

	/* control 3 */
	if (m_f54Query.has_pixel_touch_threshold_adjustment == 1)
		reg_addr += CONTROL_3_SIZE;

	/* controls 4 5 6 */
	if ((m_f54Query.touch_controller_family == 0) ||
			(m_f54Query.touch_controller_family == 1))
		reg_addr += CONTROL_4_6_SIZE;

	/* control 7 */
	if (m_f54Query.touch_controller_family == 1) {
		m_f54Control.reg_7.address = reg_addr;
		reg_addr += CONTROL_7_SIZE;
	}

	/* controls 8 9 */
	if ((m_f54Query.touch_controller_family == 0) ||
			(m_f54Query.touch_controller_family == 1))
		reg_addr += CONTROL_8_9_SIZE;

	/* control 10 */
	if (m_f54Query.has_interference_metric == 1)
		reg_addr += CONTROL_10_SIZE;

	/* control 11 */
	if (m_f54Query.has_ctrl11 == 1)
		reg_addr += CONTROL_11_SIZE;

	/* controls 12 13 */
	if (m_f54Query.has_relaxation_control == 1)
		reg_addr += CONTROL_12_13_SIZE;

	/* controls 14 15 16 */
	if (m_f54Query.has_sensor_assignment == 1) {
		reg_addr += CONTROL_14_SIZE;
		reg_addr += CONTROL_15_SIZE * m_f54Query.num_of_rx_electrodes;
		reg_addr += CONTROL_16_SIZE * m_f54Query.num_of_tx_electrodes;
	}

	/* controls 17 18 19 */
	if (m_f54Query.has_sense_frequency_control == 1) {
		reg_addr += CONTROL_17_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_18_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_19_SIZE * num_of_sensing_freqs;
	}

	/* control 20 */
	reg_addr += CONTROL_20_SIZE;

	/* control 21 */
	if (m_f54Query.has_sense_frequency_control == 1)
		reg_addr += CONTROL_21_SIZE;

	/* controls 22 23 24 25 26 */
	if (m_f54Query.has_firmware_noise_mitigation == 1)
		reg_addr += CONTROL_22_26_SIZE;

	/* control 27 */
	if (m_f54Query.has_iir_filter == 1)
		reg_addr += CONTROL_27_SIZE;

	/* control 28 */
	if (m_f54Query.has_firmware_noise_mitigation == 1)
		reg_addr += CONTROL_28_SIZE;

	/* control 29 */
	if (m_f54Query.has_cmn_removal == 1)
		reg_addr += CONTROL_29_SIZE;

	/* control 30 */
	if (m_f54Query.has_cmn_maximum == 1)
		reg_addr += CONTROL_30_SIZE;

	/* control 31 */
	if (m_f54Query.has_touch_hysteresis == 1)
		reg_addr += CONTROL_31_SIZE;

	/* controls 32 33 34 35 */
	if (m_f54Query.has_edge_compensation == 1)
		reg_addr += CONTROL_32_35_SIZE;

	/* control 36 */
	if ((m_f54Query.curve_compensation_mode == 1) ||
			(m_f54Query.curve_compensation_mode == 2)) {
		if (m_f54Query.curve_compensation_mode == 1) {
			length = std::max(m_f54Query.num_of_rx_electrodes,
					m_f54Query.num_of_tx_electrodes);
		} else if (m_f54Query.curve_compensation_mode == 2) {
			length = m_f54Query.num_of_rx_electrodes;
		}
		reg_addr += CONTROL_36_SIZE * length;
	}

	/* control 37 */
	if (m_f54Query.curve_compensation_mode == 2)
		reg_addr += CONTROL_37_SIZE * m_f54Query.num_of_tx_electrodes;

	/* controls 38 39 40 */
	if (m_f54Query.has_per_frequency_noise_control == 1) {
		reg_addr += CONTROL_38_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_39_SIZE * num_of_sensing_freqs;
		reg_addr += CONTROL_40_SIZE * num_of_sensing_freqs;
	}

	/* control 41 */
	if (m_f54Query.has_signal_clarity == 1) {
		m_f54Control.reg_41.address = reg_addr;
		reg_addr += CONTROL_41_SIZE;
	}

	/* control 42 */
	if (m_f54Query.has_variance_metric == 1)
		reg_addr += CONTROL_42_SIZE;

	/* controls 43 44 45 46 47 48 49 50 51 52 53 54 */
	if (m_f54Query.has_multi_metric_state_machine == 1)
		reg_addr += CONTROL_43_54_SIZE;

	/* controls 55 56 */
	if (m_f54Query.has_0d_relaxation_control == 1)
		reg_addr += CONTROL_55_56_SIZE;

	/* control 57 */
	if (m_f54Query.has_0d_acquisition_control == 1) {
		m_f54Control.reg_57.address = reg_addr;
		reg_addr += CONTROL_57_SIZE;
	}

	/* control 58 */
	if (m_f54Query.has_0d_acquisition_control == 1)
		reg_addr += CONTROL_58_SIZE;

	/* control 59 */
	if (m_f54Query.has_h_blank == 1)
		reg_addr += CONTROL_59_SIZE;

	/* controls 60 61 62 */
	if ((m_f54Query.has_h_blank == 1) ||
			(m_f54Query.has_v_blank == 1) ||
			(m_f54Query.has_long_h_blank == 1))
		reg_addr += CONTROL_60_62_SIZE;

	/* control 63 */
	if ((m_f54Query.has_h_blank == 1) ||
			(m_f54Query.has_v_blank == 1) ||
			(m_f54Query.has_long_h_blank == 1) ||
			(m_f54Query.has_slew_metric == 1) ||
			(m_f54Query.has_slew_option == 1) ||
			(m_f54Query.has_noise_mitigation2 == 1))
		reg_addr += CONTROL_63_SIZE;

	/* controls 64 65 66 67 */
	if (m_f54Query.has_h_blank == 1)
		reg_addr += CONTROL_64_67_SIZE * 7;
	else if ((m_f54Query.has_v_blank == 1) ||
			(m_f54Query.has_long_h_blank == 1))
		reg_addr += CONTROL_64_67_SIZE;

	/* controls 68 69 70 71 72 73 */
	if ((m_f54Query.has_h_blank == 1) ||
			(m_f54Query.has_v_blank == 1) ||
			(m_f54Query.has_long_h_blank == 1))
		reg_addr += CONTROL_68_73_SIZE;

	/* control 74 */
	if (m_f54Query.has_slew_metric == 1)
		reg_addr += CONTROL_74_SIZE;

	/* control 75 */
	if (m_f54Query.has_enhanced_stretch == 1)
		reg_addr += CONTROL_75_SIZE * num_of_sensing_freqs;

	/* control 76 */
	if (m_f54Query.has_startup_fast_relaxation == 1)
		reg_addr += CONTROL_76_SIZE;

	/* controls 77 78 */
	if (m_f54Query.has_esd_control == 1)
		reg_addr += CONTROL_77_78_SIZE;

	/* controls 79 80 81 82 83 */
	if (m_f54Query.has_noise_mitigation2 == 1)
		reg_addr += CONTROL_79_83_SIZE;

	/* controls 84 85 */
	if (m_f54Query.has_energy_ratio_relaxation == 1)
		reg_addr += CONTROL_84_85_SIZE;

	/* control 86 */
	if ((m_f54Query.has_query13 == 1) && (m_f54Query_13.has_ctrl86 == 1))
		reg_addr += CONTROL_86_SIZE;

	/* control 87 */
	if ((m_f54Query.has_query13 == 1) && (m_f54Query_13.has_ctrl87 == 1))
		reg_addr += CONTROL_87_SIZE;

	/* control 88 */
	if (m_f54Query.has_ctrl88 == 1) {
		m_f54Control.reg_88.address = reg_addr;
		reg_addr += CONTROL_88_SIZE;
	}

	/* control 89 */
	if ((m_f54Query.has_query13 == 1) &&
			(m_f54Query_13.has_cidim == 1 ||
			m_f54Query_13.has_noise_mitigation_enhancement ||
			m_f54Query_13.has_rail_im))
		reg_addr += CONTROL_89_SIZE;

	/* control 90 */
	if ((m_f54Query.has_query15) && (m_f54Query_15.has_ctrl90))
		reg_addr += CONTROL_90_SIZE;

	/* control 91 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query21) &&
			(m_f54Query_21.has_ctrl91))
		reg_addr += CONTROL_91_SIZE;

	/* control 92 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query16) &&
			(m_f54Query_16.has_ctrl92))
		reg_addr += CONTROL_92_SIZE;

	/* control 93 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query16) &&
			(m_f54Query_16.has_ctrl93))
		reg_addr += CONTROL_93_SIZE;

	/* control 94 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query16) &&
			(m_f54Query_16.has_ctrl94_query18))
		reg_addr += CONTROL_94_SIZE;

	/* control 95 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query16) &&
			(m_f54Query_16.has_ctrl95_query19))
		reg_addr += CONTROL_95_SIZE;

	/* control 96 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query21) &&
			(m_f54Query_21.has_ctrl96))
		reg_addr += CONTROL_96_SIZE;

	/* control 97 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query21) &&
			(m_f54Query_21.has_ctrl97))
		reg_addr += CONTROL_97_SIZE;

	/* control 98 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query21) &&
			(m_f54Query_21.has_ctrl98))
		reg_addr += CONTROL_98_SIZE;

	/* control 99 */
	if (m_f54Query.touch_controller_family == 2)
		reg_addr += CONTROL_99_SIZE;

	/* control 100 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query16) &&
			(m_f54Query_16.has_ctrl100))
		reg_addr += CONTROL_100_SIZE;

	/* control 101 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query22) &&
			(m_f54Query_22.has_ctrl101))
		reg_addr += CONTROL_101_SIZE;


	/* control 102 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query22) &&
			(m_f54Query_22.has_query23) &&
			(m_f54Query_23.has_ctrl102))
		reg_addr += CONTROL_102_SIZE;

	/* control 103 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query22) &&
			(m_f54Query_22.has_ctrl103_query26))
		reg_addr += CONTROL_103_SIZE;

	/* control 104 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query22) &&
			(m_f54Query_22.has_ctrl104))
		reg_addr += CONTROL_104_SIZE;

	/* control 105 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query22) &&
			(m_f54Query_22.has_ctrl105))
		reg_addr += CONTROL_105_SIZE;

	/* control 106 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_ctrl106))
		reg_addr += CONTROL_106_SIZE;

	/* control 107 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_ctrl107))
		reg_addr += CONTROL_107_SIZE;

	/* control 108 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_ctrl108))
		reg_addr += CONTROL_108_SIZE;

	/* control 109 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_ctrl109))
		reg_addr += CONTROL_109_SIZE;

	/* control 110 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_ctrl110)) {
		m_f54Control.reg_110.address = reg_addr;
		reg_addr += CONTROL_110_SIZE;
	}

	/* control 111 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_ctrl111))
		reg_addr += CONTROL_111_SIZE;

	/* control 112 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_ctrl112))
		reg_addr += CONTROL_112_SIZE;

	/* control 113 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_ctrl113))
		reg_addr += CONTROL_113_SIZE;

	/* control 114 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_ctrl114))
		reg_addr += CONTROL_114_SIZE;

	/* control 115 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_ctrl115))
		reg_addr += CONTROL_115_SIZE;

	/* control 116 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_ctrl116))
		reg_addr += CONTROL_116_SIZE;

	/* control 117 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_ctrl117))
		reg_addr += CONTROL_117_SIZE;

	/* control 118 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_ctrl118))
		reg_addr += CONTROL_118_SIZE;

	/* control 119 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_ctrl119))
		reg_addr += CONTROL_119_SIZE;

	/* control 120 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_ctrl120))
		reg_addr += CONTROL_120_SIZE;

	/* control 121 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_ctrl121))
		reg_addr += CONTROL_121_SIZE;

	/* control 122 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_ctrl122_query31))
		reg_addr += CONTROL_122_SIZE;

	/* control 123 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_ctrl123))
		reg_addr += CONTROL_123_SIZE;

	/* control 124 reserved */

	/* control 125 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_ctrl125))
		reg_addr += CONTROL_125_SIZE;

	/* control 126 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_ctrl126))
		reg_addr += CONTROL_126_SIZE;

	/* control 127 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_ctrl127))
		reg_addr += CONTROL_127_SIZE;

	/* controls 128 129 130 131 reserved */

	/* control 132 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_ctrl132))
		reg_addr += CONTROL_132_SIZE;

	/* control 133 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_ctrl133))
		reg_addr += CONTROL_133_SIZE;

	/* control 134 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_ctrl134))
		reg_addr += CONTROL_134_SIZE;

	/* controls 135 136 reserved */

	/* control 137 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query35) &&
			(m_f54Query_35.has_ctrl137))
		reg_addr += CONTROL_137_SIZE;

	/* control 138 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query35) &&
			(m_f54Query_35.has_ctrl138))
		reg_addr += CONTROL_138_SIZE;

	/* control 139 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query35) &&
			(m_f54Query_35.has_ctrl139))
		reg_addr += CONTROL_139_SIZE;

	/* control 140 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query35) &&
			(m_f54Query_35.has_ctrl140))
		reg_addr += CONTROL_140_SIZE;

	/* control 141 reserved */

	/* control 142 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36) &&
			(m_f54Query_36.has_ctrl142))
		reg_addr += CONTROL_142_SIZE;

	/* control 143 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36) &&
			(m_f54Query_36.has_ctrl143))
		reg_addr += CONTROL_143_SIZE;

	/* control 144 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36) &&
			(m_f54Query_36.has_ctrl144))
		reg_addr += CONTROL_144_SIZE;

	/* control 145 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36) &&
			(m_f54Query_36.has_ctrl145))
		reg_addr += CONTROL_145_SIZE;

	/* control 146 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36) &&
			(m_f54Query_36.has_ctrl146))
		reg_addr += CONTROL_146_SIZE;

	/* control 147 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36) &&
			(m_f54Query_36.has_query38) &&
			(m_f54Query_38.has_ctrl147))
		reg_addr += CONTROL_147_SIZE;

	/* control 148 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36) &&
			(m_f54Query_36.has_query38) &&
			(m_f54Query_38.has_ctrl148))
		reg_addr += CONTROL_148_SIZE;

	/* control 149 */
	if ((m_f54Query.has_query15) &&
			(m_f54Query_15.has_query25) &&
			(m_f54Query_25.has_query27) &&
			(m_f54Query_27.has_query29) &&
			(m_f54Query_29.has_query30) &&
			(m_f54Query_30.has_query32) &&
			(m_f54Query_32.has_query33) &&
			(m_f54Query_33.has_query36) &&
			(m_f54Query_36.has_query38) &&
			(m_f54Query_38.has_ctrl149)) {
		m_f54Control.reg_149.address = reg_addr;
		reg_addr += CONTROL_149_SIZE;
	}

	return TEST_SUCCESS;
}

int F54Test::ReadF55Queries()
{
	int retval;
	unsigned char ii;
	unsigned char rx_electrodes = m_f54Query.num_of_rx_electrodes;
	unsigned char tx_electrodes = m_f54Query.num_of_tx_electrodes;

	retval = m_device.Read(m_f55.GetQueryBase(),
			m_f55Query.data,
			sizeof(m_f55Query.data));
	if (retval < 0) {
		return retval;
	}

	if (!m_f55Query.has_sensor_assignment)
	{
		m_txAssigned = tx_electrodes;
		m_rxAssigned = rx_electrodes;
		m_txAssignment = NULL;
		m_rxAssignment = NULL;
		return TEST_SUCCESS;
	}

	if (m_txAssignment != NULL) delete [] m_txAssignment;
	if (m_rxAssignment != NULL) delete [] m_rxAssignment;
	m_txAssignment = new unsigned char[tx_electrodes];
	m_rxAssignment = new unsigned char[rx_electrodes];

	retval = m_device.Read(m_f55.GetControlBase() + SENSOR_TX_MAPPING_OFFSET,
			m_txAssignment,
			tx_electrodes);
	if (retval < 0) {
		goto exit;
	}

	retval = m_device.Read(m_f55.GetControlBase() + SENSOR_RX_MAPPING_OFFSET,
			m_rxAssignment,
			rx_electrodes);
	if (retval < 0) {
		goto exit;
	}

	m_txAssigned = 0;
	for (ii = 0; ii < tx_electrodes; ii++) {
		if (m_txAssignment[ii] != 0xff)
			m_txAssigned++;
	}

	m_rxAssigned = 0;
	for (ii = 0; ii < rx_electrodes; ii++) {
		if (m_rxAssignment[ii] != 0xff)
			m_rxAssigned++;
	}

	return TEST_SUCCESS;

exit:
	if (m_txAssignment != NULL)
	{
		delete [] m_txAssignment;
		m_txAssignment = NULL;
	}
	if (m_rxAssignment != NULL)
	{
		delete [] m_rxAssignment;
		m_rxAssignment = NULL;
	}

	return retval;
}

int F54Test::SetF54Interrupt()
{
	int retval;
	unsigned char mask = m_f54.GetInterruptMask();
	unsigned char zero = 0;
	unsigned int i;

	for (i = 0; i < m_device.GetNumInterruptRegs(); i++)
	{
		if (i == m_f54.GetInterruptRegNum())
		{
			retval = m_device.Write(m_f54.GetControlBase() + 1 + i, &mask, 1);
		}
		else
		{
			retval = m_device.Write(m_f54.GetControlBase() + 1 + i, &zero, 1);
		}

		if (retval < 0)
			return retval;
	}
	return TEST_SUCCESS;
}

int F54Test::DoF54Command(unsigned char command)
{
	int retval;

	retval = m_device.Write(m_f54.GetCommandBase(), &command, 1);
	if (retval < 0)
		return retval;

	retval = WaitForF54CommandCompletion();
	if (retval != TEST_SUCCESS)
		return retval;

	return TEST_SUCCESS;
}

int F54Test::WaitForF54CommandCompletion()
{
	int retval;
	unsigned char value;
	unsigned char timeout_count;

	timeout_count = 0;
	do {
		retval = m_device.Read(m_f54.GetCommandBase(),
				&value,
				sizeof(value));
		if (retval < 0)
			return retval;

		if (value == 0x00)
			break;

		Sleep(100);
		timeout_count++;
	} while (timeout_count < COMMAND_TIMEOUT_100MS);

	if (timeout_count == COMMAND_TIMEOUT_100MS) {
		return -ETIMEDOUT;
	}

	return TEST_SUCCESS;
}

int F54Test::ReadF54Report()
{
	int retval;
	unsigned char report_index[2];

	if (m_reportBufferSize < m_reportSize) {
		if (m_reportData != NULL)
			delete [] m_reportData;
		m_reportData = new unsigned char[m_reportSize];
		if (!m_reportData) {
			m_reportBufferSize = 0;
			retval = TEST_FAIL_MEMORY_ALLOCATION;
			goto exit;
		}
		m_reportBufferSize = m_reportSize;
	}

	report_index[0] = 0;
	report_index[1] = 0;

	retval = m_device.Write(m_f54.GetDataBase() + REPORT_INDEX_OFFSET,
				report_index,
				sizeof(report_index));

	if (retval < 0)
		goto exit;

	retval = m_device.Read(m_f54.GetDataBase() + REPORT_DATA_OFFSET,
				m_reportData,
				m_reportSize);
	if (retval < 0)
		goto exit;

	return TEST_SUCCESS;

exit:
	if (m_reportData != NULL)
	{
		delete [] m_reportData;
		m_reportData = NULL;
	}

	return retval;
}

int F54Test::ShowF54Report()
{
	unsigned int ii;
	unsigned int jj;
	unsigned int tx_num = m_txAssigned;
	unsigned int rx_num = m_rxAssigned;
	char *report_data_8;
	short *report_data_16;
	int *report_data_32;
	unsigned int *report_data_u32;
	char buf[256];

	switch (m_reportType) {
	case F54_8BIT_IMAGE:
		report_data_8 = (char *)m_reportData;
		for (ii = 0; ii < m_reportSize; ii++) {
			sprintf(buf, "%03d: %d\n",
					ii, *report_data_8);
			m_display.Output(buf);
			report_data_8++;
		}
		break;
	case F54_16BIT_IMAGE:
	case F54_RAW_16BIT_IMAGE:
	case F54_TRUE_BASELINE:
	case F54_FULL_RAW_CAP:
	case F54_FULL_RAW_CAP_NO_RX_COUPLING:
	case F54_SENSOR_SPEED:
		report_data_16 = (short *)m_reportData;
		sprintf(buf, "tx = %d\nrx = %d\n",
				tx_num, rx_num);
		m_display.Output(buf);

		for (ii = 0; ii < tx_num; ii++) {
			for (jj = 0; jj < (rx_num - 1); jj++) {
				sprintf(buf, "%-4d ",
						*report_data_16);
				report_data_16++;
				m_display.Output(buf);
			}
			sprintf(buf, "%-4d\n",
					*report_data_16);
			m_display.Output(buf);
			report_data_16++;
		}
		break;
	case F54_HIGH_RESISTANCE:
	case F54_FULL_RAW_CAP_MIN_MAX:
		report_data_16 = (short *)m_reportData;
		for (ii = 0; ii < m_reportSize; ii += 2) {
			sprintf(buf, "%03d: %d\n",
					ii / 2, *report_data_16);
			m_display.Output(buf);
			report_data_16++;
		}
		break;
	case F54_ABS_RAW_CAP:
		report_data_u32 = (unsigned int *)m_reportData;
		sprintf(buf, "rx ");
		m_display.Output(buf);

		for (ii = 0; ii < rx_num; ii++) {
			sprintf(buf, "     %2d", ii);
			m_display.Output(buf);
		}
		sprintf(buf, "\n");
		m_display.Output(buf);

		sprintf(buf, "   ");
		m_display.Output(buf);

		for (ii = 0; ii < rx_num; ii++) {
			sprintf(buf, "  %5u",
					*report_data_u32);
			report_data_u32++;
			m_display.Output(buf);
		}
		sprintf(buf, "\n");
		m_display.Output(buf);

		sprintf(buf, "tx ");
		m_display.Output(buf);

		for (ii = 0; ii < tx_num; ii++) {
			sprintf(buf, "     %2d", ii);
			m_display.Output(buf);
		}
		sprintf(buf, "\n");
		m_display.Output(buf);
		
		sprintf(buf, "   ");
		m_display.Output(buf);
		
		for (ii = 0; ii < tx_num; ii++) {
			sprintf(buf, "  %5u",
					*report_data_u32);
			report_data_u32++;
			m_display.Output(buf);
		}
		sprintf(buf, "\n");
		m_display.Output(buf);
		
		break;
	case F54_ABS_DELTA_CAP:
		report_data_32 = (int *)m_reportData;
		sprintf(buf, "rx ");
		m_display.Output(buf);
		
		for (ii = 0; ii < rx_num; ii++) {
			sprintf(buf, "     %2d", ii);
			m_display.Output(buf);
		}
		sprintf(buf, "\n");
		m_display.Output(buf);
		
		sprintf(buf, "   ");
		m_display.Output(buf);
		
		for (ii = 0; ii < rx_num; ii++) {
			sprintf(buf, "  %5d",
					*report_data_32);
			report_data_32++;
			m_display.Output(buf);
		}
		sprintf(buf, "\n");
		m_display.Output(buf);

		sprintf(buf, "tx ");
		m_display.Output(buf);
		
		for (ii = 0; ii < tx_num; ii++) {
			sprintf(buf, "     %2d", ii);
			m_display.Output(buf);
		}
		sprintf(buf, "\n");
		m_display.Output(buf);
		
		sprintf(buf, "   ");
		m_display.Output(buf);
		
		for (ii = 0; ii < tx_num; ii++) {
			sprintf(buf, "  %5d",
					*report_data_32);
			report_data_32++;
			m_display.Output(buf);
		}
		sprintf(buf, "\n");
		m_display.Output(buf);
		
		break;
	default:
		for (ii = 0; ii < m_reportSize; ii++) {
			sprintf(buf, "%03d: 0x%02x\n",
					ii, m_reportData[ii]);
			m_display.Output(buf);
		}
		break;
	}

	sprintf(buf, "\n");
	m_display.Output(buf);

	m_display.Reflesh();

	return TEST_SUCCESS;
}
