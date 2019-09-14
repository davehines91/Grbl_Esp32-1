#include <arduino.h>
#include <EEPROM.h>
#include <driver/rmt.h>
#include <esp_task_wdt.h>
#include <freertos/task.h>
#include <Wire.h>
#include "config.h"
#include "cpu_map.h"


static xQueueHandle softwareInterruptQueue = NULL;
static TaskHandle_t softwareInterruptTaskHandle = 0;

static TaskHandle_t i2cCheckTaskHandle = 0;

//static xQueueHandle cartesianQueue = NULL;


volatile int serviceRequired = 0;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

#include "i2cTask.h"


#define STEPPERS_DISABLE_PIN GPIO_NUM_13
#define TEENSYWIRE 0x8

void cncTDisplay(const LcdMessage &message)
{
   //Serial.println("cncTDisplay");
   Wire.beginTransmission(TEENSYWIRE);  
   Wire.write(message.type);
   Wire.write(message.lineNumber);
   Wire.write(message.state);
   Wire.write((const uint8_t*)&message.rpm,2);
   Wire.write((const uint8_t*)message.text,20);
   auto error = Wire.endTransmission();
   if(error != 0){
      Serial.printf("cncTDisplay End %d\n",error);
   }
  //Serial.println("cncTDisplay End");
}

void i2cCheckTask(void *pvParameters)
{
 // setupOled();
    Wire.begin(IC2_SDA_PIN,IC2_SCL_PIN);//GPIO_NUM_16,GPIO_NUM_19);// blue, green  SDA_PIN, SCL_PIN
    Wire.setClock(100000);
 
  //create a queue to handle gpio event from isr
  softwareInterruptQueue = xQueueCreate(100,sizeof(LcdMessage));
   long lastUpdate=0;
   long lastSlaveUpdate=0;
   long now = 0;
   bool splashVisible = false;
  while(true) // run continuously
  {
    LcdMessage message;
      
    if(xQueueReceive(softwareInterruptQueue, &message, 10)) {
      lastUpdate = millis();
      int qSize = uxQueueMessagesWaiting(softwareInterruptQueue);
     //Serial.printf("-i2cCheckTask %s CR\n",message.text);
     
      cncTDisplay(message);
      splashVisible = false;     
    } 
     if(((now = millis())- lastUpdate )> 5000 && !splashVisible){
      //   void splashScreen();
         lastUpdate = now;
      //   splashScreen();
         splashVisible = true; 
    }
    if((now - lastSlaveUpdate) >500){
      Wire.requestFrom(TEENSYWIRE, 2);   // request 6 bytes from slave device #8
      while(Wire.available()) { // slave may send less than requested
         uint8_t c = Wire.read();   // receive a byte as character
         Serial.print('-');Serial.print(c,HEX);Serial.print(" ");        // print the character
      }
      lastSlaveUpdate = now;
      Serial.println();
    }
    if(splashVisible && ((now-lastUpdate) > 50000)){
    //  void  clearScreen();
   //   clearScreen();
    }
    vTaskDelay(1 / portTICK_RATE_MS);  // Yield to other tasks 
  }    // while(true)
} 
void i2c_init()
{
   Serial.printf("i2c_init\n");
  xTaskCreatePinnedToCore(  i2cCheckTask, "i2cCheckTask", 8192, NULL,10, &i2cCheckTaskHandle,1); 
}
void sendSoftwareInterrupt(uint32_t interruptNumber)
{
  xQueueSendFromISR(softwareInterruptQueue, &interruptNumber, NULL);
}

void sendTextToDisplay(char *msg,uint8_t lineNumber)
{
   //Serial.printf("sendTextToDisplay %d %s\n",lineNumber,msg);
  LcdMessage lmsg;
  lmsg.type= 0;
  lmsg.lineNumber = lineNumber <8 ? lineNumber : 7;
  strncpy(lmsg.text,msg,32);
  //Serial.printf("sendTextToDisplay 2  %d %s\n",lineNumber,lmsg.text);
  if(softwareInterruptQueue != nullptr){
   //Serial.printf("xQueueSendFromISR");
      xQueueSendFromISR(softwareInterruptQueue, &lmsg, NULL);
  }
}
extern int16_t globalSpeed;
void sendPositionToDisplay(float cartX, float cartY, float cartZ,uint8_t coord, uint8_t tool,uint8_t stat,int16_t rpm)
{
  LcdMessage lmsg;
  lmsg.type= 1;
  lmsg.lineNumber = 0;
  lmsg.state = stat;
  lmsg.rpm =  rpm;
  lmsg.currentPosn[0] = cartX;
  lmsg.currentPosn[1] = cartY;
  lmsg.currentPosn[2] = cartZ;
  lmsg.coordSys=coord;
  lmsg.tool=tool;
  
  if(softwareInterruptQueue != nullptr){
      xQueueSendFromISR(softwareInterruptQueue, &lmsg, NULL);
  }
}
