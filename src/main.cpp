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

To interface, I built a level-shifter using this document as a guide:

  https://www.analog.com/en/design-notes/how-to-level-shift-1wire-systems.html

  The 'Basic Level Shifter' was built with the right-hand side powered by a 12V power
  supply, with an ESP32 as the 3.3V side. Resisters were chosen by guessing. I ended 
  up with a 51k pullup resister on the 3.3V side and a 560ohm resister on 
  the 12V side. 

  A cheap 12V-tolerant logic analyzer (LA1010) was used to assist 
  in debugging.

Notes:

  This app doesn't attempt to accommodate a trimmed sender or any sender other than the 
  710AR Rev E.

  No attempt is made to process the returned data into an actual liquid level. I'm intending
  that to be done in some other app (perhaps Node-Red).

  I'm not confident in the validity of the checksum calculation.

  Uses Arduino framework but is only tested on an ESP32.
  
  When the ESP32 pin is pulled low, the ESP32 sinks current from both sides of the 
  circuit, so care must be taken to prevent the ESP32 from going to smoke.
  
*/

#include <Arduino.h>

int SeeLevelPIN = 13;  // ESP32 pin attached to level shifting circuit

// Byte array to store data for 3 tanks x 12 bytes data per tank:
byte SeeLevelData[3][12];

byte readByte();
void readLevel(int);

void setup() {
  Serial.begin(115200);
  Serial.println("\n\nTank Level is Woke!");
  pinMode(SeeLevelPIN, OUTPUT);
  digitalWrite(SeeLevelPIN, LOW);
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
    for (int i = 2; i <= 10; i++) {             // Sum data bytes 
      byteSum = byteSum + SeeLevelData[t][i];
    }
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
  }
  delay(5000);  // wait
}

//
// Read an individual tank level
// Pass parameter 't', where 
//    t = 0 SeeLevel tank 1 (Normally Fresh Tank) 
//    t = 1 SeeLevel tank 2 (Normally Grey Tank)
//    t = 2 SeeLevel tank 3 (Normally Black Tank)
//
void readLevel(int t) {               // Passed variable (t) is 0, 1 or 2
  digitalWrite(SeeLevelPIN, HIGH);    // Power the sensor line for 2.4 ms so tank levels can be read
  delayMicroseconds(2450);
  for (int i = 0; i <= t; i++) {      // 1, 2 or 3 low pulses to select Fresh, Grey, Black tank
    digitalWrite(SeeLevelPIN, LOW);
    delayMicroseconds(85);            // Pulse 85µs down/290µs up
    digitalWrite(SeeLevelPIN, HIGH);
    delayMicroseconds(290);
  }

  // Read 12 bytes from sensor, store in array
  for (int byteLoop = 0; byteLoop < 12; byteLoop++) {
    SeeLevelData[t][byteLoop] = readByte();
  }
  delay(10);                       
  digitalWrite(SeeLevelPIN, LOW);      // Turn power off until next poll
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
    unsigned long pulseTime = (pulseIn(SeeLevelPIN, LOW, 10000));
      
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
