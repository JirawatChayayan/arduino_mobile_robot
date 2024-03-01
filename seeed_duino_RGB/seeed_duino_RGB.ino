#include "neopixel_control.h"

NeoPixelControl neoPixel(1,300);

void callback_long_loop()
{
  //for_ros_update
}

void setup() 
{
  Serial.begin(115200);
  neoPixel.init();
  neoPixel.functionPointer = &callback_long_loop;
  neoPixel.setStartup();
}

void loop() 
{
  unsigned long t1 = micros();
  neoPixel.DispLoop();
  unsigned long val = micros()-t1;
  Serial.println("Total Processingtime in loop is "+String(val)+" uS");
}
