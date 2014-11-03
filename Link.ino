#include "Link.h"
#include <GCS_MAVLink.h>

bool Link::read(mavlink_message_t* dst, mavlink_status_t* status) {
  char c;
  int r = 0;
  while( r++ < 8 && _stream.available() ) {
    c = _stream.read();
    _msgrxb++;
    if (mavlink_parse_char(_id, c, dst, status)) {
    int16_t d = (int16_t)_msgrxb - (int16_t)(MAVLINK_NUM_NON_PAYLOAD_BYTES + dst->len);
    if (d != 0) {
      _mismatch += d;
      _amismatch += abs(d);
#ifdef DEBUG
      DEBUG.print("bc mm: ");
      DEBUG.print(_amismatch);
      DEBUG.print("/");
      DEBUG.print(_mismatch);
      DEBUG.print("/");
      DEBUG.println(d);
#endif
      }
      _msgrxb = 0;
      return true;
    }
    
  }
  return false;
}

bool Link::write(mavlink_message_t* src) {
//  if (!pushMessageToStats(src))
//    return false;
    
  // TODO CTS/RTS
  // TODO don't send more then X bytes at a time
  uint8_t buf[mavlink_msg_get_send_buffer_length(src)];
  uint16_t len = mavlink_msg_to_send_buffer(buf, src);
  _stream.write(buf, len);
  
  // TODO return false if the message was not sent (completely). The caller must call again with the same message (unless a newer message arrives).
  // TODO the stack remembers the message and the send position and tries to complete the failing message on the bus. 
  //      this allows for fast & tight loops at the expense of some overhead of tx buffer management. might reduce packet loss. (how high is it?)
  return true;
}

bool Link::pushMessageToStats(mavlink_message_t* src) {
  uint32_t now = millis();
  int i;
  
  for(i=0;i<LINK_MSG_STAT_SLOTS;i++) {
    if (msgStats[i].used && msgStats[i].msgid == src->msgid) {
      float currentDelay = now - msgStats[i].lastSeen;
      msgStats[i].avgRXDelay = (1.0f - LINK_MSG_AVG_F) * msgStats[i].avgRXDelay + LINK_MSG_AVG_F * currentDelay;
      updateQutas();
      
      if (msgStats[i].lastSent + msgStats[i].desiredTXDelay <= now) {
        msgStats[i].lastSent = now;
        return true;
      }
      return false;
    }
  }
  
  uint32_t oldestTime = now;
  int oldestSlot = 0;
  int unusedSlot = -1;
  
  
  for(i=0;i<LINK_MSG_STAT_SLOTS;i++) {
    if (!msgStats[i].used) {
      unusedSlot = i;
      break;
    }
    
    if (msgStats[i].lastSeen < oldestTime) {
      oldestTime = msgStats[i].lastSeen;
      oldestSlot = i;
    }
  }

  if (unusedSlot > -1) {
    oldestSlot = unusedSlot;
  }
  
  msgStats[oldestSlot].used = true;
  msgStats[oldestSlot].lastSeen = now;
  msgStats[oldestSlot].lastSent = now;
  msgStats[oldestSlot].avgRXDelay = 1000;
  msgStats[oldestSlot].desiredTXDelay = 1000;
  msgStats[oldestSlot].msgid = src->msgid;
  msgStats[oldestSlot].length = src->len + 8;
  updateQutas();
  return true;
}

void Link::updateQutas() {
  int i;
  float totalBytesPerSecond = 0;
  
  for(i=0;i<LINK_MSG_STAT_SLOTS;i++) {
    if (!msgStats[i].used)
      continue;
    
    float bps = msgStats[i].length * 1000.0f / msgStats[i].avgRXDelay;
    totalBytesPerSecond += bps;
  }
  
  if (totalBytesPerSecond <= _airspeed) {
    for(i=0;i<LINK_MSG_STAT_SLOTS;i++) {
      if (msgStats[i].used)
        msgStats[i].desiredTXDelay = msgStats[i].avgRXDelay;
    }
  } else {
    float f = (float)_airspeed / totalBytesPerSecond;
    
    for(i=0;i<LINK_MSG_STAT_SLOTS;i++) {
      if (msgStats[i].used)
        msgStats[i].desiredTXDelay = msgStats[i].avgRXDelay / f;
    }
  }
}


