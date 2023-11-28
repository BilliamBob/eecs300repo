// Arduino code for printing AMG8833 Temperature Data to Serial Port
#include <Wire.h>
#include <SparkFun_GridEYE_Arduino_Library.h>
#include <math.h>
#include "WirelessCommunication.h"
#include "sharedVariable.h"
#include "Preferences.h"

GridEYE amg8833;

#define BUTTON_PIN 0//boot button
void init_non_vol_storage();
void update_non_vol_count();
void update_button_count();

float image[64];
volatile int room_occupancy = 0;
volatile int detectIn = 0;
volatile int detectOut = 0;
volatile int prevdetectIn = 0;
volatile int prevdetectOut = 0;
volatile int state = 0;
volatile float mean = 0;
volatile int timeOut;
volatile int count = 0;
volatile float stdev = 0;
volatile shared_uint32 x;
Preferences nonVol;//used to store the count in nonvolatile memory

void setup() {
  pinMode(BUTTON_PIN, INPUT);
  Wire.begin(); // for connecting with AMG8833
  amg8833.begin(); // begin AMG8833 comm
  Serial.begin(115200); // larger baud rate for sending all 64 pixel values
  init_wifi_task();
  init_non_vol_count();//initializes nonvolatile memory and retrieves latest count
  INIT_SHARED_VARIABLE(x, count);//init shared variable used to tranfer info to WiFi core
}


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
  int num_people = 0;
  int half1 = 0;
  int half2 = 0;
  for(int i = 0; i < 8; i++){
    if(image[i + (row * 8)] > (mean + 1.75*stdev)){
      if(i < 4){
        half1++;
      }
      else{
        half2++;
      }
    }
  }
  if(half1 > 2 && half2 > 2){
    return 2;
  }
  else if(half1 > 0 || half2 > 0){
    return 1;
  }
  return 0;
}

void loop() {
  mean = 0;
  for(unsigned char i = 0; i < 64; i++){
    image[i] = amg8833.getPixelTemperature(i);
  } 

  stdev = calcStDev();
  detectIn = detect(2);
  detectOut = detect(5);
  Serial.println(mean);
  Serial.println(stdev);
  Serial.println(detectIn);
  Serial.println(detectOut);
  Serial.println(state);
  
  /*
  if(stdev < 1.0){
    if(state != 0){
      state = 0;
    }
    detectIn = 0;
    detectOut = 0;
  }
  else if(detectIn && !prevdetectIn && state == 0){
    state = 1;
  }
  else if(detectOut && !prevdetectOut && state == 0){
    state = 2;
  }
  else if(state == 1){
    timeOut++;
    if(detectOut && !prevdetectOut){
      room_occupancy--;
      state = 0;
      timeOut = 0;
    }
    else if(count >= 20){
      state = 0;
      timeOut = 0;
    }
  }
  else if(state == 2){
    timeOut++;
    if(detectIn && !prevdetectIn){
      room_occupancy++;
      state = 0;
      timeOut = 0;
    }
    else if(count >= 20){
      state = 0;
      timeOut = 0;
    }
  }*/
  
  prevdetectIn = detectIn;
  prevdetectOut = detectOut;
  Serial.print("Room Occupancy: ");
  Serial.println(room_occupancy);
  if(count != room_occupancy){
    // end print with return
    count = room_occupancy;
    update_button_count();//update shared variable x (shared with WiFi task)
    update_non_vol_count();//updates nonvolatile count 
  }
  // short delay between sends
  delay(100);
}

//initializes nonvolatile memory and retrieves latest count
void init_non_vol_count()
{
  nonVol.begin("nonVolData", false);//Create a “storage space” in the flash memory called "nonVolData" in read/write mode
  count = nonVol.getUInt("count", 0);//attempts to retrieve "count" from nonVolData, sets it 0 if not found
}

//updates nonvolatile memery with lates value of count
void update_non_vol_count()
{
  nonVol.putUInt("count", count);//write count to nonvolatile memory
}

//example code that updates a shared variable (which is printed to server)
//under the hood, this implementation uses a semaphore to arbitrate access to x.value
void update_button_count()
{
  //minimized time spend holding semaphore
  LOCK_SHARED_VARIABLE(x);
  x.value = count;
  UNLOCK_SHARED_VARIABLE(x);   
}
