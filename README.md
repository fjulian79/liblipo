# liblipo
A class used to monitor LiPo cell voltages measured by using a simple voltage divider and ADC channel per cell.

## Optional configuration file
Instead of changing constants in lipo.hpp you should add a file called lipo_config.h and set the constants to the needed values. Here is a example on how this file might look like:

```C
#ifndef LIPO_CONFIG_H_
#define LIPO_CONFIG_H_

/**
 * @brief The The ADC Channel cell 1 is connected to
 */
#define LIPO_ADCCHANNEL_0                       3

/**
 * @brief The number of cells supported by the hardware.
 */
#define LIPO_ADCCHANNELS                        3

/**
 * @brief The scale factors to mV for each cell.
 * 
 * See lipo.hpp for mor infos on how this values are calculated.
 */
#define BATTERY_PARAMS                          \
{                                               \
    .CellScale = {2669, 5803, 8806}             \
} 

#endif /* LIPO_CONFIG_H_ */
```

### ADC Channels
The library assumes that the ADC channels are assigned to the LiPo cells in a monotonously increasing order starting at a given ADC channel. Two symbols are used to define this relation, see LIPO_ADCCHANNEL_0 and LIPO_ADCCHANNELS. In the example above the cells are connected in the following way:

|Cell|ADC Channel|
|:---:|:---:|
|0|3|
|1|4|
|2|5|

### CellScale calculation
The library needs infos on the voltage divider resistor values used in the hardware. This scale factor is implemented by a fraction. The denominator is common for all cells and defined by LIPO_DENUMERATOR. The numerator is calculated by the following formula:

![equation](https://latex.codecogs.com/svg.latex?CellScale%20=%20\frac{(R1+R2)*2^{LIPO\_DENOMINATOR}}{R2})

Hence that this scale factor is only based on the resistor values and the denominator. It is not needed to care about converting ADC raw values to millivolts.

## Example Code
This is a small example which prints the voltage of the weakest cell along with
the number of cells and the captured samples. It uses all the default values, so
you might have to adopt the configuration for your particular setup.

```C
#include <Arduino.h>
#include <stm32yyxx_ll_adc.h>
#include <lipo/lipo.hpp>
#include <stdint.h>

BatteryParams_t params = {3128, 5609, 8771, 11460, 13718, 16984};
LiPo lipo(&params);

void setup()
{
    analogReadResolution(12);

    Serial.begin(115200);
    while (!Serial); 
}

void loop()
{
    static uint32_t lastTick = 0;
    uint32_t tick = millis();

    lipo.task(tick);

    if (tick - lastTick >= 250)
    {
        lastTick = tick;

        uint32_t mV = lipo.getMinCell();
        uint16_t samples = lipo.getSamples();
        uint8_t cells = lipo.getNumCells();

        Serial.printf("%lu,%luV (%u/%u)\n", mV/1000, mV%1000), cells, samples;
    }
}
```