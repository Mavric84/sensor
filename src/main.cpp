

#include <Arduino.h>

#include <WiFi.h>
//#include <AsyncTCP.h>
//#include <ESPAsyncWebServer.h>
//#include <AsyncElegantOTA.h>
#include "SPIFFS.h"
#include "SPI.h"
#include "RH_SX126x.h"
#include <RHMesh.h>
#include "EEPROM.h"
//#include <RHReliableDatagram.h>
//#include <RH_RF95.h>

String readFile(fs::FS &fs, const char * path){ //Чтение файла из spiffs
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path);
  if(!file || file.isDirectory()){
    Serial.println("- failed to open file for reading");
    return String();
  }
  String fileContent;
  while(file.available()){
    fileContent = file.readStringUntil('\n');
    break;     
  }
  return fileContent;
}

RH_SX126x radio (8, 3, 4, 5);

#define BRIDGE_ADDRESS 77562  // address of the bridge ( we send our data to, hopefully the bridge knows what to do with our data )
#define NODE_ADDRESS 2    // address of this node

#define TXINTERVAL 3000
unsigned long nextTxTime;
#define RH_MESH_MAX_MESSAGE_LEN 50

#define sens_led 19;
#define sens_key 18;
//RHMesh* manager = nullptr;
RHMesh manager(radio, NODE_ADDRESS);


int sens_id = 010001;


IPAddress localIP(192,168,5,10);
IPAddress localGateway(192,168,5,1);
IPAddress subnet(255, 255, 255, 0); //Маска


void writeFile(fs::FS &fs, const char * path, const char * message){ //Функция записи файла в spiffs
  Serial.printf("Writing file: %s\r\n", path);

  File file = fs.open(path, FILE_WRITE);
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- frite failed");
  }
}

void setup(void) {
  Serial.begin(115200);
  SPI.begin(10, 6, 7);

   if (!SPIFFS.begin(true)) {
    Serial.println("Ошибка монтирования SPIFFS");
  }
  Serial.println("SPIFFS смонтировали успешно");
/*if (!radio.init()){
  Serial.println("init.failed");
  }else{
    Serial.println("INIT SUCCESS");
  }
  radio.setFrequency(868.0);
*/
  RHMesh manager(radio, sens_id);

//RHMesh manager(radio, BRIDGE_ADDRESS);
//manager = new RHMesh(radio, NODE_ADDRESS);

  Serial.print(F("initializing node "));
  Serial.print(NODE_ADDRESS);
   if (!manager.init())
    {Serial.println(" init failed");} 
  else
    {Serial.println(" done");} 
    radio.setFrequency(868.0);



pinMode(18,OUTPUT);
  Serial.print("MOSI:");
  Serial.println(MOSI);
  Serial.print("MISO:");
  Serial.println(MISO);
  Serial.print("SCK:");
  Serial.println(SCK);
 
  

    Serial.println("Setting AP (Access Point)"); //Раздаем точку доступа
    WiFi.softAP("sensor", NULL); // Создаем открытую точку доступа с именем
    WiFi.softAPConfig(localIP, localGateway, subnet);
 
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP); 
    Serial.print("Sensor ID: ");
    Serial.println(readFile(SPIFFS, "/sens_id.txt"));
}

uint8_t data[] = "Hello World!";
// Dont put this on the stack:
uint8_t buf[RH_MESH_MAX_MESSAGE_LEN];
uint8_t res;

void loop(void) {

// send message every TXINTERVAL millisecs
  if (millis() > nextTxTime)
    {
    nextTxTime += TXINTERVAL;
    Serial.print("Sending to bridge n.");
    Serial.print(BRIDGE_ADDRESS);
    Serial.print(" res=");
    
    // Send a message to a rf95_mesh_server
    // A route to the destination will be automatically discovered.
    res = manager.sendtoWait(data, sizeof(data), BRIDGE_ADDRESS);
    Serial.println(res);
    if (res == RH_ROUTER_ERROR_NONE)
      {
      // Data has been reliably delivered to the next node.
      // now we do... 
      }
    else
      {
      // Data not delivered to the next node.
      Serial.println("sendtoWait failed. Are the bridge/intermediate mesh nodes running?");
      }
    }
  
  // radio needs to stay always in receive mode ( to process/forward messages )
  uint8_t len = sizeof(buf);
  uint8_t from;
  if (manager.recvfromAck(buf, &len, &from))
    {
    Serial.print("message from node n.");
    Serial.print(from);
    Serial.print(": ");
    Serial.print((char*)buf);
    Serial.print(" rssi: ");
    Serial.println(radio.lastRssi()); 
    }  
}