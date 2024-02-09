## 3.3V <-> 12V SeeLevel Interface

To interface, I built a level-shifter using this forum post as a guide:

https://forums.raspberrypi.com/viewtopic.php?t=119614

This circuit works fairly well at voltages > 12.5V with a 200Ω resistor, >12.0V with a 100Ω. Generates read errors at lower voltages.

Based on available components, and to make sure that the read pin sees no more than 3.3v.

The voltage divider resistors are chosen to provide a reasonably fast falloff when 12V side is driven to zero by the pulse from the ESP32 and roughly 3.3V at the ESP pin. With 3.3kΩ/1kΩ resistors, the sensor is pulled to zero in roughly 15µs.

Tested with 100ohm resistor, three sensors, one of which was wired with 20ft. of 20ga. wire.

![My Image](SeeLevelSensorCircuit.png)
