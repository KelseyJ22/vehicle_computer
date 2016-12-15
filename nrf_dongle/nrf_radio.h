/*
  Copyright (c) 2015 Jeffrey Rogers <sgtnoodle@gmail.com>

  Permission is hereby granted, free of charge, to any person 
  obtaining a copy of this software and associated documentation 
  files (the "Software"), to deal in the Software without 
  restriction, including without limitation the rights to use, copy, 
  modify, merge, publish, distribute, sublicense, and/or sell copies 
  of the Software, and to permit persons to whom the Software is 
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be 
  included in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, 
  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF 
  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND 
  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT 
  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, 
  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, 
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER 
  DEALINGS IN THE SOFTWARE.
*/

#ifndef NRF_RADIO_H
#define NRF_RADIO_H

#include "SPI.h"

#include "nRF24L01.h"

template <int ce_pin = 8, int csn_pin = 7>
class NrfRadio {
 public:
  NrfRadio() {
  }

  void Init() {
    pinMode(ce_pin, OUTPUT);
    pinMode(csn_pin, OUTPUT);
    SetCe<false>();
    SetCsn<true>();

    WriteRegister<RF_CH>(10);
    WriteRegister<CONFIG>((1<<EN_CRC) | (1<<CRCO));
    ModifyRegister<CONFIG>((1<<PWR_UP) | (1<<PRIM_RX),
                           (1<<PWR_UP) | (1<<PRIM_RX));
    receiving_ = true;
    delay(5);

    // Set up for ack payloads.
    WriteRegister<SETUP_AW>(1<<AW);
    WriteRegister<FEATURE>((1 << EN_ACK_PAY) | (1 << EN_DPL) | (1 << EN_DYN_ACK));
    WriteRegister<DYNPD>((1<<DPL_P0) | (1<<DPL_P1));
    WriteRegister<EN_AA>((1<<ENAA_P0) | (1<<ENAA_P1));
    WriteRegister<SETUP_RETR>((0x1<<ARD) | (0xFF<<ARC));

    SetCe<true>();
    WriteRegister<STATUS>((1 << TX_DS) | (1 << MAX_RT));
    Command<FLUSH_RX>();
    Command<FLUSH_TX>();
  }

  void Bind(char *addr) {
    byte address[5] = {0,0,0,0,0};
    uint8_t len = strlen(addr);
    for (char i = 0; i < (len > sizeof(address) ? sizeof(address) : len); ++i) {
      address[i] = addr[i];
    }
    WriteRegister<RX_ADDR_P1>(address, sizeof(address));
    Command<FLUSH_RX>();
  }

  void Connect(char *addr) {
    byte address[5] = {0,0,0,0,0};
    uint8_t len = strlen(addr);
    for (char i = 0; i < (len > sizeof(address) ? sizeof(address) : len); ++i) {
      address[i] = addr[i];
    }
    WriteRegister<RX_ADDR_P0>(address, sizeof(address));
    WriteRegister<TX_ADDR>(address, sizeof(address));
  }

  void Poll() {
    if (!receiving_) {
      if (TxEmpty()) {
        SetCe<false>();
      }
    }
    WriteRegister<STATUS>((1 << TX_DS));
  }
  
  bool CheckIfJammed() {
    uint8_t stat = ReadRegister<STATUS>();
    if (stat & (1 << MAX_RT)) {    
      WriteRegister<STATUS>((1 << MAX_RT));
      return TxFull();
    }
    return false;
  }

  void Flush() {
    Command<FLUSH_TX>();
  }

  bool TxFull() const {
    return ReadRegister<FIFO_STATUS>() & (1 << FIFO_FULL);
  }

  bool Write(uint8_t *buf, int len) {
    if (TxFull()) {
      return false;
    }
    WriteBuffer<W_TX_PAYLOAD>(buf, len < 32 ? len : 32);
    return true;
  }

  bool WriteNoAck(uint8_t *buf, int len) {
    if (TxFull()) {
      return false;
    }
    WriteBuffer<W_TX_PAYLOAD_NOACK>(buf, len < 32 ? len : 32);
    return true;
  }

  bool WriteOnAck(uint8_t *buf, int len) {
    if (TxFull()) {
      return false;
    }
    WriteBuffer<W_ACK_PAYLOAD_1>(buf, len < 32 ? len : 32);
    return true;
  }

  bool TxEmpty() const {
    return ReadRegister<FIFO_STATUS>() & (1 << TX_EMPTY);
  }

  void Transmit() {
    if (receiving_) {
      SetCe<false>();
      ModifyRegister<CONFIG>((0<<PRIM_RX), (1<<PRIM_RX));
      receiving_ = false;
    }
    SetCe<true>();
  }

  bool RxEmpty() const {
    return ReadRegister<FIFO_STATUS>() & (1 << RX_EMPTY);
  }

  uint8_t Read(uint8_t *buf, int len) {
    if (RxEmpty()) {
      return 0;
    }
    uint8_t msglen = ReadCommand<R_RX_PL_WID>();
    if (msglen > 32) {
      Command<FLUSH_RX>();
      return 0;
    }
    ReadBuffer<R_RX_PAYLOAD>(buf, len > msglen ? msglen : len);
    return msglen;
  }

  void Receive() {
    if (!receiving_) {
      SetCe<false>();
      ModifyRegister<CONFIG>((1<<PRIM_RX), (1<<PRIM_RX));
      SetCe<true>();
      receiving_ = true;
    }
  }

 private:

  template <uint8_t reg>
  uint8_t ReadRegister() const {
    SetCsn<false>();
    SPI.transfer(R_REGISTER | (REGISTER_MASK & reg));
    uint8_t value = SPI.transfer(0);
    SetCsn<true>();
    return value;
  }

  template <uint8_t reg>
  void ReadRegister(uint8_t *value, int len) const {
    SetCsn<false>();
    SPI.transfer(R_REGISTER | (REGISTER_MASK & reg));
    for (int i = 0; i < len; ++i) {
      value[i] = SPI.transfer(0);
    }
    SetCsn<true>();
  }

  template <uint8_t reg>
  void WriteRegister(uint8_t value) {
    SetCsn<false>();
    SPI.transfer(W_REGISTER | (REGISTER_MASK & reg));
    SPI.transfer(value);
    SetCsn<true>();
  }

  template <uint8_t reg>
  void WriteRegister(const uint8_t *value, int len) {
    SetCsn<false>();
    SPI.transfer(W_REGISTER | (REGISTER_MASK & reg));
    for (int i = 0; i < len; ++i) {
      SPI.transfer(value[i]);
    }
    SetCsn<true>();
  }


  template <uint8_t reg>
  void ModifyRegister(uint8_t value, uint8_t mask) {
    uint8_t old = ReadRegister<reg>();
    old &= ~mask;
    old |= value;
    WriteRegister<reg>(old);
  }

  template <uint8_t cmd>
  void Command() {
    SetCsn<false>();
    SPI.transfer(cmd);
    SetCsn<true>();
  }

  template <uint8_t cmd>
  uint8_t ReadCommand() {
    SetCsn<false>();
    SPI.transfer(cmd);
    uint8_t value = SPI.transfer(0);
    SetCsn<true>();
    return value;
  }


  template <uint8_t cmd>
  void WriteBuffer(const uint8_t *buf, int len) {
    SetCsn<false>();
    SPI.transfer(cmd);
    for (int i = 0; i < len; ++i) {
      SPI.transfer(buf[i]);
    }
    SetCsn<true>();
  }

  template <uint8_t cmd>
  void ReadBuffer(uint8_t *buf, int len) {
    SetCsn<false>();
    SPI.transfer(cmd);
    for (int i = 0; i < len; ++i) {
      buf[i] = SPI.transfer(0);
    }
    SetCsn<true>();
  }

  template <bool value>
  void SetCe() {
    digitalWrite(ce_pin, value);
  }

  template <bool value>
  void SetCsn() const {
    digitalWrite(csn_pin, value);
  }

  bool receiving_;
};

#endif
