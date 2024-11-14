## ESP32 Test/Demo app: Read Garnet SeeLevel Tank Sensor/Sender

Inspired by Jim G., who did all the heavy lifting: https://forums.raspberrypi.com/viewtopic.php?t=119614

To read RV tanks equipped with the Garnet SeeLevel sensors, one can purchase a display monitor panel that has any of Bluetooth, RV-C, or NEMA2000 - all of which would be a better interface than this. This is an attempt to read the sensors (senders) directly, without a Garnet display panel. 

### Summary of information discovered by Jim G. and documented in the above post:

To trigger the tank sender/sensor, one needs to power the sender with 12V then pull the 12V line to ground in a particular pattern. The sender will respond by pulling the 12V line to ground with a series of pulses that can be interpreted as bytes.

#### Triggering the sender:

Each Garnet SeeLevel tank sender is configured as sensor 1, 2, or 3 by snipping a tab on the sender. A sender will respond when it sees a sequence of pulses to ground equal to its sensor number. Each pulse needs to be approx 85µs wide. Pulse spacing needs to be approximately 300µs.

The sender will respond by pulling the 12V line to ground in a series of pulses. Pulses will either be approximately 15µs wide or 50µs wide. In this application I'm treating the short pulses as '0', long pulses as '1'. (Jim G. did the opposite)

Bytes returned from sender:

    0:      Unknown
    1:      Checksum
    2 - 10: Fill level from each segment of sender (0 - -255)
    11:     Appears to be 255 in all cases

For the segment fill values, a 'full' value will likely be less than 255, apparently depending on tank wall thickness, tank size, and perhaps how well the sender is attached. In my testing, using a 710AR Rev E taped to a water jug, I see 'full' segments  anywhere from 126 to 200. Pressing on a segment with my thumb will cause the sensor to read a higher value. Capacitance, perhaps?

Testing can be done by temporarily attaching sensor to water jug or by simply touching sensor segments.

Demo app - Output to Serial port upon successful read:

    Tank 0: 147 121 0 0 0 0 14 108 149 179 184 255 Checksum: 121 OK

### Interpreting sensor data:

The sensor data returned from the sender is not usable without interpretation. Depending on tank configuration, tank contaminations & sensor placement:

 * Readings from segments may show values greater than zero for an empty segment. My tanks show segment readings anywhere from 0 to 20 when empty.

 * A full segment may show a value less than 255. My tanks, for example, read a full segment anywhere from 126 to 180, depending on the tank construction and sensor placement. One tank might read full segments at 126, another might read full at 160 or 180.

#### Pseudo code for interpreting sensor data:

For each sensor, set a minimum threshold value that represents a valid reading.

For each sensor, set an upper threshold value that represents a full tank. 

```
min = 15
full = 160.
```
Process the segment readings from left to right -- I.E. from top of tank to bottom of tank.

For each segment, set any segment whose value is less than the per-tank minimum threshold to zero. This eliminates 'noise' from segments when tanks walls are not perfectly dry or segments are affected by placement of nearby tank fittings, etc.

```
if (segment[i] < min) {
    segment[i] = 0
}
```
For each segment, set any segment whose value is greater than the per-tank full threshold to the full value.

```
if (segment[i] > full) {
    segment[i] = full
}
```
For each segment, if the next segment is less than the current segment, the current segment's reading is invalid. Set the current segment to zero. This assures that only segments that increase in value from the prior segment are represented in the final calculation.

```
if (segment[i+1] < segment[i]) {
    segment[i] = 0
}
```
For each segment, if the next segment is less than the full threshold, the current segment's reading is invalid. Set the current segment to zero. This assures that only one partially filled segment is represented in the final calculation.

```
if (segment[i+1] < full) {
    segment[i] = 0
}
```
Add together the rationalized values for each segment and divide by the total number of segments. This is the final value for the tank.

```
final_value = sum(segment) / total_segments
```
#### AlgorithmNotes:
 * This algorithm is based on the assumption that the sensor data is linearly related to the tank level.

 * This algorithm is still an approximation. I've found that it can be off by 5-10% when the tank is partially full, but is more accurate when near-empty or near-full. 

### Interfacing 12V sensor with 3.3V ESP32:

A description of the circuit necessary to interface the 3.3V ESP32 with the 12V SeeLevel sensor is [here](./docs/LevelShifter.md). 

The interface, designed by Jim G. of the Raspberry Pi forum, uses a high-side P-channel MOSFET controlled by an ESP32 3.3V pin. Data is read on a second pin via a voltage divider.

A cheap 12V-tolerant logic analyzer (LA1010) was used to assist in debugging.

### Project Notes:

 * This app doesn't do anything special with a trimmed sender. Sensor pads that are trimmed off will return value of '255'.

 * No attempt is made to process the returned data into an actual liquid level. I'm intending that to be done in some other app (perhaps Node-Red).

 * Uses Arduino framework but is only tested on an ESP32.

 * A Python SeeLevel tank gauge reader is [here](https://github.com/robwolff3/seelevel2mqtt/) 
 
 * A version that uses the ESP32 RMT preiphrial and broadcasts its data via ESP-NOW is here: https://github.com/mkjanke/ESP32-SeeLevel-NOW
 
 * An ESPHome version, with KiCAD drawings and PCB files: https://github.com/j9brown/esphome-seelevel


### TBD

* This is not a complete solution. To make this usable, one would have to make sure the interface circuit adequately protects both the ESP32 and sending unit. The version created by j9brown linked above adds a transistor that protects the ESP32 input pin.
* Based on information in j9bron's repository, the checksum returned from the sending units is a 12-bit sum, not an 8-bit sum as implemented in this repository. This repository will need to be updated to reflect that. 
