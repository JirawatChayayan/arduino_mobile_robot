#include <WiFi.h>
#include <ArduinoOTA.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <EEPROM.h>



#define ssid          "CIM_WIFI"
#define password      NULL
#define MQTT_SERVER   "10.151.27.1"
#define MQTT_PORT     1883

String MQTT_Prefix = "ElevatorYY";

// IO
#define ip_door1  39
#define ip_door2  36
#define ip_maintain  35
#define ip_outofservice 34

#define op_floor1 13
#define op_floor2 14
#define op_calltofloor1 27
#define op_calltofloor2 26
#define op_opendoor 25
#define op_closedoor 33
#define blink_io 2
#define wifi_io 32


WiFiClient client;
PubSubClient mqtt(client);
HTTPClient http;
unsigned long schedule_ping = millis();
unsigned long io_update = millis();
unsigned long state_update = millis();
String serverPath = "http://10.151.27.1/ClockService/api/datetime";

const int numParams = 2;
void mqtt_topic_sub_generator(String (& params) [numParams])
{
  String topic_cmd = String() + MQTT_Prefix + "/" +"cmd";
  String topic_rename = String() + MQTT_Prefix + "/" + "device_name";

  params [0] = topic_cmd;
  params [1] = topic_rename;
}


//closedoor
int state_idx_close_door = 0;

//call elev to floor
int state_idx_call_ele = 0;
int elevator_call_floor = 0;

unsigned long last_state_time = millis();
bool onProcess = false; 
String processName = "";
DynamicJsonDocument stateControl(1024);
bool test = false;
unsigned long test_time = micros();
unsigned long sum_test = 0;
float count_test = 0;

int count_reset_io = 0;
void setup() 
{
  init_io();
  Serial.begin(115200);
  delay(10);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  initEEPROM();
  waitting_for_connection();
  ota_init();
  mqtt_init();
  init_state();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}


void loop() 
{
  ArduinoOTA.handle();
  mqtt.loop();
  ping_server();
  update_io();
  close_elev_door();
  call_elev_to_floor();
  update_state();
  control_speed();
}


void auto_reset_io()
{
  if(onProcess)
  {
    count_reset_io = 0;
    return;
  }
  if(digitalRead(op_opendoor))
  {
    count_reset_io += 1;
  }
  else
  {
    count_reset_io = 0;
  }
  if(count_reset_io > 300 && !onProcess)
  {
    stateControl["msg"] = "Auto reset IO";
    reset_io();
  }
}


void control_speed()
{
  if(test)
  {
    if(count_test == 5000)
    {
      stateControl["control_loop"] = ((sum_test*1.00)/count_test);
      sum_test = 0;
      count_test = 0;
    }
    test_time = micros();
  }
  else
  {
    sum_test += micros() - test_time;
    count_test +=1;
  }

  test = !test;
}

void update_io()
{
  unsigned long schedule = 1000;
  if((millis() - io_update) > schedule)
  {
    io_update = millis();
    auto_reset_io();
    DynamicJsonDocument sendJson(1024);
    sendJson["input"]["ip_door1"] = digitalRead(ip_door1) == 1;
    sendJson["input"]["ip_door2"] = digitalRead(ip_door2) == 1;
    sendJson["input"]["ip_maintain"] = digitalRead(ip_maintain) == 1;
    sendJson["input"]["ip_outofservice"] = digitalRead(ip_outofservice) == 1;
    
    sendJson["output"]["op_floor1"] = digitalRead(op_floor1) == 1;
    sendJson["output"]["op_floor2"] = digitalRead(op_floor2) == 1;
    sendJson["output"]["op_calltofloor1"] = digitalRead(op_calltofloor1) == 1;
    sendJson["output"]["op_calltofloor2"] = digitalRead(op_calltofloor2) == 1;
    sendJson["output"]["op_opendoor"] = digitalRead(op_opendoor) == 1;
    sendJson["output"]["op_closedoor"] = digitalRead(op_closedoor) == 1;

    String str;
    serializeJson(sendJson, str);
    mqtt_publish("io_status",str);
  }
}

int step_led_1 =0;
void update_state()
{
  unsigned long schedule = 200;
  if((millis() - state_update) > schedule)
  {
    state_update = millis();
    stateControl["processName"] = processName;
    stateControl["onProcess"] = onProcess;
    if(onProcess)
    {
      digitalWrite(blink_io,!digitalRead(blink_io));
      step_led_1 = 0;
    }
    else
    {
      if(step_led_1 > 6)
      {
        digitalWrite(blink_io,HIGH);
        step_led_1 = 0;
      }
      else
      {
        digitalWrite(blink_io,LOW);
        step_led_1 +=1;
      }
    }
    String str;
    serializeJson(stateControl, str);
    mqtt_publish("state",str);
  }
}

void init_state()
{
  clear_state();
  stateControl["control_loop"] = 0.00;
}
void clear_state()
{
  stateControl["cmd"] = "";
  stateControl["processName"] = "";
  stateControl["onProcess"] = false;
  stateControl["msg"] = "";
  stateControl["finished"] = false;
  stateControl["timeOut"] = false;
  stateControl["outofservice"] = false;

}


void ping_server()
{
  unsigned long schedule = 4000;
  if((millis() - schedule_ping) > schedule)
  {
    schedule_ping = millis();
    http.begin(serverPath.c_str());
    int httpResponseCode = http.GET();  
    if (httpResponseCode == 200) 
    {
      unsigned long respTime = millis() -  schedule_ping;
      if(respTime >300)
      {
        digitalWrite(wifi_io,LOW);
        delay(1000);
        ESP.restart();
      }
      mqtt_publish("response_time",String(respTime)+" ms");
      digitalWrite(wifi_io,HIGH);
    }
    else
    {
      delay(1000);
      ESP.restart();
    }
    http.end();
  }
}

void mqtt_callback(char* topic, byte* payload, unsigned int length) 
{
  payload[length] = '\0';
  String topic_str = topic, payload_str = (char*)payload;
  String topics [numParams];
  mqtt_topic_sub_generator(topics);
  if(topics[0] == topic_str)
  {
    
    if(onProcess)
    {
      Serial.print("Controller are in process ");
      Serial.println(processName);
    }
    else
    {
      last_state_time = millis();
      payload_str.toLowerCase();
      clear_state();
      if(payload_str == "dcb")
      {
        stateControl["cmd"] = payload_str;
        state_idx_close_door = 1;
      }
      else if(payload_str == "call:1" || 
              payload_str == "call;1" || 
              payload_str == "call1" ||
              payload_str == "c1")
      {
        elevator_call_floor = 1;
        state_idx_call_ele = 1;
        stateControl["cmd"] = payload_str;


      }
      else if(payload_str == "call:2" || 
              payload_str == "call;2" || 
              payload_str == "call2" ||
              payload_str == "c2")
      {
        elevator_call_floor = 2;
        state_idx_call_ele = 1;
        stateControl["cmd"] = payload_str;
      }
      else if(payload_str == "rst")
      {
        Serial.println("Reset controller");
        reset_io();
        ESP.restart();
        
      }
    }
  }
  else if(topics[1] == topic_str)
  {
    setName(payload_str);
  }
}

void mqtt_init()
{
  mqtt.setServer(MQTT_SERVER, MQTT_PORT);
  mqtt.setCallback(mqtt_callback);
  if (mqtt.connected() == false) 
  {
    Serial.println("MQTT connection... ");
    const char* id = WiFi.getHostname();
    if (mqtt.connect(id)) 
    {
      mqtt_subscribe();
    } 
    else 
    {
      Serial.println("failed");
      ESP.restart();
    }
  } 
}


void mqtt_subscribe()
{
  String params [numParams];
  mqtt_topic_sub_generator(params);
  for (int i = 0; i < numParams; i++)
  {
    String topic = params [i];
    mqtt.subscribe(topic.c_str());
  }
}


void mqtt_publish(String topic,String msg)
{
  String topic_sum = String() + MQTT_Prefix +"/"+topic;
  mqtt.publish(topic_sum.c_str(), msg.c_str());

}


void waitting_for_connection()
{
  Serial.println("WiFi connecting ");
  unsigned long lastMsg2 = millis();
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(200);
    Serial.print("."); 
    if (millis() - lastMsg2 > 10000) 
    {    
      ESP.restart();
    }
    digitalWrite(wifi_io,!digitalRead(wifi_io));
  }
}

void ota_init()
{
  // ArduinoOTA.setPort(3232);
  ArduinoOTA.setHostname(MQTT_Prefix.c_str());
  //ArduinoOTA.setPassword("admin");
  ArduinoOTA.setPasswordHash("baba1a748d75574422bce4127a8ede9f");

  
  // ส่วนของ OTA
  ArduinoOTA
    .onStart([]() {
      String type;          // ประเภทของ OTA ที่เข้ามา
      if (ArduinoOTA.getCommand() == U_FLASH)         // แบบ U_FLASH
        type = "sketch";
      else          // แบบ U_SPIFFS
        type = "filesystem";

      // NOTE: ถ้าใช้เป็นแบบ SPIFFS อาจใช้คำสั่ง SPIFFS.end()
      Serial.println("Start updating " + type);
      
    })
    .onEnd([]() {
      Serial.println("\nEnd");
      reset_io();
      digitalWrite(blink_io,LOW);
      digitalWrite(wifi_io,LOW);
      ESP.restart();
    })

    // เริ่มทำงาน (รับข้อมูลโปรแกรม) พร้อมแสดงความคืบหน้าทาง Serial Monitor
    .onProgress([](unsigned int progress, unsigned int total) {
      Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
       if(millis()-last_state_time > 200)
       {
        last_state_time = millis();
        digitalWrite(blink_io,!digitalRead(blink_io));
        digitalWrite(wifi_io,!digitalRead(blink_io));
       }
    })

    // แสดงข้อความต่างๆหากเกิด Error ขึ้น
    .onError([](ota_error_t error) {
      Serial.printf("Error[%u]: ", error);
      if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
      else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
      else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
      else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
      else if (error == OTA_END_ERROR) Serial.println("End Failed");
      ESP.restart();
    });

  ArduinoOTA.begin();
}

void init_io()
{
  pinMode(ip_door1, INPUT_PULLUP);
  pinMode(ip_door2, INPUT_PULLUP);
  pinMode(ip_maintain, INPUT_PULLUP);
  pinMode(ip_outofservice, INPUT_PULLUP);
  
  pinMode(op_floor1, OUTPUT);
  pinMode(op_floor2, OUTPUT);
  pinMode(op_calltofloor1, OUTPUT);
  pinMode(op_calltofloor2, OUTPUT);
  pinMode(op_opendoor, OUTPUT);
  pinMode(op_closedoor, OUTPUT);

  pinMode(blink_io, OUTPUT);
  pinMode(wifi_io, OUTPUT);
  digitalWrite(blink_io,LOW);
  digitalWrite(wifi_io,LOW);
  reset_io();
}

void reset_io()
{
  digitalWrite(op_floor1,LOW);
  digitalWrite(op_floor2,LOW);
  digitalWrite(op_calltofloor1,LOW);
  digitalWrite(op_calltofloor2,LOW);
  digitalWrite(op_opendoor,LOW);
  digitalWrite(op_closedoor,LOW);
}

int address = 0;
void setName(String Name) 
{
  if(Name == "")
  {
    return;
  }
  EEPROM.writeString(address, Name);
  EEPROM.commit();
  address = 0;
  delay(50);
  ESP.restart();
}

void initEEPROM() 
{
  if (!EEPROM.begin(1000)) 
  {
    delay(50);
    ESP.restart();
  }
  
  String eeprom_name  = EEPROM.readString(address);
  if(eeprom_name == "")
  {
    setName(MQTT_Prefix);
  }
  MQTT_Prefix = eeprom_name;
}


//close_door_function

void close_elev_door()
{
  if(state_idx_close_door==0)
    return;
  else
  {
    onProcess = true;
    processName = "Close door";


  
    
    bool is_abnormal = !digitalRead(ip_outofservice) || !digitalRead(ip_maintain);
    if(is_abnormal)
      state_idx_close_door = 98;
    switch(state_idx_close_door)
    {
      case 1:
        stateControl["msg"] = processName + " : IO Reset";
        Serial.println(processName + " : IO Reset");
        reset_io();
        last_state_time = millis();
        state_idx_close_door += 1;
        stateControl["msg"] = processName + " : Waitting 50 ms";
        Serial.println(processName + " : Waitting 50 ms");
        break;
      case 2:
        if(millis()-last_state_time > 100)
        {
          stateControl["msg"] = processName + " : Force button closedoor";
          Serial.println(processName + " : Force button closedoor");
          digitalWrite(op_closedoor,HIGH);
          last_state_time = millis();
          state_idx_close_door += 1;
          Serial.println(processName + " : Waitting 800 ms");
          stateControl["msg"] = processName + " : Waitting 800 ms";
        }
        break;
     case 3:
        if(millis()-last_state_time > 800)
        {
          reset_io();
          last_state_time = millis();
          state_idx_close_door = 0;
          Serial.println(processName + " : Finish Process");
          stateControl["msg"] = processName + " : Finish Process";
          onProcess = false;
          processName = "";
          stateControl["finished"] = true;
        } 
        break;
      case 98:
        Serial.println(processName + " : out of service or maintain case !!!");
        stateControl["msg"] = processName + " : out of service or maintain case !!!";
        reset_io();
        last_state_time = millis();
        state_idx_close_door = 0;
        onProcess = false;
        processName = "";
        stateControl["outofservice"] = true;
        break;
        
    }
  }
}

void call_elev_to_floor()
{
  if(state_idx_call_ele == 0)
    return;
  else
  {
    onProcess = true;
    processName = "Call elevator to floor "+String(elevator_call_floor);
    bool status_ele = true; 
    bool status_door_open = false; 

    bool is_abnormal = !digitalRead(ip_outofservice) || !digitalRead(ip_maintain);
    if(is_abnormal)
      state_idx_call_ele = 98;
    switch(state_idx_call_ele)
    {
      case 1:
        Serial.println(processName + " : IO Reset");
        stateControl["msg"] = processName + " : IO Reset";
        reset_io();
        last_state_time = millis();
        state_idx_call_ele += 1;
        break;
      case 2:
        if(millis()-last_state_time > 100)
        {
          if(elevator_call_floor == 1)
          {
            digitalWrite(op_calltofloor1,HIGH);
          }
          else
          {
            digitalWrite(op_calltofloor2,HIGH);
          }
          Serial.println(processName + " : Pushed button call to floor " + String(elevator_call_floor));
          stateControl["msg"] = processName + " : Pushed button call to floor " + String(elevator_call_floor);
          last_state_time = millis();
          state_idx_call_ele += 1;
        }
        break;
     case 3:
        if(millis()-last_state_time > 800)
        {
          reset_io();
          Serial.println(processName + " : Release button call to floor " + String(elevator_call_floor));
          last_state_time = millis();
          state_idx_call_ele += 1;
          Serial.println(processName + " : Waitting elevator process");
          stateControl["msg"] = processName + " : Waitting elevator process";
        } 
        break;
     case 4:
          if(millis()-last_state_time > 1000)
          {
            if(elevator_call_floor == 1)
            {
              status_ele = !digitalRead(ip_door1);
            }
            else
            {
              status_ele = !digitalRead(ip_door2);
            }
            if(status_ele)
            {
              digitalWrite(op_opendoor,HIGH);
              Serial.println(processName + " : Elevator process finish");
              stateControl["msg"] = processName + " : Finished";
              last_state_time = millis();
              state_idx_call_ele = 0;
              onProcess = false;
              processName = "";
              stateControl["finished"] = true;
            }
            else
            {
              if(millis()-last_state_time > 120000) //2 miniute
              {
                state_idx_call_ele = 99;
              }
            }
          }
         
          break;
//     case 5:
//          if(millis()-last_state_time > 1000)
//          {
//            if(elevator_call_floor == 1)
//            {
//              status_door_open = digitalRead(ip_door1);
//            }
//            else
//            {
//              status_door_open = digitalRead(ip_door2);
//            }
//            if(status_door_open)
//            {
//              //force open door
//              digitalWrite(op_opendoor,HIGH);
//            
//              Serial.println(processName + " : Elevator Door is openned");
//              Serial.println(processName + " : Finished");
//              stateControl["msg"] = processName + " : Finished";
//              last_state_time = millis();
//              state_idx_call_ele = 0;
//              onProcess = false;
//              processName = "";
//              stateControl["finished"] = true;
//            }
//            else
//            {
//              if(millis()-last_state_time > 10000) //10 sec
//              {
//                state_idx_close_door = 99;
//              }
//            }
//          }
//          break;
      case 98:
          Serial.println(processName + " : out of service or maintain case !!!");
          stateControl["msg"] = (processName + " : out of service or maintain case !!!");
          reset_io();
          last_state_time = millis();
          state_idx_call_ele = 0;
          onProcess = false;
          processName = "";
          stateControl["outofservice"] = true;
          break;
      case 99:
          Serial.println(processName + " : Process timeout");
          stateControl["msg"] = (processName + " : Process timeout");
          reset_io();
          last_state_time = millis();
          state_idx_call_ele = 0;
          onProcess = false;
          processName = "";
          stateControl["timeOut"] = true;
          break;
    }
  }
}



//const uint8_t Ix10[8] = { 39, 36, 35, 34, 21, 19, 18, 4 };
//const uint8_t Qx10[8] = { 13, 14, 27, 26, 25, 33, 32, 2 };
