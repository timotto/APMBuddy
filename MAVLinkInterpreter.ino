#define MAV_DUP_LIST     64
#define MAV_DUP_MAXMS  2000

// TODO interpret rc channel x as switch for LED modes: batt(amps/votl/percent rmaining), mode, white lights only

// messages from GCS to UAV
void handle_uplink(mavlink_message_t* msg) 
{
//  Serial.print("on_uplink: ");
//  Serial.println(msg->msgid);
}

#define DISPLAY_TOGGLE_DELAY  2000
uint32_t nextDisplayToggle = 0;
int currentDisplay = -1;

uint16_t lastCh12 = 0;

void handle_rc_channels(mavlink_message_t* msg)
{
  uint16_t ch12;
  // if ch12 > 1700 toggle lights
  if ((ch12=mavlink_msg_rc_channels_get_chan12_raw(msg)) >= 1750) {
    if (lastCh12 >= 1750)
      return;
    
    camLightOn = !camLightOn;
  } else {
    if (lastCh12 < 1750)
      return;
  }
  lastCh12 = ch12;
}

// messages from UAV to GCS
void handle_downlink(mavlink_message_t* msg)
{
//  Serial.print("on_downlink: ");
//  Serial.println(msg->msgid);
  uint8_t b;
  uint16_t c;
  
  switch(msg->msgid) {
    case MAVLINK_MSG_ID_RC_CHANNELS_SCALED:
      break;
    case MAVLINK_MSG_ID_RC_CHANNELS_RAW:
      break;
    case MAVLINK_MSG_ID_RC_CHANNELS:
      handle_rc_channels(msg);
      break;
    case MAVLINK_MSG_ID_BATTERY_STATUS:
      b = mavlink_msg_battery_status_get_battery_remaining(msg);
      if (b > -1)
        battPerc = ((float)b / 100.0f);
      c = mavlink_msg_battery_status_get_current_battery(msg);
      if (c > -1)
        battAmps = ((float)c / 100);
        
      Serial.print("battperc: ");Serial.println(battPerc);
      break;
    case MAVLINK_MSG_ID_HEARTBEAT:
      flashOn = mavlink_msg_heartbeat_get_base_mode(msg) & MAV_MODE_FLAG_SAFETY_ARMED;
      
      // acro  209/1
      // guided 217/4
      // loiter 217/5
      // circle 217/7
      // land 209/9
      // drift 209/11
      // sport 209/13
      
      switch (mavlink_msg_heartbeat_get_custom_mode(msg)) {
        case 1: // acro
          selectedFX = FX_FULL;
          break;
        case 0: // base 81 stabelize
          selectedFX = FX_GLOW;
          break;
        case 2: // base 81 alt holt
          selectedFX = FX_DROPINV;
          break;
        case 3: // base 89 auto
          selectedFX = FX_WANDER;
          break;
        case 4: // guided
          selectedFX = FX_WANDER;
          break;
        case 5: // loiter
          selectedFX = FX_MIN;
          break;
        case 6: // base 89 rtl
          selectedFX = FX_KRSCAN;
          break;
        case 9: // land
          selectedFX = FX_REDUCE;
          break;
        case 16: //base 89 pos hold
          selectedFX = FX_DROP;
          break;
        default:
          Serial.print("unk custmode, base: ");
          Serial.print(mavlink_msg_heartbeat_get_base_mode(msg));
          Serial.print(" cust: ");
          Serial.println(mavlink_msg_heartbeat_get_custom_mode(msg));
          break;
      }
      
      uint32_t now = millis();
      switch(mavlink_msg_heartbeat_get_system_status(msg)) {
        case MAV_STATE_CRITICAL:
          currentDisplay = 0;
//          selectedFX = FX_BLINKHI;
          ledColorBg = 0x2f002f;
//          if (nextDisplayToggle < now) {
//            currentDisplay = (currentDisplay + 1) % 2;
//            nextDisplayToggle = now + (currentDisplay==0?4:1) * DISPLAY_TOGGLE_DELAY;
//          }
          break;
        case MAV_STATE_STANDBY:
          // toggle led displays of mode/batt percent
          ledColorBg = 0x000300;
          if (nextDisplayToggle < now) {
            currentDisplay = (currentDisplay + 1) % 2;
            nextDisplayToggle = now + (currentDisplay==0?4:1) * DISPLAY_TOGGLE_DELAY;
          }
          break;
        case MAV_STATE_ACTIVE:
          // toggle led displays of mode/batt percent/batt amps
          ledColorBg = 0x000000;
          if (nextDisplayToggle < now) {
            currentDisplay = (currentDisplay + 1) % 3;
            nextDisplayToggle = now + (currentDisplay==0?3:1) * DISPLAY_TOGGLE_DELAY;
          }
          break;
        default:
          currentDisplay = 0;
          ledColorBg = 0x000303;
          break;
      }
//      switch(currentDisplay) {
//        case 1:
//          selectedFX = FX_BATT_V;
//          break;
//        case 2:
//          selectedFX = FX_BATT_A;
//          break;
//        default:
//          break;
//      }
      break;
  }
}

class MavlinkMessageSigSlot {
public:
  MavlinkMessageSigSlot() : used(false), timestamp(0), sig(0) {}
  bool used;
  uint32_t timestamp;
  uint32_t sig;
};

MavlinkMessageSigSlot sigSlots[MAV_DUP_LIST];

// detect duplicate messages because of multiple telemetry links
bool is_duplicate(mavlink_message_t* msg)
{
  return false;
}

bool is_duplicateREAL(mavlink_message_t* msg)
{
  uint32_t msgSig = (((uint32_t)msg->sysid)<<24 |
                     ((uint32_t)msg->compid)<<16 |
                     ((uint32_t)msg->msgid)<<8 |
                     ((uint32_t)msg->seq)) ^ ( ((uint32_t)msg->checksum) << 16 | (uint32_t)msg->checksum );

  uint32_t now = millis();
  
  bool dup = false;
  int free = -1;
  int i;
  for(i=0;i<MAV_DUP_LIST;i++) {
    
    // only used
    if (!sigSlots[i].used) {
      // remember first free slot for later
      if (free == -1)
        free = i;
      continue;
    }
      
    // not too old
    if (sigSlots[i].timestamp + MAV_DUP_MAXMS < now) {
      sigSlots[i].used = false;
      // remember first free slot for later
      if (free == -1)
        free = i;
      continue;
    }
    
    if (msgSig == sigSlots[i].sig) {
      dup = true;
      break;
    }
  }
  
  // done if duplicate
  if (dup)return true;
  
  // no duplicate, remember
  
  // if no free slot was found
  if (free == -1) {
    uint32_t otime = now;
    int oslot = 0;
    for(i=0;i<MAV_DUP_LIST;i++) {
      if (!sigSlots[i].used) {
        oslot = i;
        break;
      }
      
      if (sigSlots[i].timestamp <= otime) {
        otime = sigSlots[i].timestamp;
        oslot = i;
      }
    }
    free = oslot;
  }
  
  sigSlots[free].used = true;
  sigSlots[free].timestamp = now;
  sigSlots[free].sig = msgSig;
  
  return false;
}

