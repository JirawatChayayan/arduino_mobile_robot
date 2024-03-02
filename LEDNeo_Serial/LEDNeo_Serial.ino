#include "triggerstate.h"
#include "neopixel_control.h"

int color1[3] = {55,0,0};
int color2[3] = {0,55,0};
int color3[3] = {0,0,55};
int mode_int = 0;
unsigned long delaytime = 5;
unsigned long last_msg_running = 0;

int chargingPercent = 0;
NeoPixelControl neoPixel(27,300);

bool color_has_change = false;

String inputString = "";         // a String to hold incoming data
bool stringComplete = false;

void OnActionNeoPixel()
{
  switch(mode_int)
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
  neoPixel.colorUpdate(color1,color2,color3);
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

void serialEvent() 
{
  
  while (Serial.available()) 
  {
    char inChar = (char)Serial.read();
    inputString += inChar;
    if (inChar == '\n') 
    {
      stringComplete = true;
    }
  }
}

void setup() 
{
   Serial.begin(115200);
   inputString.reserve(2000);
   delay(20);
   neoPixel.init();
   neoPixel.setStartup();
   //xTaskCreate(&LEDTwo_Task, "LEDTwo_Task", 2048, NULL, 10, NULL);
   last_msg_running = millis();
}

void loop()
{
  if (stringComplete) 
  {
    handleCommand();
    //Serial.println(inputString);
    inputString = "";
    stringComplete = false;
  }
  neoPixel.DispLoop();
  if(mode_int != 0)
  {
    if(millis()-last_msg_running > 2000)
    {
      //neoPixel.setStartup();
      mode_int = 0;
      OnActionNeoPixel();
      Serial.println('autoReset');
    }
  }
}

void handleCommand()
{
  
  String cmdHeader = inputStringSeparator(inputString, ':',0);
  if(cmdHeader == "chargingPercent")
  {
    String percent = inputStringSeparator(inputString, ':',1);
    chargingPercent = percent.toInt();
    neoPixel.setChargingPercent(chargingPercent);
  }
  else if(cmdHeader == "mode")
  {
    last_msg_running = millis();
    String mode_val = inputStringSeparator(inputString, ':',1);
    String delay_val = inputStringSeparator(inputString, ':',2);
    if(mode_int != mode_val.toInt() || delaytime != delay_val.toInt() || color_has_change)
    {
      color_has_change = false;
      mode_int = mode_val.toInt();
      delaytime = delay_val.toInt();
      OnActionNeoPixel();
    }

  }
  else if(cmdHeader == "color")
  {
    String color1_str = inputStringSeparator(inputString, ':',1);
    String color2_str = inputStringSeparator(inputString, ':',2);
    String color3_str = inputStringSeparator(inputString, ':',3);
    bool change = color_update(color1_str,color2_str,color3_str);
    if(change)
    {
      color_has_change = true;
    }
    last_msg_running = millis();
  }
  else if(cmdHeader=="bright")
  {
    String val = inputStringSeparator(inputString, ':',1);
    neoPixel.brightness(val.toInt());
  }
  else if(cmdHeader=="all")
  {
    //color = r,g,b
    //all:{mode}:{delay}:{color1}:{color2}:{color3}
    last_msg_running = millis();
    String mode_str   = inputStringSeparator(inputString, ':',1);
    String delay_str  = inputStringSeparator(inputString, ':',2);
    String color1_str = inputStringSeparator(inputString, ':',3);
    String color2_str = inputStringSeparator(inputString, ':',4);
    String color3_str = inputStringSeparator(inputString, ':',5);

    bool change_color = color_update(color1_str,color2_str,color3_str);
    bool change_mode  = mode_str.toInt() != mode_int;
    bool change_delay = delay_str.toInt() != delaytime;

    if(change_color || change_mode || change_delay || mode_str == "2")
    {
      color_has_change = false;
      mode_int = mode_str.toInt();
      delaytime = delay_str.toInt();
      OnActionNeoPixel();
    }

  }

}

String inputStringSeparator(String data, char separator, int index) {
  int found = 0;
  int strIndex[] = { 0, -1 };
  int maxIndex = data.length() - 1;

  for (int i = 0; i <= maxIndex && found <= index; i++) {
    if (data.charAt(i) == separator || i == maxIndex) {
      found++;
      strIndex[0] = strIndex[1] + 1;
      strIndex[1] = (i == maxIndex) ? i + 1 : i;
    }
  }
  return found > index ? data.substring(strIndex[0], strIndex[1]) : "";
}

bool color_update(String color1_str,String color2_str,String color3_str)
{
    int color1_R = inputStringSeparator(color1_str, ',',0).toInt();
    int color1_G = inputStringSeparator(color1_str, ',',1).toInt();
    int color1_B = inputStringSeparator(color1_str, ',',2).toInt();

    int color2_R = inputStringSeparator(color2_str, ',',0).toInt();
    int color2_G = inputStringSeparator(color2_str, ',',1).toInt();
    int color2_B = inputStringSeparator(color2_str, ',',2).toInt();

    int color3_R = inputStringSeparator(color3_str, ',',0).toInt();
    int color3_G = inputStringSeparator(color3_str, ',',1).toInt();
    int color3_B = inputStringSeparator(color3_str, ',',2).toInt();


    bool change = false;
    change = (color1[0] != color1_R || color1[1] != color1_G || color1[2] != color1_B ||
              color2[0] != color2_R || color2[1] != color2_G || color2[2] != color2_B ||
              color3[0] != color3_R || color3[1] != color3_G || color3[2] != color3_B );
                 
    color1[0] = color1_R;
    color1[1] = color1_G;
    color1[2] = color1_B;
  
    color2[0] = color1_R;
    color2[1] = color2_G;
    color2[2] = color2_B;
  
    color3[0] = color3_R;
    color3[1] = color3_G;
    color3[2] = color3_B;
    return change;
}
