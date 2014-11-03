/*
 > help
 ERROR
 > set 2.baud.uart=57600
 OK
 set 2.baud.air=32000
 >
 OK
 > get 3.baud.uart
 57600
 OK
 > 
 
*/

#include "consolecommands.h"

#define DEBUG Serial

#define CONSOLE_LINE_MAX    32

char consoleLineBuf[CONSOLE_LINE_MAX];
char consoleLineBufPos = 0;

void consolePut(char c) {
  consoleLineBuf[consoleLineBufPos++] = c;

  if (c == '\n' || c == '\r') {
    if (consoleLineBufPos < 4) {
      consoleError();
      return;
    }
    if (strncmp(CONS_COM_SET, consoleLineBuf, CONSOLE_LINE_MAX) == 0) {
      uint8_t lineId = '0' + consoleLineBuf[4];
    }
  }
  
  if (consoleLineBufPos >= CONSOLE_LINE_MAX) {
    Serial.print("ERROR: input overflow\n> ");
    consoleLineBufPos = 0;
  }
  
}

void consoleError() {
  Serial.print("ERROR\n> ");
  consoleLineBufPos = 0;
}

