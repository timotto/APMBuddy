/* MAVQoS
 * 
 * Put a Teensy 3.1 between your Telemetry port and two different
 * Telemetry devices plus an OSD at fast baudrates.
 *
 * The OSD shares the RX line with Serial1 of the Teensy 3.1
 * TX CTS RTS ar connected just between Telemetry port and Teensy.
 * A 3DR Radio is connected to Serial2 including the flow control
 * pins.
 * An XBee Pro 868 Module is connected to Serial3 including CTS/RTS.
 *
 * All devices have different air speeds. The OSD might process the
 * highest possible baudrate your MAVLink device is capable of, but
 * at least the XBee Pro is slower.
 *
 * Telemetry links should be connected with the highest possible baudrate 
 * to reduce the time spent to exchange information.
 *
 *
 * This Sketch is a MAVLink packet forwarder that handles the different speeds
 * of those multiple bidirectional links on the local TX side. To achieve this
 * every link has to be configured with the expected air speed baudrate.
 * The packets coming from the local APM are measured by their frequency and
 * size. Packets of the same type are only forwarded on a link if the quota
 * for that packet class is not exhausted. The quotas are equally distributed 
 * among all MAVLink message types, but some message types as the heart beat
 * message are more equal than the others.
 */

#include <EEPROM.h>
#include <GCS_MAVLink.h>
#include "defaults.h"
#include "Link.h"
#include "Leds.h"

#include <HardwareSerial.h>

Link downlink(0, Serial1);
Link uplink1(1, Serial2);
Link uplink2(2, Serial3);

bool uavStandby = true;
float battAmpsMax = 40.0f;
uint32_t camLightDimDelay = 3000;
uint32_t ledColorBg = 0x010000;

int selectedFX = FX_BLINKSLO;
bool flashOn = false;
bool camLightOn = false;
float battPerc = 1;
float battAmps = 0;

#define UART_MODEM_RXRTSE   (uint8_t)0x08
#define UART_MODEM_TXRTSPOL   (uint8_t)0x04
#define UART_MODEM_TXRTSE   (uint8_t)0x02
#define UART_MODEM_TXCTSE   (uint8_t)0x01

void setup() {
  // USB setup interface
  Serial.begin(115200);
  
  led_setup();
  
  readEEPROM();
  Serial1.begin(hwBaudSerial1);
  Serial2.begin(hwBaudSerial2);
  Serial3.begin(hwBaudSerial3);
//  UART0_MODEM = UART_MODEM_RXRTSE | UART_MODEM_TXCTSE;
//  UART1_MODEM = UART_MODEM_RXRTSE | UART_MODEM_TXCTSE;
//  UART2_MODEM = UART_MODEM_RXRTSE | UART_MODEM_TXCTSE;

  downlink.begin(airBaudSerial1);
  uplink1.begin(airBaudSerial2);
  uplink1.begin(airBaudSerial3);
}

mavlink_message_t msg;
mavlink_status_t status;
void loop() {
  while(downlink.read(&msg, &status)) {
    on_rx(0, &msg);
    handle_downlink(&msg);
    if(uplink1.write(&msg))
      on_tx(1, &msg);
    if(uplink2.write(&msg))
      on_tx(2, &msg);
  }
  
  while(uplink1.read(&msg, &status)) {
    on_rx(1, &msg);
    if (is_duplicate(&msg)) {
      on_duplicate(1, &msg);
      continue;
    }
    handle_uplink(&msg);
    if(downlink.write(&msg))
      on_tx(0, &msg);
  }
  
  while(uplink2.read(&msg, &status)) {
    on_rx(2, &msg);
    if (is_duplicate(&msg)) {
      on_duplicate(2, &msg);
      continue;
    }
    handle_uplink(&msg);
    if(downlink.write(&msg))
      on_tx(0, &msg);
  }

  while(Serial.available())
    consolePut(Serial.read());
    
  downlink.loop();
  uplink1.loop();
  uplink2.loop();
  
  loop_led();
}

void readEEPROM() {
  uint16_t u16;
  
  u16 = readeeuint16(0);
  if (u16 != ('M' << 8 | 'Q'))
    return; // no magic
  hwBaudSerial1 = readeeuint16(2);
  hwBaudSerial2 = readeeuint16(4);
  hwBaudSerial3 = readeeuint16(6);
}

void writeEEPROM() {
  EEPROM.write(0, 'M');
  EEPROM.write(1, 'Q');
  EEPROM.write(2, hwBaudSerial1);
  EEPROM.write(4, hwBaudSerial2);
  EEPROM.write(6, hwBaudSerial3);
}

void writeeeuint16(int a, uint16_t v) {
  EEPROM.write(a, (v >> 8) & 0xff);
  EEPROM.write(a+1, v&0xff);
}

uint16_t readeeuint16(int a) {
  uint16_t ret = 0;
  
  ret = EEPROM.read(a);
  ret <<= 8;
  ret |= EEPROM.read(a);
  return ret;
}


