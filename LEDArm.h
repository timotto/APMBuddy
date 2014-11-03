#ifndef _APMLED_LEDARM_H
#define _APMLED_LEDARM_H

#include <Arduino.h>

class LEDArm {
public:
  LEDArm(uint32_t* ledBuffer, uint8_t ledCount, uint8_t offset, bool forward) : _ledCount(ledCount) 
  {
    // inner led is [0], outer [n]
    int i,j;
    
    _leds = (uint32_t**)malloc(sizeof(uint32_t*) * _ledCount);
    
    for(i=0;i<_ledCount;i++) 
    {
      // forwards / backwards
      if (forward) j = offset + i; else j = offset + ledCount - i - 1;
      _leds[i] = &(ledBuffer[j]);
      *_leds[i] = i * i;
    }
  }
  
  void write(int offset, uint32_t color)
  {
    if (offset >= _ledCount)return;
    *_leds[offset] = color;
  }

  void write(int offset, uint32_t* colors, int len)
  {
    if (offset >= _ledCount) return;
    if ((offset+len) > _ledCount) len = _ledCount - offset;
    
    int i;
    for(i=0;i<len;i++) 
    {
      *_leds[offset + i] = colors[i];
    }
  }
  
private:
  uint8_t _ledCount;
  uint32_t** _leds;
  
};


#endif

