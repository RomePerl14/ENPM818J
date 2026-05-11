#include <SerialWombat.h>

SerialWombatChip sw6C; //Declare a Serial Wombat chip
SerialWombatQuadEnc qeWithPullUps(sw6C);
// This example is explained in a video tutorial at: https://youtu.be/_wO8cOada3w
void setup() 
{
  // put your setup code here, to run once:
  { //I2C Initialization
    Wire.begin();
    sw6C.begin(Wire, 0x6C); //Initialize the Serial Wombat library to use the primary I2C port, SerialWombat is address 6C
  }
  qeWithPullUps.begin(2, 3, 1); // Initialize a QE on pins 2 and 3
  Serial.begin(115200);
}
void loop() 
{
  // Serial.print(" ");
  Serial.print(qeWithPullUps.read());
  Serial.println();
  delay(50);
}
