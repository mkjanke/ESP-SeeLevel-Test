## Basic 3.3V <-> 12V level shifter

To interface, I built a level-shifter using this document as a guide:

  https://www.analog.com/en/design-notes/how-to-level-shift-1wire-systems.html

The 'Basic Level Shifter' was built with the right-hand side powered by a 12V power supply, an ESP32 as the 3.3V side, and a 2N7000 N-channel MOSFET. Resisters were chosen by guessing. I ended up with a 51kohm pullup resister on the 3.3V side and a 560ohm resister on the 12V side, which limits current to approx 25mA. I am not an EE.

![My Image](SeeLevelSensorCircuit.png)

Operation is described in the document linked below.

https://www.analog.com/en/design-notes/how-to-level-shift-1wire-systems.html

### Description of the Level-Shifting Operation

> To understand how the logic shifter works for any open-drain bus line, there are three main states to consider during a bi-directional transmission.

#### State 1 – No device pulls down the line to a low level

> With nothing pulling down on the bus line, the lower voltage side gets pulled up to 3.3V by its pullup resistor. In this event, the MOSFET's VGS is below the threshold voltage because the gate and the source are both at 3.3V, so the MOSFET does not conduct. This allows the bus line at the higher voltage side to get pulled up to 5V (12V) by its pullup resistor. As a result, both sides are high but at different voltage levels. 

[Note: Should be no current on ESP32 Pin]

#### State 2 – 3.3V system pulls down the open-drain bus to a low level

> With the lower voltage side pulling down on the bus line, the MOSFET's source gets pulled down low and leaves the gate high. In this event, the MOSFET's VGS is above the threshold voltage and begins to conduct, also pulling the higher voltage side down. As a result, both sides are pulled low. 

[Note: ESP32 pin will sink 14.5V/560ohms = 26mA from 12V side, plus 3.3v/51kohm from 3.3v side.]

### State 3 – 5V (12V) system pulls down the open-drain bus to a low level

> With the higher voltage side pulling down on the bus line, the MOSFET's drain sits low. In this event, the MOSFET's drain-substrate diode begins to conduct, indirectly pulling the source low. When this happens, the MOSFET begins to conduct like in State 2 and, as a result, pulls down both sides of the bus line low. 

[Note: Current draw 14.5v/560ohm = 26mA. (plus 3.3v/51kohm)]

### Sources:

https://forums.raspberrypi.com/viewtopic.php?t=119614

Note: Author Jim G. uses two pins (R & W), 5V arduino, P-channel MOSFET. 
