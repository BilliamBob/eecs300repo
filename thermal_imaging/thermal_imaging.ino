// Arduino code for printing AMG8833 Temperature Data to Serial Port
#include <Wire.h>
#include <SparkFun_GridEYE_Arduino_Library.h>

GridEYE amg8833;

void setup() {
  Wire.begin(); // for connecting with AMG8833
  amg8833.begin(); // begin AMG8833 comm
  Serial.begin(115200); // larger baud rate for sending all 64 pixel values
}

void loop() {
  // Print the temperature of each pixel
  for(unsigned char i = 0; i < 64; i++){
    Serial.print(amg8833.getPixelTemperature(i));
    Serial.print(",");
  } 
  // end print with return
  Serial.println();
  // short delay between sends
  delay(100);
}