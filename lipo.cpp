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

#include <lipo/lipo.hpp>

#include <string.h>
#include <Arduino.h>
#include <stm32yyxx_ll_adc.h>

LiPo::LiPo(BatteryParams_t *_pParam) : 
      pParam(_pParam)
    , CellScaleDen(LIPO_DENOMINATOR)
    , AdcChStart(LIPO_ADCCHANNEL_0)
    , AdcChCnt(LIPO_ADCCHANNELS)
    , GateTime(LIPO_DEFAULT_GATETIME)
    , LastTick(0)
    , SampleCnt(0)
    , Samples(0)
    , VRefAdc(0)
{
    memset(Accu, 0, sizeof(Accu));
    memset(VCell, 0, sizeof(VCell));
}

LiPo::~LiPo()
{
    
}

void LiPo::setGateTime(uint16_t millis)
{
    GateTime = millis;
}

bool LiPo::task(uint32_t millis)
{
    bool newData = false;

    SampleCnt++;

    for (size_t i = 0; i < AdcChCnt; i++)
    {
        Accu[i] += analogRead(AdcChStart + i);
    }

    if (millis - LastTick >= GateTime)
    {
        LastTick = millis;
        update();
        newData = true;
    }

    return newData;
}

uint32_t LiPo::getCell(uint8_t cell, bool abs)
{
    uint32_t volt = 0;

    if (cell == 0 || abs)
    {
        volt = VCell[cell];
    }
    else if (cell >= AdcChCnt)
    {
        volt = 0;
    }
    else if (VCell[cell] >= VCell[cell - 1])
    {
        volt = VCell[cell] - VCell[cell - 1];
    }

    return volt;
}

int8_t LiPo::getNumCells(void)
{
    int8_t cells = 0;
    bool done = false;

    for (uint8_t i = 0; i < AdcChCnt; i++)
    {
        if(getCell(i) >= LIPO_VCELL_MIN)
        {
            if(!done)
            {
                cells++;
            }
            else
            {
                cells = -1;
                break;
            }
        }
        else
        {
            done = true;
        }
    }
    
    return cells;
}

uint32_t LiPo::getMinCell(void)
{
    uint32_t volt = 0;
    int8_t cells = getNumCells();

    if(cells > 0)
    {
        volt = UINT32_MAX;
        for (uint8_t i = 0; i < cells; i++)
        {
            uint32_t cell = getCell(i);
            if(cell >= LIPO_VCELL_MIN)
            {
                volt = min(volt, cell);
            }
        }
    }

    return volt;
}

uint16_t LiPo::getSamples(void)
{
    return Samples;
}

uint32_t LiPo::getVref(void)
{
    return VRefAdc;
}

uint32_t LiPo::calibrate(uint8_t cell, uint32_t voltage)
{
    updateVref();

    Accu[cell] = 0;
    SampleCnt = 0;    
    LastTick = millis();

    while (millis() - LastTick <= GateTime)
    {
        Accu[cell] += analogRead(AdcChStart + cell);
        SampleCnt++;
    }
    
    voltage <<= CellScaleDen;
    Accu[cell] /= SampleCnt;

    printf("raw: %lu, samples: %lu\n", Accu[cell], SampleCnt);
    Accu[cell] = __LL_ADC_CALC_DATA_TO_VOLTAGE(
                VRefAdc, Accu[cell], LL_ADC_RESOLUTION_12B);
    printf("Scale old: %lu\n", pParam->CellScale[cell]); 
    pParam->CellScale[cell] = voltage/Accu[cell];
    printf("Scale new: %lu\n", pParam->CellScale[cell]); 

    memset(Accu, 0, sizeof(Accu));
    SampleCnt = 0;
    LastTick = millis();

    return 0;
}

void LiPo::updateVref(void)
{
    VRefAdc = LIPO_VREFINT * __LL_ADC_DIGITAL_SCALE(LL_ADC_RESOLUTION_12B) / 
                    analogRead(AVREF);
}

void LiPo::update(void)
{
    updateVref();

    for (size_t i = 0; i < AdcChCnt; i++)
    {
        Accu[i] /= SampleCnt;
        VCell[i] = __LL_ADC_CALC_DATA_TO_VOLTAGE(
            VRefAdc, Accu[i], LL_ADC_RESOLUTION_12B);
        VCell[i] = (VCell[i] * pParam->CellScale[i]) >> CellScaleDen; 
        Accu[i] = 0;
    }

    Samples = SampleCnt;
    SampleCnt = 0;
}