#include <util/crc16.h>

#include "nrf_radio.h"

NrfRadio<> radio;

int32_t next_stat_micros;
uint32_t tx_bytes;
uint32_t rx_bytes;

void setup(){
  Serial.begin(115200);

  SPI.begin();
  SPI.setDataMode(SPI_MODE0);
  SPI.setClockDivider(SPI_CLOCK_DIV2);

  radio.Init();
  radio.Bind("dgl");
  radio.Connect("brg");
  Serial.println("Started.");
  
  next_stat_micros = micros();
  tx_bytes = 0;
  rx_bytes = 0;
}

void loop() {
  int32_t now = micros();
  
  radio.Poll();

  radio.Receive();

  byte buf[32];
  uint8_t len;
  if (len = radio.Read(buf, 32)) {
    rx_bytes += len;
    Serial.write(len);
    for (uint8_t i = 0; i < len; ++i) {
      Serial.write(buf[i]);
    }
  }

#if 0
  byte buf2[] = "1234567812345678123456781234567";
  if (radio.WriteOnAck(buf2, sizeof(buf2))) {
    tx_bytes += sizeof(buf2);
  }

  if ((now - next_stat_micros) >= 0) {
    next_stat_micros += 1000000;
    Serial.print("Bytes: ");
    Serial.print(tx_bytes);
    Serial.print(" ");
    Serial.println(rx_bytes);
    tx_bytes = 0;
    rx_bytes = 0;
  }
#endif
}
