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
float oldImage[64];
float deltas[64];

volatile int room_occupancy = 0;
volatile int detectIn = 0;
volatile int detectOut = 0;
volatile int prevDetectIn = 0;
volatile int prevDetectOut = 0;
volatile int state = 0;
volatile float mean = 0;
volatile float stdev = 0;

volatile float imageStdev = 0;
volatile float imageMean = 0;
float thres = 1.7;

volatile int timeOutIn;
volatile int timeOutOut;
volatile int count = 0;
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

float calcStDev(float arr[])
{
    int N = 64;
    // variable to store sum of the given data
    float sum = 0;
    for (int i = 0; i < N; i++) {
        sum += arr[i];
    }
 
    // calculating mean
    mean = sum / N;
 
    // temporary variable to store the summation of square
    // of difference between individual data items and mean
    float values = 0;
 
    for (int i = 0; i < N; i++) {
        values += pow(arr[i] - mean, 2);
    }
 
    // variance is the square of standard deviation
    float variance = values / N;
 
    // calculating standard deviation by finding square root
    // of variance
    return sqrt(variance);
}

float imageCalcStDev(float arr[])
{
    int N = 64;
    // variable to store sum of the given data
    float sum = 0;
    for (int i = 0; i < N; i++) {
        sum += arr[i];
    }
 
    // calculating mean
    imageMean = sum / N;
 
    // temporary variable to store the summation of square
    // of difference between individual data items and mean
    float values = 0;
 
    for (int i = 0; i < N; i++) {
        values += pow(arr[i] - imageMean, 2);
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
    if(deltas[i + (row * 8)] < (mean - 1.1*stdev)){
      if(i < 4){
        half1++;
      }
      else{
        half2++;
      }
    }
  }
  if (half1 > 2 && half2 > 2) {
    Serial.println("WAOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");
    return 2;
  }
  else if (half1 > 0 || half2 > 0) {
    return 1;
  }
  return 0;
}

int detectCol(int col){
  int num_people = 0;
  int half1 = 0;
  int half2 = 0;

  if (stdev < 0.55) {
    return 0;
  }

  for(int i = 0; i < 8; i++){
    if(deltas[col + i*8] < mean-thres*stdev){
      if (i<4) {
        half1++;
      } else {
        half2++;
      }
    }
  }
  if (half1 > 1 && half2 > 1) {
    Serial.println("WAOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOOO");

    return 2;
  }
  else if (half1 > 0 || half2 > 0) {
    return 1;
  }
  return 0;
}

void loop() {
  mean = 1000;
  for(unsigned char i = 0; i < 64; i++){
    image[i] = amg8833.getPixelTemperature(i); //reading current into image
  }

  for(unsigned char i = 0; i < 64; i++){
    deltas[i] = image[i] - oldImage[i];
  }


  stdev = calcStDev(deltas);

  detectIn = detectCol(5);
  detectOut = detectCol(3);
  Serial.print("deltas mean ");
  Serial.println(mean);
  Serial.print("deltas stdev ");
  Serial.println(stdev);
  Serial.print("detectIn ");
  Serial.println(detectIn);
  Serial.print("detectOut ");
  Serial.println(detectOut);
  Serial.print("prevDetectIn ");
  Serial.println(prevDetectIn);
  Serial.print("prevDetectOut ");
  Serial.println(prevDetectOut);


  //Serial.println(state);
  
//have a timeout
//evaluate between 

  imageStdev = imageCalcStDev(image);
  Serial.print("image mean ");
  Serial.println(imageMean);
  Serial.print("image stdev ");
  Serial.println(imageStdev);
  if (imageStdev < 1.0) { //nobody in sensor
    detectIn = 0;
    detectOut = 0;
    prevDetectIn = 0;
    prevDetectOut = 0;
    timeOutIn = 0;
    timeOutOut = 0; //reset stuff to be ready for next challenger!
    Serial.println("RESET SHMUCKS!");
  } else { //someone is here!
  
    if (detectIn) {
      timeOutIn = 0;
      if (prevDetectOut) { //people entering room completed
        room_occupancy += (min(detectIn, prevDetectOut));
        prevDetectOut -= min(detectIn, prevDetectOut);
        if (detectIn > prevDetectOut) { //detectIn = 2, prevDetectOut = 1 case handling
          prevDetectIn = detectIn - prevDetectOut; 
        }
      } else { //people beginning to exit
        prevDetectIn = detectIn;
        timeOutIn = 0;
      }
    }
    if (detectOut) {
      timeOutOut = 0;
      if (prevDetectIn) { //people entering room completed
        room_occupancy -= (min(detectOut, prevDetectIn));
        prevDetectIn -= min(detectOut, prevDetectIn);
        if (detectOut > prevDetectIn) { //detectOut = 2, prevDetectIn = 1 case handling
          prevDetectOut = detectOut - prevDetectIn; 
        }
      } else { //people beginning to exit
        prevDetectOut = detectOut;
        timeOutOut = 0;
      }
    }
  }
  timeOutIn++;
  timeOutOut++;

  if (timeOutIn > 10) {
    prevDetectIn = 0;
    Serial.println("TIMED OUT");
  }
  if (timeOutOut > 10) {
    prevDetectOut = 0;
    Serial.println("TIMED OUT");
  }


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
  


  // prevdetectIn = detectIn;
  // prevdetectOut = detectOut;
  //memcpy(oldImage, image, 64);
  //oldImage = image;

  for (unsigned char i = 0; i < 64; i++) {
    oldImage[i] = image[i]; //oldImag here!
  }  


  Serial.println("FULL DEBUG DELTAS");
   for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      Serial.print(deltas[8*i + j] < (mean-thres*stdev));
      Serial.print(" ");
    }
    Serial.println();
  }

  if (room_occupancy < 0) {
    room_occupancy = 0;
  }

  Serial.print("Room Occupancy: ");
  Serial.println(room_occupancy);


Serial.println("FULL DEBUG IMAGE");
   for (int i = 0; i < 8; i++) {
    for (int j = 0; j < 8; j++) {
      Serial.print(image[8*i + j] );
      Serial.print(" ");
    }
    Serial.println();
  }

  Serial.print("Room Occupancy: ");
  Serial.println(room_occupancy);
  // for (int i = 0; i < 8; i++) {
  //   for (int j = 0; j < 8; j++) {
  //     Serial.print(deltas[8*i + j] < (mean-1.75*stdev));
  //     Serial.print(" ");
  //   }
  //   Serial.println();
  // }

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
