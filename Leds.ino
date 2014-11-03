#include <FastLED.h>
#include "Leds.h"

#define STRIP_LENGTH  24

#include "LEDArm.h"

#define ARM_LEDS  6

int pwmLightLevel = 16;

uint32_t camLightOnTime = 0;
uint16_t whites[2][ARM_LEDS];
uint32_t armStripRightFront[ARM_LEDS];
uint32_t armStripLeftFront[ARM_LEDS];
uint32_t armStripRightRear[ARM_LEDS];
uint32_t armStripLeftRear[ARM_LEDS];

CRGB leds[STRIP_LENGTH];
static uint32_t strip_colors[STRIP_LENGTH];
LEDArm ledArmRightRear(strip_colors, ARM_LEDS, 0, true);
LEDArm ledArmRightFront(strip_colors, ARM_LEDS, 6, false);
LEDArm ledArmLeftFront(strip_colors, ARM_LEDS, 12, true);
LEDArm ledArmLeftRear(strip_colors, ARM_LEDS, 18, false);

#ifdef FX_DEMO
uint32_t nextFXChange = 15000;
uint32_t nextCamLightChange = 8*15000;
#endif

uint32_t nextLEDUpdate = 0;
uint32_t nextFlashLeft = 7000;
uint32_t nextFlashRight = 7000;
int flashStepLeft = 0;
int flashStepRight = 0;

int wpos = 0;
int wpos2 = 0;


void led_setup() {
//  pinMode(LED_CKI, OUTPUT);
//  pinMode(LED_SDI, OUTPUT);
  pinMode(PWMLED1, OUTPUT);
  pinMode(PWMLED2, OUTPUT);
  pinMode(PWMLED3, OUTPUT);
  pinMode(PWMLED4, OUTPUT);
  
  FastLED.addLeds<WS2801, RGB>(leds, STRIP_LENGTH);
  post_frame(); //Push the current color frame to the strip
}

uint32_t nextPwmFlash = 0;
int pwmFlashState = 0;

void loop_led()
{
  uint32_t now = millis();
  
//#ifdef FX_DEMO
//  if (nextCamLightChange <= now) {
//    camLightOn = !camLightOn;
//    nextCamLightChange = now + 8 * 15000;
//  }
//  
//  if (nextFXChange <= now) {
//    selectedFX++;
//    if (selectedFX > FX_KRSCAN)
//      selectedFX = 0;
//    if (selectedFX == FX_BATT_V) {
//      battPerc -= 0.2f;
//      if (battPerc < 0.0f)
//        battPerc = 1.0f;
//      Serial.print("battPerc: ");
//      Serial.println(battPerc);
//    }
//    if (selectedFX == FX_BATT_A) {
//      battAmps += 6;
//      if (battAmps > 40.0f)
//        battAmps = 0.0f;
//      Serial.print("battAmps: ");
//      Serial.println(battAmps);
//    }
//    nextFXChange = now + 15000;
//  }
//#endif

  if (nextLEDUpdate <= now) {
    
    loop_fx();
    loop_flashes(now);
    loop_camlight(now);
    loop_pwm(now);
    
    ledArmLeftRear.write(0, armStripLeftRear, ARM_LEDS);
    ledArmRightRear.write(0, armStripRightRear, ARM_LEDS);
    ledArmLeftFront.write(0, armStripLeftFront, ARM_LEDS);
    ledArmRightFront.write(0, armStripRightFront, ARM_LEDS);

    post_frame();
    
    nextLEDUpdate = millis() + 50;
  }
}

#define PWM_MAX       240
#define PWM_MIN       16
#define PWM_DIM_TIME  10000

bool pwmWasBright = false;
uint32_t pwmStartDim = 0 - PWM_DIM_TIME;

void loop_pwm(uint32_t now)
{
  if (flashOn) {
    if (!pwmWasBright) {
      pwmStartDim = now;
    }
    if (pwmStartDim + PWM_DIM_TIME >= now) {
      pwmLightLevel = PWM_MIN + (int) ((float)(PWM_MAX-PWM_MIN) * (float)(now - pwmStartDim) / (float)(PWM_DIM_TIME));
    } else 
      pwmLightLevel = PWM_MAX;
      
//    if (nextPwmFlash <= now) {
//      pwmflash(pwmFlashState, PWMLED1);
//      pwmflash(pwmFlashState - 4, PWMLED2);
//      if (pwmFlashState++ > 16) {
//          nextPwmFlash = now + 4000;
//          pwmFlashState = 0;
//      }
//    }
  } else {
    if (pwmWasBright) {
      pwmStartDim = now;
    }
    if (pwmStartDim + PWM_DIM_TIME >= now) {
      pwmLightLevel = PWM_MIN + (int) ((float)(PWM_MAX-PWM_MIN) * (float)((PWM_DIM_TIME) - (now - pwmStartDim)) / (float)(PWM_DIM_TIME));
    } else 
      pwmLightLevel = PWM_MIN;
    
    analogWrite(PWMLED1, pwmLightLevel/2);
    analogWrite(PWMLED2, pwmLightLevel/2);
  }
  
  analogWrite(PWMLED3, pwmLightLevel);
  analogWrite(PWMLED4, pwmLightLevel);
  
  if (camLightOn) {
    analogWrite(PWMLED1, PWM_MAX);
    analogWrite(PWMLED2, PWM_MAX);
  }
  
  pwmWasBright = flashOn;
}

void pwmflash(int step, int port)
{
    switch(step) {
      case 0:
      case 2:
        analogWrite(port, pwmLightLevel);
        break;
      case 1:
      case 3:
        analogWrite(port, 0);
        break;
    }
}


void loop_camlight(uint32_t now) 
{
  int i,j;
  if (camLightOn) {
    if (camLightOnTime == 0) {
      // turned on right now
      camLightOnTime = now;
      for(i=0;i<2;i++) {
        for(j=0;j<ARM_LEDS;j++) {
          whites[i][j] = 0;
        }
      }
    } else if (camLightOnTime + camLightDimDelay > now) {
      // dimming up
      for(i=0;i<2;i++)
        for(j=0;j<ARM_LEDS;j++) {
          whites[i][j] += random(0,8);
          if (whites[i][j] > 255)whites[i][j] = 255;
        }
      
    } else {
      // full on
      for(i=0;i<2;i++)
        for(j=0;j<ARM_LEDS;j++)
          whites[i][j] = 0xff;
    }
    for(i=0;i<ARM_LEDS;i++) {
      armStripLeftFront[i] = whites[0][i] << 16 | whites[0][i] << 8 | whites[0][i];
      armStripRightFront[i] = whites[1][i] << 16 | whites[1][i] << 8 | whites[1][i];
      armStripLeftRear[i] = whites[0][i] << 16 | whites[0][i] << 8 | whites[0][i];
      armStripRightRear[i] = whites[1][i] << 16 | whites[1][i] << 8 | whites[1][i];
    }
    
  } else if (camLightOnTime > 0) {
    if (whites[0][0] == 0) {
      camLightOnTime = 0;
      return;
    }
    if (whites[0][0] > 8)
      whites[0][0] -= random(0, 8);
    else whites[0][0] = 0;
    
    for(i=0;i<ARM_LEDS;i++) {
      if (random(0,whites[0][0]) > 4)
        armStripLeftFront[i] = whites[0][0] << 16 | whites[0][0] << 8 | whites[0][0];
      if (random(0,whites[0][0]) > 4)
        armStripRightFront[i] = whites[0][0] << 16 | whites[0][0] << 8 | whites[0][0];
      if (random(0,whites[0][0]) > 4)
        armStripLeftRear[i] = whites[0][0] << 16 | whites[0][0] << 8 | whites[0][0];
      if (random(0,whites[0][0]) > 4)
        armStripRightRear[i] = whites[0][0] << 16 | whites[0][0] << 8 | whites[0][0];
    }
  }
}

void loop_fx() {
    int i,j,a,b;
    float f;
    
    switch(selectedFX) {
      case FX_NONE:
        for(i=0;i<ARM_LEDS;i++) {
          armStripLeftFront[i] = 
          armStripRightFront[i] = 
          armStripLeftRear[i] = 
          armStripRightRear[i] = ledColorBg;
        }
        break;
        
      case FX_MIN:
        arm_clear(armStripLeftFront);
        arm_clear(armStripLeftRear);
        arm_clear(armStripRightFront);
        arm_clear(armStripRightRear);
        armStripLeftFront[0] = LED_RED;
        armStripLeftRear[0] = LED_RED;
        armStripRightFront[0] = LED_GREEN;
        armStripRightRear[0] = LED_GREEN;
        break;
        
      case FX_FULL:
        for(i=0;i<ARM_LEDS;i++) {
          armStripLeftFront[i] = 
          armStripLeftRear[i] = LED_RED;
          armStripRightFront[i] = 
          armStripRightRear[i] = LED_GREEN;
        }
        break;
        
      case FX_GLOW:
        wpos = (wpos + 1) % 128;
        if (wpos < 64) {
          for(i=0;i<ARM_LEDS;i++) {
            armStripLeftFront[i] = 
            armStripLeftRear[i] = wpos + 2 + i * i;
            armStripRightFront[i] = 
            armStripRightRear[i] = (wpos + 2 + i * i) << 8;
          }
        } else {
          for(i=0;i<ARM_LEDS;i++) {
            armStripLeftFront[i] = 
            armStripLeftRear[i] = 64 - (wpos - 64) + 2 + i * i;
            armStripRightFront[i] = 
            armStripRightRear[i] = (64 - (wpos - 64) + 2 + i * i) << 8;
          }
        }
        break;
        
      case FX_BLINKSLO:
        wpos = (wpos + 1) % 20;
        for(i=0;i<ARM_LEDS;i++) {
          armStripLeftFront[i] = 
          armStripLeftRear[i] = wpos > 10 ? LED_RED : ledColorBg;
          armStripRightFront[i] = 
          armStripRightRear[i] = wpos > 10 ? LED_GREEN : ledColorBg;
        }
        break;
        
      case FX_BLINKHI:
        wpos = (wpos + 1) % 10;
        for(i=0;i<ARM_LEDS;i++) {
          armStripLeftFront[i] = 
          armStripLeftRear[i] = wpos > 5 ? LED_RED : ledColorBg;
          armStripRightFront[i] = 
          armStripRightRear[i] = wpos > 5 ? LED_GREEN : ledColorBg;
        }
        break;
        
      case FX_BATT_V:
        wpos = (int)( (float)(ARM_LEDS-1) * battPerc );
        if (wpos == 0) {
          for(i=0;i<ARM_LEDS;i++) {
            armStripLeftFront[i] = 
            armStripLeftRear[i] = 
            armStripRightFront[i] = 
            armStripRightRear[i] = (wpos2++%3==0)?LED_BLUE:LED_RED;
          }
          wpos2++;
        } else {
          for(i=0;i<ARM_LEDS;i++) {
            armStripLeftFront[i] = 
            armStripLeftRear[i] = 
            armStripRightFront[i] = 
            armStripRightRear[i] = i<wpos?LED_BLUE:ledColorBg;
          }
        }
        break;
        
      case FX_BATT_A:
        f = battAmps / battAmpsMax;
        if (f > 1.0f)f=1.0f;
        // f now 0..1 where 1=max amps
        
        f = ((1.0f - f) * 15) + 2;
        // when max amps, off time must be short
        wpos = (wpos + 1) % (int)(f);
        for(i=0;i<ARM_LEDS;i++) {
          armStripLeftFront[i] = 
          armStripLeftRear[i] = 
          armStripRightFront[i] = 
          armStripRightRear[i] = 0==wpos?LED_BLUE:ledColorBg;
        }
        break;
        
      case FX_WANDER:
        wpos = (wpos + 1) % ARM_LEDS;
        if (wpos % 3 == 0) {
          wpos2 = (wpos2 + 1) % ARM_LEDS;
        }
        for(i=0;i<ARM_LEDS;i++) {
          armStripLeftFront[i] = wpos == i ? LED_RED : ledColorBg;
          armStripRightFront[i] = wpos == i ? LED_GREEN : ledColorBg;
        }
        for(i=0;i<ARM_LEDS;i++) {
          armStripLeftRear[i] = wpos2 == i ? LED_RED : ledColorBg;
          armStripRightRear[i] = wpos2 == i ? LED_GREEN : ledColorBg;
        }
        break;
        
      case FX_DROPINV:
        wpos = (wpos + 1) % (ARM_LEDS * 2);
        
        arm_clear(armStripLeftFront);
        arm_clear(armStripLeftRear);
        arm_clear(armStripRightFront);
        arm_clear(armStripRightRear);

        a=(ARM_LEDS/2)-(wpos/2) - 1;
        if (a>=0 && a<ARM_LEDS) {
          armStripLeftFront[a] = 
          armStripLeftRear[a] = 
            LED_RED;
          armStripRightFront[a] = 
          armStripRightRear[a] = 
            LED_GREEN;
        }
        
        a=(ARM_LEDS/2)+(wpos/2) - 1;
        if (a>=0 && a<ARM_LEDS) {
          armStripLeftFront[a] = 
          armStripLeftRear[a] = 
            LED_RED;
          armStripRightFront[a] = 
          armStripRightRear[a] = 
            LED_GREEN;
        }

        break;
      
      case FX_DROP:
        wpos = (wpos + 1) % (ARM_LEDS*2);
        
        arm_clear(armStripLeftFront);
        arm_clear(armStripLeftRear);
        arm_clear(armStripRightFront);
        arm_clear(armStripRightRear);

        a=(ARM_LEDS/2)-(ARM_LEDS - 1 - (wpos/2)) - 1;
        if (a>=0 && a<ARM_LEDS) {
          armStripLeftFront[a] = 
          armStripLeftRear[a] = 
            LED_RED;
          armStripRightFront[a] = 
          armStripRightRear[a] = 
            LED_GREEN;
        }
        
        a=(ARM_LEDS/2)+(ARM_LEDS - 1 - (wpos/2)) - 1;
        if (a>=0 && a<ARM_LEDS) {
          armStripLeftFront[a] = 
          armStripLeftRear[a] = 
            LED_RED;
          armStripRightFront[a] = 
          armStripRightRear[a] = 
            LED_GREEN;
        }

        break;
        
      case FX_REDUCE:
        wpos = (wpos + 1) % (4*ARM_LEDS);
        for(i=0;i<ARM_LEDS;i++) {
          armStripLeftFront[ARM_LEDS-1-i] =
          armStripLeftRear[ARM_LEDS-1-i] = (i*4>=wpos?LED_RED:ledColorBg);
          
          armStripRightFront[ARM_LEDS-1-i] =
          armStripRightRear[ARM_LEDS-1-i] = (i*4>=wpos?LED_GREEN:ledColorBg);
        }
        break;
      
      case FX_KRSCAN:
        wpos = (wpos + 1) % (7*ARM_LEDS);
        
        for(i=0;i<ARM_LEDS;i++) {
          armStripLeftFront[i] = 
          armStripLeftRear[i] = 
          armStripRightFront[i] = 
          armStripRightRear[i] = ledColorBg;
        }

        for (i=1;1<=7;i++) {
          if (wpos < (i * ARM_LEDS)) {
            switch (i) {
              case 1:
                // right arm starts from outer to inner
                for(i=0;i<ARM_LEDS;i++) {
                  armStripRightFront[i] = 
                  armStripRightRear[i] = i==(ARM_LEDS - 1 - (wpos%ARM_LEDS))?LED_RED:ledColorBg;
                }
                break;
              case 2:
                // left arm starts inner to outer
                for(i=0;i<ARM_LEDS;i++) {
                  armStripLeftFront[i] = 
                  armStripLeftRear[i] = i==(wpos%ARM_LEDS)?LED_RED:ledColorBg;
                }
                break;
              case 3:
                // left arm starts outer to inner
                for(i=0;i<ARM_LEDS;i++) {
                  armStripLeftFront[i] = 
                  armStripLeftRear[i] = i==(ARM_LEDS - 1 - (wpos%ARM_LEDS))?LED_RED:ledColorBg;
                }
                break;
              case 4:
                // right arm starts inner to outer
                for(i=0;i<ARM_LEDS;i++) {
                  armStripRightFront[i] = 
                  armStripRightRear[i] = i==(wpos%ARM_LEDS)?LED_RED:ledColorBg;
                }
                break;
              default:
                for(i=0;i<ARM_LEDS;i++) {
                  armStripRightFront[i] = 
                  armStripRightRear[i] = ledColorBg;
                }
                break;
            }
            break;
          }
        }
        break;
    }

    armStripLeftFront[ARM_LEDS-1] = LED_RED;
    armStripLeftRear[ARM_LEDS-1] = LED_RED;
    armStripRightFront[ARM_LEDS-1] = LED_GREEN;
    armStripRightRear[ARM_LEDS-1] = LED_GREEN;
}

void arm_clear(uint32_t* arm) 
{
  int i=0;
  for(i=0;i<ARM_LEDS;i++)
    arm[i] = ledColorBg;
}

void loop_flashes(uint32_t now) {
  if (!flashOn)return;
  
  int i;
  
    if (nextFlashLeft <= now) {
      bool whiteOn = false;
      switch(flashStepLeft++) {
        case 0:
        case 2:
          whiteOn = true;
          break;
        case 3:
          nextFlashLeft = now + 3456;
          flashStepLeft = 0;
        case 1:
          whiteOn = false;
          break;
      }
      
      for(i=0;i<ARM_LEDS;i++)
        armStripLeftRear[i] = 
        armStripLeftFront[i] = 
//          armStripRightRear[i] = 
//        armStripLeftFront[i] = 
//          armStripRightFront[i] = 
          whiteOn?LED_WHITE:ledColorBg;
      analogWrite(PWMLED2, whiteOn?pwmLightLevel:0);
    }




    if (nextFlashRight <= now) {
      bool whiteOn = false;
      switch(flashStepRight++) {
        case 0:
        case 2:
          whiteOn = true;
          break;
        case 3:
          nextFlashRight = now + 3567;
          flashStepRight = 0;
        case 1:
          whiteOn = false;
          break;
      }
      
      for(i=0;i<ARM_LEDS;i++)
//        armStripLeftRear[i] = 
          armStripRightRear[i] = 
          armStripRightFront[i] = 
//        armStripLeftFront[i] = 
//          armStripRightFront[i] = 
          whiteOn?LED_WHITE:ledColorBg;
      analogWrite(PWMLED1, whiteOn?pwmLightLevel:0);
      
    }
}

void post_frame(void) {
  int i;
  for(i=0;i<STRIP_LENGTH;i++)
    leds[i] = CRGB(strip_colors[i]);
  FastLED.show();
}

