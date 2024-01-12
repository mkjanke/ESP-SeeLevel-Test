/* ESP32 Test/Demo app: Read Garnet SeeLevel Tank Sensor/Sender

Inspired by Jim G.: https://forums.raspberrypi.com/viewtopic.php?t=119614

To read RV tanks equipped with the Garnet SeeLevel sensors, one can purchase
a display monitor panel that has any of Bluetooth, RV-C, or NEMA2000 - all of
which would be a better interface than this. This is an attempt to read the
sensors (senders) directly, without a Garnet display panel.

Summary of information discovered by Jim G. and documented in the above post:

  To trigger the tank sender/sensor, one needs to power the sender with 12V then
  pull the 12V line to ground in a particular pattern. The sender will respond
  by pulling the 12V line to ground with a series of pulses that can be
  interpreted as bytes.

  Triggering the sender:

  Each Garnet SeeLevel tank sender is configured as sensor 1, 2, or 3 by snipping
  a tab on the sender. A sender will respond when it sees a sequence of pulses
  to ground equal to its sensor number. Each pulse needs to be approx 85µs wide.
  Pulse spacing needs to be approximately 300µs.

  The sender will respond by pulling the 12V line to ground in a series of pulses.
  Pulses will either be approximately 13µs wide or 48µs wide. In this application
  I'm treating the short pulses as '0', long pulses as '1'.

  Bytes returned from sender:

    0:      Unknown
    1:      Checksum
    2 - 10: Fill level from each segment of sender (0 - -255)
    11:     Appears to be 255 in all cases

  For the segment fill values, a 'full' value will likely be less than 255, apparently
  depending on tank wall thickness, tank size, and perhaps how well the sender is
  attached. In my testing, using a 710AR Rev E taped to a water jug, I see 'full' segments
  anywhere from 126 to 200. Pressing on a segment with my thumb will cause the sensor
  to read a higher value. Capacitance, perhaps?

Demo app - Output to Serial port:

    Tank 0: 147 121 0 0 0 0 14 108 149 179 184 255 Checksum: 121 OK

Interfacing 12V sensor with 3.3V ESP32:

To interface, I wired a circuit using this document as a guide:

  https://forums.raspberrypi.com/viewtopic.php?t=119614

  Jim D.'s circuit appears to work fine. For an ESP32, the voltage divider
  on the read pin should be modified to limit pin to no more than 3.3V

Notes:

  This app doesn't attempt to accommodate a trimmed sender or any sender other than the
  710AR Rev E.

  No attempt is made to process the returned data into an actual liquid level. I'm intending
  that to be done in some other app (perhaps Node-Red).

  I'm not confident in the validity of the checksum calculation.

  Uses Arduino framework but is only tested on an ESP32.

*/

#include <Arduino.h>

// ESP32 Write Pin. Set HIGH to power sensors
const int SeeLevelWritePIN = 13;
// ESP32 Read pin. Will be pulled low by sensors
const int SeeLevelReadPIN = 16;

// Time 12V bus is powered before sending pulse(s) to sensor(s)
#define SEELEVEL_POWERON_DELAY 2450
// Width of pulse sent to sensors
#define SEELEVEL_PULSE_LOW_MICROSECONDS 85
// Time between pulses snt to sensors
#define SEELEVEL_PULSE_HIGH_MICROSECONDS 290
// How long to wait for response before timing out
#define SEELEVEL_PULSE_TIMEOUT 10000

// Byte array to store data for 3 tanks x 12 bytes data per tank:
byte SeeLevelData[3][12];

byte readByte();
void readLevel(int);

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nTank Level is Woke!");
  pinMode(SeeLevelWritePIN, OUTPUT);
  pinMode(SeeLevelReadPIN, INPUT);
  digitalWrite(SeeLevelWritePIN, LOW);
  delay(5000);
}

void loop() {
  for (int t = 0; t < 3; t++) {
    Serial.print("Tank " + (String)t + ": ");
    readLevel(t);

    for (int i = 0; i < 12; i++) {
      Serial.print(SeeLevelData[t][i]);
      Serial.print(' ');
    }

    // Verify checksum
    int byteSum = 0;
    for (int i = 2; i <= 10; i++) {  // Sum data bytes
      byteSum = byteSum + SeeLevelData[t][i];
    }
    // Calculate and compare checksum
    int checkSum = SeeLevelData[t][1];
    // Checksum appears to be (sum of bytes % 256) - 1, with special case for checksum 255.
    if (((byteSum % 256) - 1 == checkSum) || (byteSum % 256 == 0 && checkSum == 255)) {
      Serial.print("Checksum: ");
      Serial.print((byteSum % 256) - 1);
      Serial.println(" OK");
    } else {
      Serial.print(" byteSum % 256 - 1 = ");
      Serial.print(" Checksum: ");
      Serial.print((byteSum % 256) - 1);
      Serial.println(" Not OK");
    }
    // Bus must be pulled low for some time before attempting to read another sensor
    delay(1000);
  }
  delay(5000);  // wait between reads
}

//
// Read an individual tank level
// Pass parameter 't', where
//    t = 0 SeeLevel tank 1 (Normally Fresh Tank)
//    t = 1 SeeLevel tank 2 (Normally Grey Tank)
//    t = 2 SeeLevel tank 3 (Normally Black Tank)
//
// Passed variable (t) is 0, 1 or 2
//
void readLevel(int t) {
  
  // Power the sensor line for 2.4 ms so tank levels can be read
  digitalWrite(SeeLevelWritePIN, HIGH);
  delayMicroseconds(SEELEVEL_POWERON_DELAY);

  // 1, 2 or 3 low pulses to select Fresh, Grey, Black tank
  for (int i = 0; i <= t; i++) {
    digitalWrite(SeeLevelWritePIN, LOW);
    delayMicroseconds(SEELEVEL_PULSE_LOW_MICROSECONDS);
    digitalWrite(SeeLevelWritePIN, HIGH);
    delayMicroseconds(SEELEVEL_PULSE_HIGH_MICROSECONDS);
  }

  // Read 12 bytes from sensor, store in array
  for (int byteLoop = 0; byteLoop < 12; byteLoop++) {
    SeeLevelData[t][byteLoop] = readByte();
  }
  delay(10);
  digitalWrite(SeeLevelWritePIN, LOW);  // Turn power off until next poll
}

//
// Read individual bytes from SeeLevel sensor
//
// Sensor pulses are interpreted as approx:
//       48 microseconds for digital '1',
//       13 microseconds for digital '0'
//
// Each byte will roughly approximate the fill level of a segment of the sensor 0 <> 255
//
// A 'full' sensor segment will show somewhere between 126 and 255, depending on tank wall
// thickness, other factors.
//
// Testing can be done by temporarily attaching sensor to water jug or by simply touching
// sensor segments.
//
byte readByte() {
  int result = 0;
  int thisBit = 0;
  for (auto bitLoop = 0; bitLoop < 8; bitLoop++) {  // Populate byte from right to left,
    result = result << 1;                           // Shift right for each incoming bit

    // Wait for low pulse, timeout if no pulse (I.E. no tank present)
    unsigned long pulseTime = (pulseIn(SeeLevelReadPIN, LOW, SEELEVEL_PULSE_TIMEOUT));

    // Decide if pulse is logical '0', '1' or invalid
    if ((pulseTime >= 5) && (pulseTime <= 20)) {
      thisBit = 0;
    } else if ((pulseTime >= 30) && (pulseTime <= 60)) {
      thisBit = 1;
    } else {
      return 0;  // Pulse width out of range, Return zero
    }
    // Bitwise OR and shift pulses into single byte
    result |= thisBit;
  }
  return result;
}
