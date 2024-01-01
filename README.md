## ESP32 Test/Demo app: Read Garnet SeeLevel Tank Sensor/Sender

Inspired by Jim G., who did all the heavy lifting: https://forums.raspberrypi.com/viewtopic.php?t=119614

To read RV tanks equipped with the Garnet SeeLevel sensors, one can purchase a display monitor panel that has any of Bluetooth, RV-C, or NEMA2000 - all of which would be a better interface than this. This is an attempt to read the sensors (senders) directly, without a Garnet display panel. 

### Summary of information discovered by Jim G. and documented in the above post:

To trigger the tank sender/sensor, one needs to power the sender with 12V then pull the 12V line to ground in a particular pattern. The sender will respond by pulling the 12V line to ground with a series of pulses that can be interpreted as bytes.

#### Triggering the sender:

Each Garnet SeeLevel tank sender is configured as sensor 1, 2, or 3 by snipping a tab on the sender. A sender will respond when it sees a sequence of pulses to ground equal to its sensor number. Each pulse needs to be approx 85µs wide. Pulse spacing needs to be approximately 300µs.

The sender will respond by pulling the 12V line to ground in a series of pulses. Pulses will either be approximately 13µs wide or 48µs wide. In this application I'm treating the short pulses as '0', long pulses as '1'. (JIm G. did the opposite)

Bytes returned from sender:

    0:      Unknown
    1:      Checksum
    2 - 10: Fill level from each segment of sender (0 - -255)
    11:     Appears to be 255 in all cases

For the segment fill values, a 'full' value will likely be less than 255, apparently depending on tank wall thickness, tank size, and perhaps how well the sender is attached. In my testing, using a 710AR Rev E taped to a water jug, I see 'full' segments  anywhere from 126 to 200. Pressing on a segment with my thumb will cause the sensor to read a higher value. Capacitance, perhaps?

Testing can be done by temporarily attaching sensor to water jug or by simply touching sensor segments.

Demo app - Output to Serial port upon successful read:

    Tank 0: 147 121 0 0 0 0 14 108 149 179 184 255 Checksum: 121 OK

### Interfacing 12V sensor with 3.3V ESP32:

A description of the level shifter necessary to interface the 3.3V ESP32 with the 12V SeeLevel sensor is [here](./docs/LevelShifter.md). 

The 'Basic Level Shifter' was built with the right-hand side powered by a 12V power supply, an ESP32 as the 3.3V side, and a 2N7000 N-type MOSFET. Resisters were chosen by guessing. I ended up with a 51kohm pullup resister on the 3.3V side and a 560ohm resister on the 12V side, which limits current to approx 25mA. I am not an EE.

A cheap 12V-tolerant logic analyzer (LA1010) was used to assist in debugging.

### Notes:

 * This app doesn't attempt to accommodate a trimmed sender or any sender other than the 710AR Rev E.

 * No attempt is made to process the returned data into an actual liquid level. I'm intending that to be done in some other app (perhaps Node-Red).

 * I'm not confident in the validity of the checksum calculation.

 * Uses Arduino framework but is only tested on an ESP32.
  
 * When the ESP32 pin is pulled low, the ESP32 sinks current from both sides of the circuit, so care must be taken to prevent the ESP32 from going to smoke.

 * My logic analyzer shows much shorter pulse width on 12V side than on 3.3V side. Uncertain as to why. Not an EE. The sender/sensor does respond to the short (1.5µs) pulse however. 

### TBD

This is not a complete solution. To make this usable, one would have to make sure the interface circuit adequately protects both the ESP32 and sending unit; that the checksum math is correct; that the circuit and app work with multiple sensors & trimmed sensors; that the timing is not affected by wire length and type; and much,, much more.