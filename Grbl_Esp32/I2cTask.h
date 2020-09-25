#pragma once
void sendSoftwareInterrupt(uint32_t interruptNumber);
void sendTextToDisplay(char *msg,uint8_t lineNumber=3);
void sendPositionToDisplay(float cartX, float cartY, float cartZ,uint8_t coord, uint8_t tool,uint8_t stat,int16_t rpm);
void i2c_init();
struct LcdMessage{
  uint8_t type;
  uint8_t lineNumber;
  uint8_t state;
  uint16_t rpm;
  union{
    char text[64];
    struct{
      float currentPosn[3];
      uint8_t coordSys;
      uint8_t tool;
    };
 };  

};
