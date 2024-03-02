#include <Arduino.h>
#include <SlowSoftI2CMaster.h>
#include <avr/io.h>

SlowSoftI2CMaster si = SlowSoftI2CMaster(4,5);



int ultrasonic_range[8];

const char* sensor_id[8] = {"0x6C", "0x6D", "0x6E", "0x6F", "0x71", "0x72", "0x73", "0x74"};
int arraySize = sizeof(sensor_id) / sizeof(sensor_id[0]);
const int numSamples = 100;
unsigned long previosUpdateTime = 0;
const unsigned long updateInterval = 20; //100hz
int ultrasonic_samples[8][numSamples];
int sampleIndex = 0;



boolean start_sensor(byte bit8address)
{
  boolean errorlevel = 0;
  bit8address = bit8address & B11111110; //Do a bitwise 'and' operation to force the last bit to be zero -- we are writing to the address.
  errorlevel = !si.i2c_start(bit8address) | errorlevel; //Run i2c_start(address) while doing so, collect any errors where 1 = there was an error.
  errorlevel = !si.i2c_write(81) | errorlevel; //Send the 'take range reading' command. (notice how the library has error = 0 so I had to use "!" (not) to invert the error)
 
  si.i2c_stop();
  return errorlevel;
}

byte ByteAddress (char*address)
{
  byte a = strtoul(address, NULL, 16)*2;
  return a;
}

boolean read_sensor(byte bit8address,int &range)
{
  boolean errorlevel = 0;
  byte range_highByte = 0;
  byte range_lowByte = 0;
  bit8address = bit8address | B00000001;
  errorlevel = !si.i2c_start(bit8address) | errorlevel;
  range_highByte = si.i2c_read(0);
  range_lowByte = si.i2c_read(1);
  si.i2c_stop();
  range = (range_highByte * 256)+range_lowByte;
  if(errorlevel){
    return false;
  }
  else
  {
    return true;
  }
}


void update_data()
{
  
  unsigned long currentMillis = millis();
  if(currentMillis - previosUpdateTime >= updateInterval)
  {
    previosUpdateTime = currentMillis;
    boolean error = 0;
    String result = ":";
    for(int i = 0; i<8; i++)
    {
  
      byte addr = strtoul(sensor_id[i], NULL, 16)*2;
      error = 0;
      error = start_sensor(addr);
      if (!error) 
      {
        int range = 0;
        if(read_sensor(addr, range)){
          ultrasonic_range[i]= range;
          result += String(ultrasonic_range[i]); 
          
        }
//        result += String(ultrasonic_range[i]); 
      } 
//      result += String(ultrasonic_range[i]);
      if(i!= 7)
      {
        result+=",";
      }
    }
    result+= ";"; 
    Serial.println(result);
  }
}

void setup() 
{
  delay(10);
  Serial.begin(115200);
  si.i2c_init();
}

void loop() 
{
  update_data();
}
