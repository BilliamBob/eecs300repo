// Arduino code for printing AMG8833 Temperature Data to Serial Port
#include <Wire.h>
#include <SparkFun_GridEYE_Arduino_Library.h>
#include <math.h>

GridEYE amg8833;

void setup() {
  Wire.begin(); // for connecting with AMG8833
  amg8833.begin(); // begin AMG8833 comm
  Serial.begin(115200); // larger baud rate for sending all 64 pixel values
}

float image[64];
int room_occupancy = 0;
int detect2 = 0;
int detect7 = 0;
int state = 0;
float mean = 0;
int count = 0;
float stdev = 0;

float calcStDev()
{
    int N = 64;
    // variable to store sum of the given data
    float sum = 0;
    for (int i = 0; i < N; i++) {
        sum += image[i];
    }
 
    // calculating mean
    mean = sum / N;
 
    // temporary variable to store the summation of square
    // of difference between individual data items and mean
    float values = 0;
 
    for (int i = 0; i < N; i++) {
        values += pow(image[i] - mean, 2);
    }
 
    // variance is the square of standard deviation
    float variance = values / N;
 
    // calculating standard deviation by finding square root
    // of variance
    return sqrt(variance);
}

int detect(int row){
  for(int i = 0; i < 8; i++){
    if(image[i + (row * 8)] > (mean + 1.5*stdev)){
      return 1;
    }
  }
  return 0;
}

void loop() {
  mean = 0;
  for(unsigned char i = 0; i < 64; i++){
    image[i] = amg8833.getPixelTemperature(i);
  } 

  stdev = calcStDev();
  detect2 = detect(1);
  detect7 = detect(6);
  Serial.println(mean);
  Serial.println(stdev);
  Serial.println(detect2);
  Serial.println(detect7);
  Serial.println(state);

  if(detect2 && state == 0){
    state = 1;
  }
  else if(detect7 && state == 0){
    state = 2;
  }
  else if(state == 1){
    count++;
    if(detect7){
      room_occupancy--;
      state = 0;
    }
    else if(count >= 20){
      state = 0;
      count = 0;
    }
  }
  else if(state == 2){
    count++;
    if(detect2){
      room_occupancy++;
      state = 0;
    }
    else if(count >= 20){
      state = 0;
      count = 0;
    }
  }
  // end print with return
  Serial.print("Room Occupancy: ");
  Serial.println(room_occupancy);
  // short delay between sends
  delay(100);
}