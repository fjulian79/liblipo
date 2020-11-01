/*
 * lipo, a class used to monitor lipo cell voltages measured by using a simple 
 * voltage divider and adc channel per cell.
 * 
 * Copyright (C) 2020 Julian Friedrich
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>. 
 *
 * You can file issues at https://github.com/fjulian79/liblipo/issues
 */

#ifndef LIPO_HPP_
#define LIPO_HPP_

#include <stdint.h>

#if __has_include ("lipo_config.h")
#include "lipo_config.h"
#endif

#ifndef LIPO_ADCCHANNEL_0
/**
 * @brief The The ADC Channel cell 1 is connected to
 */
#define LIPO_ADCCHANNEL_0               0
#endif

#ifndef LIPO_ADCCHANNELS
/**
 * @brief The number of used ADC channels 
 */
#define LIPO_ADCCHANNELS                6
#endif

#ifndef LIPO_DEFAULT_GATETIME
/**
 * @brief The default gate time.
 * 
 * The gate time defines the amount of time the cell values are accumulated to 
 * reduce jitter. jitter is a issue on higher cell number where the bad signal
 * to noise ratio has a negative impact. The bad signal to noise ratio is 
 * caused by the significantly down scaled cell voltage. Hence at cell 6 more 
 * then 22V have to be scaled to max ADC voltage (most likely 3V3).
 */
#define LIPO_DEFAULT_GATETIME           250
#endif

#ifndef LIPO_VCELL_MIN
/**
 * @brief The minimum voltage in mV which has to be measured to consider the
 * reading as valid cell voltage.
 */
#define LIPO_VCELL_MIN                  250
#endif

#ifndef LIPO_VREFINT
/**
 * @brief The ADC's reference voltage.
 */
#define LIPO_VREFINT                    1200
#endif

#ifndef LIPO_DENOMINATOR
/**
 * @brief The denominator used to scale the raw value to mV.
 * 
 * Se also BatteryParams_t and CellScale.
 */
#define LIPO_DENOMINATOR                11
#endif

/**
 * @brief Definition of the LIPO parameter structure.
 */
typedef struct
{
    /**
     * @brief The scale factors to mV for each cell.
     * 
     * These values are defined by the voltage divider resistor values and the
     * LIPO_DENUMERATOR value. They shall be calculated by: 
     * 
     * CellScale = (R1+R2) * (2^LIPO_DENUMERATOR) / R2
     * 
     *                ADC
     *                 |
     * Cell x >-- R1 --+-- R2 --| GND
     * 
     * Hence that is not needed to include any conversion from ADC raw values to
     * millivolt into this scale factor! This is done by using the STM macro
     * __LL_ADC_CALC_DATA_TO_VOLTAGE(...)
     */
    uint32_t CellScale[LIPO_ADCCHANNELS];

} BatteryParams_t;

class LiPo
{
    public:

        /**
         * @brief Construct a new LiPo object
         * 
         * @param _pParam Pointer to the parameters.
         */
        LiPo(BatteryParams_t *_pParam);

        /**
         * @brief Destroys the LiPo object.
         */
        ~LiPo();

        /**
         * @brief Set the GateTime in milliseconds.
         * 
         * See description of the LIPO_DEFAULT_GATETIME value.
         * 
         * @param millis new gate time in ms.
         */
        void setGateTime(uint16_t millis);

        /**
         * @brief the lipo class periodic task function.
         * 
         * Every time this function gets called new ADC values are sampled, 
         * accumulated and scaled to useful data if the defined gate time has
         * elapsed.
         * 
         * @param millis The current wall clock in ms.
         * 
         * @return true     If new data is available.
         * @return false    If no new data is available.
         */
        bool task(uint32_t millis);

        /**
         * @brief Get the volatge of a particular cell in mV
         * 
         * @param cell the cell number to read.
         * @param abs  Optional parameter which defaults to false. 
         *             If true, the absolute value will be returned. 
         *             If false, the voltage relative value will be returned.
         * 
         * @return uint32_t the requested voltage in mV.
         */
        uint32_t getCell(uint8_t cell, bool abs = false);

        /**
         * @brief Get the number of currently detected cells.
         * 
         * @return int8_t If positive or zero, the number of cells.
         *                -1 in case of a error. Known errors are a bad cell or 
         *                bad contact of the balancer connector.
         */
        int8_t getNumCells(void);

        /**
         * @brief Get the minimal relative cell voltage of the currently 
         * connected pack in mV
         * 
         * @return uint32_t The voltage of the weakest cell in mV.
         */
        uint32_t getMinCell(void);

        /**
         * @brief Get the number of data samples accumulated for the last data 
         * update.
         * 
         * @return uint16_t Number of samples.
         */
        uint16_t getSamples(void);

        /**
         * @brief Get the ADC Vref voltage if supported.
         * 
         * @return uint32_t 
         */
        uint32_t getVref(void);

        /**
         * @brief Calibrates the scale value of the given cell based on the 
         * provided voltage.
         * 
         * This function will sample the ADC channel used for the given cell to
         * calculate the scale factor.
         * 
         * @param cell 
         * @param voltage 
         * @return uint32_t 
         */
        uint32_t calibrate(uint8_t cell, uint32_t voltage);

    private:

        /**
         * @brief Updates the internal Vref value.
         */
        void updateVref(void);

        /**
         * @brief Transfer's the data from the Accu to VCell array.
         */
        void update(void);

        /**
         * @brief The Parameters to use.
         */
        BatteryParams_t *const pParam;

        /**
         * @brief The denominator used in combination with the scale values.
         */
        const uint8_t CellScaleDen;

        /**
         * @brief The ADC channel where cell 0 is connected to. 
         * 
         * It is mandatory to connect all other cells to the following ADC 
         * channels. AdcChannel_n = AdcChStart + Cell_n
         */
        uint8_t AdcChStart;

        /**
         * @brief The number of cells supported by the hardware.
         */
        uint8_t AdcChCnt;

        /**
         * @brief The configured gate time.
         */
        uint16_t GateTime;

        /**
         * @brief The time stamp of the last VCall update.
         */
        uint32_t LastTick;

        /**
         * @brief The Sample counter related to the Accu array.
         */
        uint32_t SampleCnt;

        /**
         * @brief The number Samples used to measure the current VCell values.
         */
        uint32_t Samples;

        /**
         * @brief The internal ADC VRef voltage.
         */
        uint32_t VRefAdc;

        /**
         * @brief The accumulator array
         */
        uint32_t Accu[LIPO_ADCCHANNELS];

        /**
         * @brief The cell voltage data array.
         */
        uint32_t VCell[LIPO_ADCCHANNELS];
};

#endif /* LIPO_HPP_ */