# Potentiometer

Added support to control system volume using a potentiometer.

## Operation

Keycode `POTENTIOMETER_KEY` will synchronise the keyboard's volume state with the host computer's. 

Once synchronised, the potentiometer will adjust system volume to match the dial.

## Reading the Potentiometer Value
Since the ADC on the ESP32 boards are quite noisy, we cut-off small changes, and we use exponential smoothing to prevent the volume from jittering when there is no user input.

The cut-off is a simple check whether the difference between the current reading and the previous reading is less than a certain value:
```
if (delta > -cutOff && delta < cutOff) return;
```

The exponential smoothing function uses the simple function:
```
sₜ = sₜ₋₁+α(xₜ-sₜ₋₁)
```
or in our C++ code:
```
smoothedStatistic = smoothedStatistic + smoothingFactor * (thisSample - smoothedStatistic);
```

## Value Range

The ADC on the board I'm using (ESP32) returns a 12-bit result. i.e. the values range from 0 to 4096. We then need to map these to 0 to 100 for our volume range. 

Bear in mind that due to the nature of ADC cirucits, the values returned seldom range the full gamut from 0 to 4096, even if the dial is physically at either extreme.

## Keyboard Volume versus Host Volume
Since the keyboard has no knowledge of what the current system host volume is, we calibrate our state by sending the `VOL_DN` keycode 50x times, which regardless of what the starting system volume is, will reduce it down to 0% (this assumes it changes volume by 2% as it does on Windows machines).

Now that we know the host volume, we can compare it on every tick with the target volume, i.e. what the current value of the potentiometer is set to, and send `VOL_UP` and `VOL_DN` keycodes appropriately.

## References

- [Analog to Digital Converter (ADC)](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/adc.html) (ESP-IDF Programming Guide)
- [EspressIf Code Sample](https://github.com/espressif/esp-idf/tree/a82e6e63d9/examples/peripherals/adc/single_read) (GitHub)
- [Exponential Smoothing](https://en.wikipedia.org/wiki/Exponential_smoothing) (Wikipedia)