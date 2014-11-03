#ifndef _MAVQOS_LINK_H
#define _MAVQOS_LINK_H

#define LINK_MSG_STAT_SLOTS  32
#define LINK_MSG_AVG_F       0.3f
#define LINK_TXBUFFSIZE      384
#define LINK_TX_BURST        32

class MsgStats {
public:
  MsgStats() : lastSeen(0), lastSent(0), avgRXDelay(0), length(0), used(false) {}
  uint32_t lastSeen;
  uint32_t lastSent;
  float avgRXDelay;
  float desiredTXDelay;
  uint8_t msgid;
  uint8_t length;
  bool used;
};

class Link {
public:
  Link(uint8_t id, Stream &stream) : _id(id), _msgrxb(0), _mismatch(0), _amismatch(0), _stream(stream), _airspeed(1200), txbufSize(0)
    {
      txbufStart = txbuf;
    }

  void begin(uint16_t airspeed) { _airspeed = airspeed; }
  bool read(mavlink_message_t *dst, mavlink_status_t* status);
  bool write(mavlink_message_t* src);
  bool loop();
  
private:
  bool pushMessageToStats(mavlink_message_t* src);
  void updateQutas();
  
  uint8_t _id;
  uint8_t _msgrxb;
  uint32_t _mismatch;
  uint32_t _amismatch;
  Stream &_stream;
  uint16_t _airspeed;
  MsgStats msgStats[LINK_MSG_STAT_SLOTS];

  uint8_t txbuf[LINK_TXBUFFSIZE];
  uint8_t *txbufStart;
  int txbufSize;
  
};

#endif

