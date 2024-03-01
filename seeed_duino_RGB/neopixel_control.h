#include <Adafruit_NeoPixel.h>
#ifdef __AVR__
#include <avr/power.h>
#endif

//#define NeoPixelPin A4
//#define NeoPixelNum 300
//Adafruit_NeoPixel strip = Adafruit_NeoPixel(NeoPixelNum,  NeoPixelPin, NEO_GRB + NEO_KHZ800);

class NeoPixelControl {
  private:
    int powerConsumtion = 80;
    byte NeoPixelPin = 4;
    int NeoPixelNum = 300;
    Adafruit_NeoPixel strip;

    int fadeColor[3] = {0, 191, 255};
    bool fadeMode = false;

    int fade2_Color1[3] = {0, 191, 255};
    int fade2_Color2[3] = {0, 191, 255};
    bool fade2Mode = false;
    int fade2State = 0;

    int fade3_Color1[3] = {0, 191, 255};
    int fade3_Color2[3] = {0, 191, 255};
    int fade3_Color3[3] = {0, 191, 255};
    bool fade3Mode = false;
    int fade3State = 0;





    unsigned long fadeT1;
    unsigned int fadeWaitMs = 15;
    int fadecount = 0;
    bool fadeUp = false;
    int fadeMin = 10;
    int fadeMax = 90;
    int fadeStep = 1;

    unsigned long blinkT1;
    unsigned int blinkWaitMs = 100;
    int blink1_Color[3] = {0, 191, 255};
    bool blink1_Mode = false;

    int blink2_Color1[3] = {0, 191, 255};
    int blink2_Color2[3] = {0, 191, 255};
    bool blink2_Mode = false;

    int blink3_Color1[3] = {0, 191, 255};
    int blink3_Color2[3] = {0, 191, 255};
    int blink3_Color3[3] = {0, 191, 255};
    bool blink3_Mode = false;
    int blink_state = 0;

    bool chargingMode = false;
    unsigned long chargingT1 = 0;
    unsigned int chargingWaitMs = 5;
    int chargingPercent = 100;
    int charging_count = 0;
    bool charging_state = false;


    bool StartupMode = false;
    uint16_t Startup_j = 0;
    uint16_t Startup_i = 0;
    unsigned long startupT1 = 0;





  public:
    NeoPixelControl(byte NeoPixelPin, unsigned int NeoPixelNum)
    {
      this->NeoPixelNum = NeoPixelNum;
      this->NeoPixelPin = NeoPixelPin;
    }


    void (*functionPointer)(void);

    void init() {
      strip = Adafruit_NeoPixel(NeoPixelNum,  NeoPixelPin, NEO_GRB + NEO_KHZ800);
#if defined (__AVR_ATtiny85__)
      if (F_CPU == 16000000) clock_prescale_set(clock_div_1);
#endif
      strip.begin();
      strip.show(); // Initialize all pixels to 'off'
      fadeT1 = millis();
      blinkT1 = millis();
      startupT1  = millis();
    }


    void colorUpdate(int color1[3],int color2[3],int color3[3])
    {
      fadeColor[0] = color1[0];
      fadeColor[1] = color1[1];
      fadeColor[2] = color1[2];

      fade2_Color1[0] = color1[0];
      fade2_Color1[1] = color1[1];
      fade2_Color1[2] = color1[2];

      fade2_Color2[0] = color2[0];
      fade2_Color2[1] = color2[1];
      fade2_Color2[2] = color2[2];

      fade3_Color1[0] = color1[0];
      fade3_Color1[1] = color1[1];
      fade3_Color1[2] = color1[2];

      fade3_Color2[0] = color2[0];
      fade3_Color2[1] = color2[1];
      fade3_Color2[2] = color2[2];

      fade3_Color3[0] = color3[0];
      fade3_Color3[1] = color3[1];
      fade3_Color3[2] = color3[2];

      blink1_Color[0] = color1[0];
      blink1_Color[1] = color1[1];
      blink1_Color[2] = color1[2];

      blink2_Color1[0] = color1[0];
      blink2_Color1[1] = color1[1];
      blink2_Color1[2] = color1[2];

      blink2_Color2[0] = color2[0];
      blink2_Color2[1] = color2[1];
      blink2_Color2[2] = color2[2];

      blink3_Color1[0] = color1[0];
      blink3_Color1[1] = color1[1];
      blink3_Color1[2] = color1[2];

      blink3_Color2[0] = color2[0];
      blink3_Color2[1] = color2[1];
      blink3_Color2[2] = color2[2];

      blink3_Color3[0] = color3[0];
      blink3_Color3[1] = color3[1];
      blink3_Color3[2] = color3[2];
    }
    
    void DispLoop()
    {
      fadeProcess();
      blinkProcess();
      chargingProcess();
      startupProcess();
    }

    int percent_cal(int total, int percent)
    {
      functionPointer();
      return (((total * 1.00) / 100.00) * percent);
    }
    //    int percent_cal(int total, int percent)
    //    {
    //      return (((total)/100)*percent);
    //    }

    int spatial_minus(int data1, int data2)
    {
      int val = data1 - data2;
      if (val < 0)
      {
        return 0;
      }
      return val;
    }

    int maxValue(int val[])
    {
      int maxVal = val[0];
      for (int i = 0; i < 3; i++)
      {
        maxVal = max(val[i], maxVal);
      }
      return maxVal;
    }
    int minValue(int val[])
    {
      int minVal = val[0];
      for (int i = 0; i < 3; i++)
      {
        minVal = min(val[i], minVal);
      }
      return minVal;
    }

    void setColorAll(int color[])
    {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, strip.Color(color[0], color[1], color[2]));
        functionPointer();
      }
      strip.show();
    }
    void setColorAll2(uint32_t color)
    {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, color);
        functionPointer();
      }
      strip.show();
    }

    void clearAllState()
    {
      int colorSend[] = {0, 0, 0};
      setColorAll(colorSend);
      setColorAll(colorSend);
      setColorAll(colorSend);
      fadeMode = false;
      fade2Mode = false;
      fade2State = 0;
      fade3Mode = false;
      fade3State = 0;

      blink1_Mode = false;
      blink2_Mode = false;
      blink3_Mode = false;
      blink_state = 0;

      chargingMode = false;
      //chargingPercent = 0;
      charging_count = 0;
      charging_state = true;
      StartupMode = false;

    }

    void fadeProcess()
    {
      if (fadeMode || fade2Mode || fade3Mode)
      {
        if (millis() - fadeT1 > fadeWaitMs)
        {
          fadeT1 = millis();
          if (!fadeUp)
          {
            fadecount += fadeStep;
            if (fadecount > fadeMax)
            {
              fade2State += 1;
              fade3State += 1;
              fadeUp = true;
            }
          }
          else
          {
            fadecount = spatial_minus(fadecount, fadeStep);
            if (fadecount < fadeMin)
            {
              fade2State += 1;
              fade3State += 1;
              fadeUp = false;
            }
          }
          int colorSend[3];
          if (fadeMode)
          {
            colorSend[0] =  percent_cal(fadeColor[0], fadecount);
            colorSend[1] =  percent_cal(fadeColor[1], fadecount);
            colorSend[2] =  percent_cal(fadeColor[2], fadecount);
            fade3State = 0;
            fade2State = 0;
          }
          else if (fade2Mode)
          {

            if (fade2State > 3)
            {
              fade3State = 0;
              fade2State = 0;
            }
            if (fade2State < 2)
            {
              colorSend[0] =  percent_cal(fade2_Color1[0], fadecount);
              colorSend[1] =  percent_cal(fade2_Color1[1], fadecount);
              colorSend[2] =  percent_cal(fade2_Color1[2], fadecount);
            }
            else
            {
              colorSend[0] =  percent_cal(fade2_Color2[0], fadecount);
              colorSend[1] =  percent_cal(fade2_Color2[1], fadecount);
              colorSend[2] =  percent_cal(fade2_Color2[2], fadecount);
            }
          }
          else
          {
            if (fade2State > 5)
            {
              fade3State = 0;
              fade2State = 0;
            }
            if (fade2State < 2)
            {
              colorSend[0] =  percent_cal(fade3_Color1[0], fadecount);
              colorSend[1] =  percent_cal(fade3_Color1[1], fadecount);
              colorSend[2] =  percent_cal(fade3_Color1[2], fadecount);
            }
            else if (fade2State < 4)
            {
              colorSend[0] =  percent_cal(fade3_Color2[0], fadecount);
              colorSend[1] =  percent_cal(fade3_Color2[1], fadecount);
              colorSend[2] =  percent_cal(fade3_Color2[2], fadecount);
            }
            else
            {
              colorSend[0] =  percent_cal(fade3_Color3[0], fadecount);
              colorSend[1] =  percent_cal(fade3_Color3[1], fadecount);
              colorSend[2] =  percent_cal(fade3_Color3[2], fadecount);
            }
          }
          setColorAll(colorSend);
        }
      }
      else
      {
        fadecount = 0;
        fadeT1 = millis();
      }
    }

    void setFade(int color[3], unsigned int fadeTime)
    {
      clearAllState();
      fadeColor[0] = color[0];
      fadeColor[1] = color[1];
      fadeColor[2] = color[2];
      fadeMode = true;
      fadeWaitMs = fadeTime;
    }

    void setFade2(int color1[3], int color2[3], unsigned int fadeTime)
    {
      clearAllState();
      fade2_Color1[0] = color1[0];
      fade2_Color1[1] = color1[1];
      fade2_Color1[2] = color1[2];

      fade2_Color2[0] = color2[0];
      fade2_Color2[1] = color2[1];
      fade2_Color2[2] = color2[2];
      fade2Mode = true;
      fadeWaitMs = fadeTime;
    }
    void setFade3(int color1[3], int color2[3], int color3[3], unsigned int fadeTime)
    {
      clearAllState();
      fade3_Color1[0] = color1[0];
      fade3_Color1[1] = color1[1];
      fade3_Color1[2] = color1[2];

      fade3_Color2[0] = color2[0];
      fade3_Color2[1] = color2[1];
      fade3_Color2[2] = color2[2];

      fade3_Color3[0] = color3[0];
      fade3_Color3[1] = color3[1];
      fade3_Color3[2] = color3[2];
      fade3Mode = true;
      fadeWaitMs = fadeTime;
    }

    void setOnColor(int color[3])
    {
      clearAllState();
      setColorAll(color);
      delay(1);
      setColorAll(color);
      delay(1);
      setColorAll(color);
      delay(1);
      setColorAll(color);

    }

    void blinkProcess()
    {
      if (blink1_Mode || blink2_Mode || blink3_Mode)
      {
        if (millis() - blinkT1 > blinkWaitMs)
        {
          blinkT1 = millis();
          blink_state += 1;
          int color[3];
          if (blink1_Mode)
          {
            switch (blink_state)
            {
              case 1:
                color[0] = blink1_Color[0];
                color[1] = blink1_Color[1];
                color[2] = blink1_Color[2];
                break;
              case 2:
                color[0] = 0;
                color[1] = 0;
                color[2] = 0;
                blink_state = 0;
                break;
            }
          }
          else if (blink2_Mode)
          {
            switch (blink_state)
            {
              case 1:
                color[0] = blink2_Color1[0];
                color[1] = blink2_Color1[1];
                color[2] = blink2_Color1[2];
                break;
              case 2:
                color[0] = blink2_Color2[0];
                color[1] = blink2_Color2[1];
                color[2] = blink2_Color2[2];
                blink_state = 0;
                break;
            }
          }
          else
          {
            switch (blink_state)
            {
              case 1:
                color[0] = blink3_Color1[0];
                color[1] = blink3_Color1[1];
                color[2] = blink3_Color1[2];
                break;
              case 2:
                color[0] = blink3_Color2[0];
                color[1] = blink3_Color2[1];
                color[2] = blink3_Color2[2];
                break;
              case 3:
                color[0] = blink3_Color3[0];
                color[1] = blink3_Color3[1];
                color[2] = blink3_Color3[2];
                blink_state = 0;
                break;
            }

          }
          setColorAll(color);
        }
      }
      else
      {
        blinkT1 = millis();
        blink_state = 0;
      }
    }

    void setBlink(int color1[3], unsigned int blinkTime)
    {
      clearAllState();
      blink1_Color[0] = color1[0];
      blink1_Color[1] = color1[1];
      blink1_Color[2] = color1[2];



      blink1_Mode = true;
      blinkWaitMs = blinkTime;
    }

    void setBlink2(int color1[3], int color2[3], unsigned int blinkTime)
    {
      clearAllState();
      blink2_Color1[0] = color1[0];
      blink2_Color1[1] = color1[1];
      blink2_Color1[2] = color1[2];

      blink2_Color2[0] = color2[0];
      blink2_Color2[1] = color2[1];
      blink2_Color2[2] = color2[2];

      blink2_Mode = true;
      blinkWaitMs = blinkTime;
    }
    void setBlink3(int color1[3], int color2[3], int color3[3], unsigned int blinkTime)
    {
      clearAllState();
      blink3_Color1[0] = color1[0];
      blink3_Color1[1] = color1[1];
      blink3_Color1[2] = color1[2];

      blink3_Color2[0] = color2[0];
      blink3_Color2[1] = color2[1];
      blink3_Color2[2] = color2[2];

      blink3_Color3[0] = color3[0];
      blink3_Color3[1] = color3[1];
      blink3_Color3[2] = color3[2];
      blink3_Mode = true;
      blinkWaitMs = blinkTime;
    }



    void chargingProcess()
    {
      if (chargingMode)
      {
        functionPointer();
        if(chargingPercent <100)
        {
          if (millis() - chargingT1 > chargingWaitMs)
          {
            chargingT1 = millis();
            if (charging_count > strip.numPixels())
            {
              charging_count = 0;
              charging_state = !charging_state;
            }
  
            if (charging_state)
            {
              functionPointer();
              strip.setPixelColor(charging_count, Wheel((percent_cal(100, chargingPercent)) & 90));
              strip.show();
            }
            else
            {
              functionPointer();
              strip.setPixelColor(charging_count, strip.Color(0, 0, 0));
              strip.show();
            }
            charging_count += 1;
          }
        }
        else
        {
          if (millis() - chargingT1 > 300)
          {
            chargingT1 = millis();
            if (charging_state)
            {
              setColorAll2(Wheel((percent_cal(100, chargingPercent)) & 90));
            }
            else
            {
              setColorAll2(strip.Color(0, 0, 0));
            }
            charging_state = !charging_state;
          }
        }
      }
      else
      {
        chargingT1 = millis();
        //chargingPercent = 0;
      }
    }

    void setChargingPercent(int percent)
    {
      chargingPercent = percent;
    }
    void setCharging()
    {
      clearAllState();
      chargingT1 = millis();
      chargingMode = true;
    }

    void startupProcess()
    {
      if (StartupMode)
      {
        if(millis() - startupT1 > 10)
        {
          startupT1 = millis();
          if (Startup_j > 256 * 5)
          {
            Startup_j = 0;
          }
          for (uint16_t Startup_i = 0; Startup_i < strip.numPixels(); Startup_i++)
          {
            functionPointer();
            strip.setPixelColor(Startup_i, Wheel(((Startup_i * 256 / strip.numPixels()) + Startup_j) & 255));
          }
          strip.show();
          Startup_j++;
        }
      }
    }
    
    void setStartup()
    {
      clearAllState();
      StartupMode = true;
    }

    // Fill the dots one after the other with a color
    void colorWipe(uint32_t c, uint8_t wait)
    {
      for (uint16_t i = 0; i < strip.numPixels(); i++) {
        strip.setPixelColor(i, c);
        strip.show();
        delay(wait);
      }
    }

    void rainbow(uint8_t wait)
    {
      uint16_t i, j;

      for (j = 0; j < 256; j++) {
        for (i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, Wheel((i + j) & 255));
        }
        strip.show();
        delay(wait);
      }
    }

    // Slightly different, this makes the rainbow equally distributed throughout
    void rainbowCycle(uint8_t wait)
    {
      uint16_t i, j;

      for (j = 0; j < 256 * 5; j++) { // 5 cycles of all colors on wheel
        for (i = 0; i < strip.numPixels(); i++) {
          strip.setPixelColor(i, Wheel(((i * 256 / strip.numPixels()) + j) & 255));
        }
        strip.show();
        delay(wait);
      }
    }

    //Theatre-style crawling lights.
    void theaterChase(uint32_t c, uint8_t wait)
    {
      for (int j = 0; j < 10; j++) { //do 10 cycles of chasing
        for (int q = 0; q < 3; q++) {
          for (int i = 0; i < strip.numPixels(); i = i + 3) {
            strip.setPixelColor(i + q, c);  //turn every third pixel on
          }
          strip.show();

          delay(wait);

          for (int i = 0; i < strip.numPixels(); i = i + 3) {
            strip.setPixelColor(i + q, 0);      //turn every third pixel off
          }
        }
      }
    }

    //Theatre-style crawling lights with rainbow effect
    void theaterChaseRainbow(uint8_t wait)
    {
      for (int j = 0; j < 256; j++) {   // cycle all 256 colors in the wheel
        for (int q = 0; q < 3; q++) {
          for (int i = 0; i < strip.numPixels(); i = i + 3) {
            strip.setPixelColor(i + q, Wheel( (i + j) % 255)); //turn every third pixel on
          }
          strip.show();

          delay(wait);

          for (int i = 0; i < strip.numPixels(); i = i + 3) {
            strip.setPixelColor(i + q, 0);      //turn every third pixel off
          }
        }
      }
    }

    // Input a value 0 to 255 to get a color value.
    // The colours are a transition r - g - b - back to r.
    uint32_t Wheel(byte WheelPos)
    {
      functionPointer();
      WheelPos = 255 - WheelPos;
      if (WheelPos < 85) {
        //return strip.Color(percent_cal(255 - WheelPos * 3,powerConsumtion), 0, percent_cal(WheelPos * 3,powerConsumtion));
        return strip.Color(255 - WheelPos * 3, 0, WheelPos * 3);
      }
      functionPointer();
      if (WheelPos < 170) {
        WheelPos -= 85;
        //return strip.Color(0, percent_cal(WheelPos * 3,powerConsumtion), percent_cal(255 - WheelPos * 3,powerConsumtion));
        return strip.Color(0, WheelPos * 3, 255 - WheelPos * 3);
      }
      functionPointer();
      WheelPos -= 170;
      //return strip.Color(percent_cal(WheelPos * 3,powerConsumtion), percent_cal(255 - WheelPos * 3,powerConsumtion), 0);
      return strip.Color(WheelPos * 3, 255 - WheelPos * 3, 0);
    }


};
