
//rosrun rosserial_python serial_node.py _port:=/dev/ttyUSB3
#define USE_USBCON
#if (ARDUINO >= 100)
    #include <Arduino.h>
#else
    #include <WProgram.h>
#endif


#include <ros.h>
#include <ros/time.h>
#include <std_msgs/Bool.h>
#include <std_msgs/UInt8MultiArray.h>
#include <std_msgs/UInt8.h>
#include "triggerstate.h"
#include "neopixel_control.h"

ros::NodeHandle_<ArduinoHardware, 5, 5, 512, 512> nh;

std_msgs::Bool lidarL,lidarR,door;
ros::Publisher pub_lidarL("utl_ext_io/lidar_detection_L", &lidarL);
ros::Publisher pub_lidarR("utl_ext_io/lidar_detection_R", &lidarR);
ros::Publisher pub_door("utl_ext_io/door_status", &door);

int color1[3] = {55,0,0};
int color2[3] = {0,55,0};
int color3[3] = {0,0,55};
int mode = 0;
int delaytime = 5;
int chargingPercent = 0;
bool on_requested = false;


TriggerInput inputLidarL(14,0);
TriggerInput inputLidarR(15,0);
TriggerInput inputDoor(16,35);

NeoPixelControl neoPixel(A4,300);

unsigned long T1,T2;
int Priority2_Count = 0; 
int Priority2_Set = 0; 
bool hasChange = false;
void message_color(const std_msgs::UInt8MultiArray& obj)
{
  //nh.loginfo("Recived Color");

  
  if(color1[0] != obj.data[0] || color1[1] != obj.data[1] || color1[2] != obj.data[2] ||
     color2[0] != obj.data[3] || color2[1] != obj.data[4] || color2[2] != obj.data[5] ||
     color3[0] != obj.data[6] || color3[1] != obj.data[7] || color3[2] != obj.data[8])
     {
      hasChange = true;
     }
  color1[0] = obj.data[0];
  color1[1] = obj.data[1];
  color1[2] = obj.data[2];

  color2[0] = obj.data[3];
  color2[1] = obj.data[4];
  color2[2] = obj.data[5];

  color3[0] = obj.data[6];
  color3[1] = obj.data[7];
  color3[2] = obj.data[8];

//  if(hasChange)
//  {
//    nh.loginfo("color change");
//  }
}

void message_mode(const std_msgs::UInt8MultiArray& obj)
{
 // nh.loginfo("Recived Mode");
  if(mode == obj.data[0] && !hasChange)
  {
    return;
  }
  hasChange = false;
  mode = obj.data[0];
  delaytime = obj.data[1];
  //on_requested = true;
  //neoPixel.colorUpdate(color1,color2,color3);
  OnActionNeoPixel();
  neoPixel.colorUpdate(color1,color2,color3);
}

void message_chargingPercent(const std_msgs::UInt8& obj)
{
  chargingPercent = obj.data;
  neoPixel.setChargingPercent(chargingPercent);
}

void message_cb(const std_msgs::Bool& obj)
{
  if(obj.data)
  {
    neoPixel.setOnColor(color1);
  }
  else
  {
    setOffLED();
  }

}

ros::Subscriber<std_msgs::UInt8MultiArray> sub_color("utl_ext_io/neo_pixel/color", &message_color);
ros::Subscriber<std_msgs::UInt8MultiArray> sub_mode("utl_ext_io/neo_pixel/mode", &message_mode);
ros::Subscriber<std_msgs::UInt8> sub_percent("utl_ext_io/neo_pixel/chargingPercent", &message_chargingPercent);
ros::Subscriber<std_msgs::Bool> sub("utl_ext_io/neo_pixel", &message_cb);



void callback()
{
//  nh.spinOnce();
//  nh.loginfo("Recived Callback");
//  //digitalWrite(33,!digitalRead(33));
  //publish_loop();
}



void setup() 
{
   neoPixel.init();
   neoPixel.functionPointer = &callback;
   Serial.begin(115200);
   delay(1);
   nh.getHardware()->setBaud(115200);
   delay(1);
   nh.initNode();
   nh.advertise(pub_lidarL);
   nh.advertise(pub_lidarR);
   nh.advertise(pub_door);

   nh.subscribe(sub_color);
   nh.subscribe(sub_mode);
   nh.subscribe(sub_percent);
   nh.subscribe(sub);

   T1 = millis();
   T2 = millis();
   neoPixel.setStartup();
   //pinMode(A15,OUTPUT);
}

void loop() {

    if(millis()-T2 > 5)
    {
       T2 = millis();
       neoPixel.DispLoop();
    }
    //neoPixel.DispLoop();
    publish_loop();

}

void publish_loop()
{
    lidarL.data = inputLidarL.getState();
    lidarR.data = inputLidarR.getState();
    door.data = inputDoor.getState();
    if(millis()-T1 > 20)
    {
      T1 = millis();
      pub_lidarL.publish(&lidarL);
      pub_lidarR.publish(&lidarR);
      pub_door.publish(&door);
    }
    
    //digitalWrite(A15,!digitalRead(A15));
    nh.spinOnce();

}

void OnActionNeoPixel()
{
  switch(mode)
  {
    case 0:
      // Startup and ShuttingDown
      neoPixel.setStartup();
      
      break;
    case 1:
      // Charging
      neoPixel.setCharging();
      break;
    case 2:
      // SetOn
      setOnLED();
      break;
    case 3:
      // SetFade1
      setFadeLED();
      break;
    case 4:
      // SetFade2
      setFade2LED();
      break;
    case 5:
      // SetFade3
      setFade3LED();
      break;
    case 6:
      // SetBlink1
      setBlinkLED();
      break;
    case 7:
      // SetBlink2
      setBlink2LED();
      break;
    case 8:
      // SetBlink3
      setBlink3LED();
      break;
    default:
      setOffLED();
      break;
  }
}














void setFadeLED()
{
  neoPixel.setFade(color1,delaytime);
}
void setFade2LED()
{
  neoPixel.setFade2(color1,color2,delaytime);
}
void setFade3LED()
{
  neoPixel.setFade3(color1,color2,color3,delaytime);
}

void setBlinkLED()
{
  neoPixel.setBlink(color1,delaytime);
}
void setBlink2LED()
{
  neoPixel.setBlink2(color1,color2,delaytime);
}
void setBlink3LED()
{
  neoPixel.setBlink3(color1,color2,color3,delaytime);
}


void setOnLED()
{
  neoPixel.setOnColor(color1);
}

void setOffLED()
{
  neoPixel.clearAllState();
}
