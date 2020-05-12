#ifndef __RFM_H__
#define __RFM_H__

#include <stdint.h>

// -----------------------------------------------------------------------------------------------------------------------

// #include "config.h"
#include "ogn.h"
#include "fanet.h"

class RFM_RxPktData                 // OGN packet received by the RF chip
{ public:
   static const uint8_t Bytes=26;   // [bytes] number of bytes in the packet
   uint32_t Time;                   // [sec] Time slot
   uint16_t msTime;                 // [ms] reception time since the PPS[Time]
   uint8_t Channel;                 // [] channel where the packet has been recieved
   uint8_t RSSI;                    // [-0.5dBm]
   uint8_t Data[Bytes];             // Manchester decoded data bits/bytes
   uint8_t Err [Bytes];             // Manchester decoding errors

  public:

   void Print(void (*CONS_UART_Write)(char), uint8_t WithData=0) const
   { // uint8_t ManchErr = Count1s(RxPktErr, 26);
     Format_String(CONS_UART_Write, "RxPktData: ");
     Format_HHMMSS(CONS_UART_Write, Time);
     CONS_UART_Write('+');
     Format_UnsDec(CONS_UART_Write, msTime, 4, 3);
     CONS_UART_Write(' '); Format_Hex(CONS_UART_Write, Channel);
     CONS_UART_Write('/');
     Format_SignDec(CONS_UART_Write, (int16_t)(-5*(int16_t)RSSI), 3, 1);
     Format_String(CONS_UART_Write, "dBm\n");
     if(WithData==0) return;
     for(uint8_t Idx=0; Idx<Bytes; Idx++)
     { CONS_UART_Write(' '); Format_Hex(CONS_UART_Write, Data[Idx]); }
     CONS_UART_Write('\r'); CONS_UART_Write('\n');
     for(uint8_t Idx=0; Idx<Bytes; Idx++)
     { CONS_UART_Write(' '); Format_Hex(CONS_UART_Write, Err[Idx]); }
     CONS_UART_Write('\r'); CONS_UART_Write('\n');
   }

   bool NoErr(void) const
   { for(uint8_t Idx=0; Idx<Bytes; Idx++)
       if(Err[Idx]) return 0;
     return 1; }

   uint8_t ErrCount(void) const                         // count detected manchester errors
   { uint8_t Count=0;
     for(uint8_t Idx=0; Idx<Bytes; Idx++)
       Count+=Count1s(Err[Idx]);
     return Count; }

   uint8_t ErrCount(const uint8_t *Corr) const          // count errors compared to data corrected by FEC
   { uint8_t Count=0;
     for(uint8_t Idx=0; Idx<Bytes; Idx++)
       Count+=Count1s((uint8_t)((Data[Idx]^Corr[Idx])&(~Err[Idx])));
     return Count; }

 template <class OGNx_Packet>
  uint8_t Decode(OGN_RxPacket<OGNx_Packet> &Packet, LDPC_Decoder &Decoder, uint8_t Iter=32) const
  { uint8_t Check=0;
    uint8_t RxErr = ErrCount();                                // conunt Manchester decoding errors
    Decoder.Input(Data, Err);                                  // put data into the FEC decoder
    for( ; Iter; Iter--)                                       // more loops is more chance to recover the packet
    { Check=Decoder.ProcessChecks();                           // do an iteration
      if(Check==0) break; }                                    // if FEC all fine: break
    Decoder.Output(Packet.Packet.Byte());                      // get corrected bytes into the OGN packet
    RxErr += ErrCount(Packet.Packet.Byte());
    if(RxErr>15) RxErr=15;
    Packet.RxErr  = RxErr;
    Packet.RxChan = Channel;
    Packet.RxRSSI = RSSI;
    Packet.Correct= Check==0;
    return Check; }

} ;

// -----------------------------------------------------------------------------------------------------------------------

// OGN frequencies for Europe: 868.2 and 868.4 MHz
// static const uint32_t OGN_BaseFreq  = 868200000; // [Hz] base frequency
// static const uint32_t OGN_ChanSpace =   0200000; // [Hz] channel spacing

// OGN frequencies for Australia: 917.0 base channel, with 0.4MHz channel raster and 24 hopping channels
// static const uint32_t OGN_BaseFreq  = 921400000; // [Hz] base frequency
// static const uint32_t OGN_ChanSpace =    400000; // [Hz] channel spacing

// static const double    XtalFreq = 32e6;        // [MHz] RF chip crystal frequency
//
// static const uint32_t BaseFreq  = floor(OGN_BaseFreq /(XtalFreq/(1<<19))+0.5); // conversion from RF frequency
// static const uint32_t ChanSpace = floor(OGN_ChanSpace/(XtalFreq/(1<<19))+0.5); // to RF chip synthesizer setting

// integer formula to convert from frequency to the RFM69 scheme: IntFreq = ((Freq<<16)+ 20)/ 40; where Freq is in [100kHz]
//                                                            or: IntFreq = ((Freq<<14)+ 50)/100; where Freq is in [ 10kHz]
//                                                            or: IntFreq = ((Freq<<12)+125)/250; where Freq is in [  1kHz]
//                                                            or: IntFreq = ((Freq<<11)+ 62)/125; where Freq is in [  1kHz]
// 32-bit arythmetic is enough in the above formulas


#ifdef WITH_RFM69

#include "sx1231.h"            // register addresses and values for SX1231 = RFM69

#define REG_AFCCTRL    0x0B  // AFC method

#define REG_TESTLNA     0x58  // Sensitivity boost ?
#define REG_TESTPA1     0x5A  // only present on RFM69HW/SX1231H
#define REG_TESTPA2     0x5C  // only present on RFM69HW/SX1231H
#define REG_TESTDAGC    0x6F  // Fading margin improvement ?
#define REG_TESTAFC     0x71

#define RF_IRQ_AutoMode       0x0200

#endif // of WITH_RFM69

#ifdef WITH_RFM95

#include "sx1276.h"

#define RF_IRQ_PreambleDetect 0x0200

#endif // of WITH_RFM95

                                     // bits in IrqFlags1 and IrfFlags2
#define RF_IRQ_ModeReady      0x8000 // mode change done (between some modes)
#define RF_IRQ_RxReady        0x4000
#define RF_IRQ_TxReady        0x2000 //
#define RF_IRQ_PllLock        0x1000 //
#define RF_IRQ_Rssi           0x0800
#define RF_IRQ_Timeout        0x0400
#define RF_IRQ_PreambleDetect 0x0200
#define RF_IRQ_SyncAddrMatch  0x0100

#define RF_IRQ_FifoFull       0x0080 //
#define RF_IRQ_FifoNotEmpty   0x0040 // at least one byte in the FIFO
#define RF_IRQ_FifoLevel      0x0020 // more bytes than FifoThreshold
#define RF_IRQ_FifoOverrun    0x0010 // write this bit to clear the FIFO
#define RF_IRQ_PacketSent     0x0008 // packet transmission was completed
#define RF_IRQ_PayloadReady   0x0004
#define RF_IRQ_CrcOk          0x0002
#define RF_IRQ_LowBat         0x0001

#include "manchester.h"

class RFM_TRX
{ public:                             // hardware access functions

#ifdef USE_BLOCK_SPI                                                    // SPI transfers in blocks, implicit control of the SPI-select
   void (*TransferBlock)(uint8_t *Data, uint8_t Len);
   static const size_t MaxBlockLen = 64;
   uint8_t Block_Buffer[MaxBlockLen];

   uint8_t *Block_Read(uint8_t Len, uint8_t Addr)                       // read given number of bytes from given Addr
   { Block_Buffer[0]=Addr; memset(Block_Buffer+1, 0, Len);
     (*TransferBlock) (Block_Buffer, Len+1);
     return  Block_Buffer+1; }                                          // return the pointer to the data read from the given Addr

   uint8_t *Block_Write(const uint8_t *Data, uint8_t Len, uint8_t Addr) // write given number of bytes to given Addr
   { Block_Buffer[0] = Addr | 0x80; memcpy(Block_Buffer+1, Data, Len);
     // printf("Block_Write( [0x%02X, .. ], %d, 0x%02X) .. [0x%02X, 0x%02X, ...]\n", Data[0], Len, Addr, Block_Buffer[0], Block_Buffer[1]);
     (*TransferBlock) (Block_Buffer, Len+1);
     return  Block_Buffer+1; }
#else                                                                   // SPI transfers as single bytes, explicit control of the SPI-select
   void (*Select)(void);                                                // activate SPI select
   void (*Deselect)(void);                                              // desactivate SPI select
   uint8_t (*TransferByte)(uint8_t);                                    // exchange one byte through SPI
#endif

   bool (*DIO0_isOn)(void);                                              // read DIO0 = packet is ready
   // bool (*DIO4_isOn)(void);
   void (*RESET)(uint8_t On);                                            // activate or desactivate the RF chip reset

                                      // the following are in units of the synthesizer with 8 extra bits of precision
   uint32_t BaseFrequency;            // [32MHz/2^19/2^8] base frequency = channel #0
//    int32_t FrequencyCorrection;      // [32MHz/2^19/2^8] frequency correction (due to Xtal offset)
   uint32_t ChannelSpacing;           // [32MHz/2^19/2^8] spacing between channels
    int16_t FreqCorr;                 // [0.1ppm]
    int16_t Channel;                  // [       integer] channel being used

  uint8_t chipVer;                    // [] version ID read from the RF chip
   int8_t chipTemp;                   // [degC] temperature read from the RF chip
  uint8_t averRSSI;                   // [-0.5dB]
  uint8_t dummy;

#ifdef WITH_RFM95
   void WriteDefaultReg(void)
   { const uint8_t Default[64] = {  0x00, 0x01, 0x1A, 0x0B, 0x00, 0x52, 0xE4, 0xC0, 0x00, 0x0F, 0x19, 0x2B, 0x20, 0x08, 0x02, 0x0A,
                                    0xFF, 0x00, 0x15, 0x0B, 0x28, 0x0C, 0x12, 0x47, 0x32, 0x3E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
                                    0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x03, 0x93, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55,
                                    0x90, 0x40, 0x40, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x00, 0xF5, 0x20, 0x82, 0x00, 0x02, 0x80, 0x40 };
     WriteBytes(Default+0x01, 0x0F, 0x01);
     WriteBytes(Default+0x10, 0x10, 0x10);
     WriteBytes(Default+0x20, 0x10, 0x20);
     WriteBytes(Default+0x30, 0x10, 0x30);
     WriteWord(0x0000, 0x40);
     WriteByte(0x2D, 0x44);
     WriteByte(0x09, 0x4B);
     WriteByte(0x84, 0x4D);
     WriteByte(0x00, 0x5D);
     WriteByte(0x13, 0x61);
     WriteByte(0x0E, 0x62);
     WriteByte(0x5B, 0x63);
     WriteByte(0xDB, 0x64);
   }
#endif

   static uint32_t calcSynthFrequency(uint32_t Frequency) { return (((uint64_t)Frequency<<16)+7812)/15625; }

  public:
   void setBaseFrequency(uint32_t Frequency=868200000) { BaseFrequency=calcSynthFrequency(Frequency); } // [Hz]
   void setChannelSpacing(uint32_t  Spacing=   200000) { ChannelSpacing=calcSynthFrequency(Spacing); }  // [Hz]
   void setFrequencyCorrection(int16_t ppmFreqCorr=0)  { FreqCorr = ppmFreqCorr; }                      // [0.1ppm]
//    void setFrequencyCorrection(int32_t Correction=0)
//    { if(Correction<0) FrequencyCorrection = -calcSynthFrequency(-Correction);
//                 else  FrequencyCorrection =  calcSynthFrequency( Correction); }
   void setChannel(int16_t newChannel)
   { Channel=newChannel;
     uint32_t Freq = BaseFrequency+ChannelSpacing*Channel;
      int32_t Corr = ((int64_t)Freq*FreqCorr+5000000)/10000000;
              Freq += Corr;
     WriteFreq((Freq+128)>>8); }
   uint8_t getChannel(void) const { return Channel; }

#ifdef USE_BLOCK_SPI

   static uint16_t SwapBytes(uint16_t Word) { return (Word>>8) | (Word<<8); }

   uint8_t WriteByte(uint8_t Byte, uint8_t Addr=0) // write Byte
   { // printf("WriteByte(0x%02X => [0x%02X])\n", Byte, Addr);
     uint8_t *Ret = Block_Write(&Byte, 1, Addr); return *Ret; }

   void WriteWord(uint16_t Word, uint8_t Addr=0) // write Word => two bytes
   { // printf("WriteWord(0x%04X => [0x%02X])\n", Word, Addr);
     uint16_t Swapped = SwapBytes(Word); Block_Write((uint8_t *)&Swapped, 2, Addr); }

   uint8_t ReadByte (uint8_t Addr=0)
   { uint8_t *Ret = Block_Read(1, Addr);
     // printf("ReadByte(0x%02X) => 0x%02X\n", Addr, *Ret );
     return *Ret; }

   uint16_t ReadWord (uint8_t Addr=0)
   { uint16_t *Ret = (uint16_t *)Block_Read(2, Addr);
     // printf("ReadWord(0x%02X) => 0x%04X\n", Addr, SwapBytes(*Ret) );
     return SwapBytes(*Ret); }

   void WriteBytes(const uint8_t *Data, uint8_t Len, uint8_t Addr=0)
   { Block_Write(Data, Len, Addr); }

   void WriteFreq(uint32_t Freq)                       // [32MHz/2^19] Set center frequency in units of RFM69 synth.
   { const uint8_t Addr = REG_FRFMSB;
     uint8_t Buff[4];
     Buff[0] = Freq>>16;
     Buff[1] = Freq>> 8;
     Buff[2] = Freq    ;
     Buff[3] =        0;
     Block_Write(Buff, 3, Addr); }

   uint32_t ReadFreq(uint8_t Addr=REG_FRFMSB)
   { uint8_t *Data = Block_Read(3, Addr);
     uint32_t Freq=Data[0]; Freq<<=8; Freq|=Data[1]; Freq<<=8; Freq|=Data[2];
     return Freq; }

   void WriteFIFO(const uint8_t *Data, uint8_t Len)
   { Block_Write(Data, Len, REG_FIFO); }

   void WritePacketOGN(const uint8_t *Data, uint8_t Len=26)         // write the packet data (26 bytes)
   { uint8_t Packet[2*Len];
     uint8_t PktIdx=0;
     for(uint8_t Idx=0; Idx<Len; Idx++)
     { uint8_t Byte=Data[Idx];
       Packet[PktIdx++]=ManchesterEncode[Byte>>4];                               // software manchester encode every byte
       Packet[PktIdx++]=ManchesterEncode[Byte&0x0F];
     }
     Block_Write(Packet, 2*Len, REG_FIFO);
   }

   uint8_t *ReadFIFO(uint8_t Len)
   { return Block_Read(Len, REG_FIFO); }

   void ReadPacketOGN(uint8_t *Data, uint8_t *Err, uint8_t Len=26)             // read packet data from FIFO
   { uint8_t *Packet = Block_Read(2*Len, REG_FIFO);                         // read 2x26 bytes from the RF chip RxFIFO
     uint8_t PktIdx=0;
     for(uint8_t Idx=0; Idx<Len; Idx++)                                     // loop over packet bytes
     { uint8_t ByteH = Packet[PktIdx++];
       ByteH = ManchesterDecode[ByteH]; uint8_t ErrH=ByteH>>4; ByteH&=0x0F; // decode manchester, detect (some) errors
       uint8_t ByteL = Packet[PktIdx++];
       ByteL = ManchesterDecode[ByteL]; uint8_t ErrL=ByteL>>4; ByteL&=0x0F;
       Data[Idx]=(ByteH<<4) | ByteL;
       Err [Idx]=(ErrH <<4) | ErrL ;
     }
   }

#else // single Byte transfer SPI

  private:
   uint8_t WriteByte(uint8_t Byte, uint8_t Addr=0) const  // write Byte
   { Select();
     TransferByte(Addr | 0x80);
     uint8_t Old=TransferByte(Byte);
     Deselect();
     return Old; }

   uint16_t WriteWord(uint16_t Word, uint8_t Addr=0) const // write Word => two bytes
   { Select();
     TransferByte(Addr | 0x80);
     uint16_t Old=TransferByte(Word>>8);             // upper byte first
     Old = (Old<<8) | TransferByte(Word&0xFF);       // lower byte second
     Deselect();
     return Old; }

   void WriteBytes(const uint8_t *Data, uint8_t Len, uint8_t Addr=0) const
   { Select();
     TransferByte(Addr | 0x80);
     for(uint8_t Idx=0; Idx<Len; Idx++)
     { TransferByte(Data[Idx]); }
     Deselect(); }

   uint8_t ReadByte (uint8_t Addr=0) const
   { Select();
     TransferByte(Addr);
     uint8_t Byte=TransferByte(0);
     Deselect();
     return Byte; }

   uint16_t ReadWord (uint8_t Addr=0) const
   { Select();
     TransferByte(Addr);
     uint16_t Word=TransferByte(0);
     Word = (Word<<8) | TransferByte(0);
     Deselect();
     return Word; }

  public:
   uint32_t WriteFreq(uint32_t Freq) const                       // [32MHz/2^19] Set center frequency in units of RFM69 synth.
   { const uint8_t Addr = REG_FRFMSB;
     Select();
     TransferByte(Addr | 0x80);
     uint32_t Old  =  TransferByte(Freq>>16);
     Old = (Old<<8) | TransferByte(Freq>>8);
     Old = (Old<<8) | TransferByte(Freq);                        // actual change in the frequency happens only when the LSB is written
     Deselect();
     return Old; }                                               // return the previously set frequency

   void WriteFIFO(const uint8_t *Data, uint8_t Len)
   { const uint8_t Addr=REG_FIFO;                                // write to FIFO
     Select();
     TransferByte(Addr | 0x80);
     for(uint8_t Idx=0; Idx<Len; Idx++)
       TransferByte(Data[Idx]);
     Deselect(); }

   void WritePacketOGN(const uint8_t *Data, uint8_t Len=26) const   // write the packet data (26 bytes)
   { const uint8_t Addr=REG_FIFO;                                // write to FIFO
     Select();
     TransferByte(Addr | 0x80);
     for(uint8_t Idx=0; Idx<Len; Idx++)
     { uint8_t Byte=Data[Idx];
       TransferByte(ManchesterEncode[Byte>>4]);                  // software manchester encode every byte
       TransferByte(ManchesterEncode[Byte&0x0F]);
     }
     Deselect(); }

   void ReadPacketOGN(uint8_t *Data, uint8_t *Err, uint8_t Len=26) const       // read packet data from FIFO
   { const uint8_t Addr=REG_FIFO;
     Select();                                                              // select the RF chip: start SPI transfer
     TransferByte(Addr);                                                    // trasnfer the address/read: FIFO
     for(uint8_t Idx=0; Idx<Len; Idx++)                                     // loop over packet byte
     { uint8_t ByteH = 0;
       ByteH = TransferByte(ByteH);
       ByteH = ManchesterDecode[ByteH]; uint8_t ErrH=ByteH>>4; ByteH&=0x0F; // decode manchester, detect (some) errors
       uint8_t ByteL = 0;
       ByteL = TransferByte(ByteL);
       ByteL = ManchesterDecode[ByteL]; uint8_t ErrL=ByteL>>4; ByteL&=0x0F;
       Data[Idx]=(ByteH<<4) | ByteL;
       Err [Idx]=(ErrH <<4) | ErrL ;
     }
     Deselect(); }                                                          // de-select RF chip: end of SPI transfer

#endif // USE_BLOCK_SPI

#ifdef WITH_RFM69
   void WriteSYNC(uint8_t WriteSize, uint8_t SyncTol, const uint8_t *SyncData)
   { if(SyncTol>7) SyncTol=7;                                                // no more than 7 bit errors can be tolerated on SYNC
     if(WriteSize>8) WriteSize=8;                                            // up to 8 bytes of SYNC can be programmed
     WriteBytes(SyncData+(8-WriteSize), WriteSize, REG_SYNCVALUE1);          // write the SYNC, skip some initial bytes
     WriteByte(  0x80 | ((WriteSize-1)<<3) | SyncTol, REG_SYNCCONFIG);       // write SYNC length [bytes] and tolerance to errors [bits]
     WriteWord( 9-WriteSize, REG_PREAMBLEMSB); }                             // write preamble length [bytes] (page 71)
//              ^ 8 or 9 ?
#endif

// #ifdef WITH_RFM95
#if defined(WITH_RFM95) || defined(WITH_SX1272)
   void WriteSYNC(uint8_t WriteSize, uint8_t SyncTol, const uint8_t *SyncData)
   { if(SyncTol>7) SyncTol=7;
     if(WriteSize>8) WriteSize=8;
     WriteBytes(SyncData+(8-WriteSize), WriteSize, REG_SYNCVALUE1);        // write the SYNC, skip some initial bytes
     WriteByte(  0x90 | (WriteSize-1), REG_SYNCCONFIG);                    // write SYNC length [bytes] (or 0xB0 for reversed preamble) => p.92
     WriteWord( 9-WriteSize, REG_PREAMBLEMSB); }                           // write preamble length [bytes] (page 71)
//              ^ 8 or 9 ?
#endif

   void    WriteMode(uint8_t Mode=RF_OPMODE_STANDBY) { WriteByte(Mode, REG_OPMODE); } // SLEEP/STDBY/FSYNTH/TX/RX
   uint8_t ReadMode (void) { return ReadByte(REG_OPMODE); }
   uint8_t ModeReady(void) { return ReadByte(REG_IRQFLAGS1)&0x80; }

   uint16_t ReadIrqFlags(void) { return ReadWord(REG_IRQFLAGS1); }

   void ClearIrqFlags(void)    { WriteWord(RF_IRQ_FifoOverrun | RF_IRQ_Rssi | RF_IRQ_PreambleDetect | RF_IRQ_SyncAddrMatch, REG_IRQFLAGS1); }

#ifdef WITH_RFM69
   void WriteTxPower_W(int8_t TxPower=10)       // [dBm] for RFM69W: -18..+13dBm
   { if(TxPower<(-18)) TxPower=(-18);           // check limits
     if(TxPower>  13 ) TxPower=  13 ;
     WriteByte(  0x80+(18+TxPower), REG_PALEVEL);
     WriteByte(  0x1A             , REG_OCP);
     WriteByte(  0x55             , REG_TESTPA1);
     WriteByte(  0x70             , REG_TESTPA2);
   }

   void WriteTxPower_HW(int8_t TxPower=10)       // [dBm] // for RFM69HW: -14..+20dBm
   { if(TxPower<(-14)) TxPower=(-14);            // check limits
     if(TxPower>  20 ) TxPower=  20 ;
     if(TxPower<=17)
     { WriteByte(  0x60+(14+TxPower), REG_PALEVEL);
       WriteByte(  0x1A             , REG_OCP);
       WriteByte(  0x55             , REG_TESTPA1);
       WriteByte(  0x70             , REG_TESTPA2);
     } else
     { WriteByte(  0x60+(11+TxPower), REG_PALEVEL);
       WriteByte(  0x0F             , REG_OCP);
       WriteByte(  0x5D             , REG_TESTPA1);
       WriteByte(  0x7C             , REG_TESTPA2);
     }
   }

   void WriteTxPower(int8_t TxPower, bool isHW)
   { WriteByte(  0x09, REG_PARAMP); // Tx ramp up/down time 0x06=100us, 0x09=40us, 0x0C=20us, 0x0F=10us (page 66)
     if(isHW) WriteTxPower_HW(TxPower);
         else WriteTxPower_W (TxPower);  }

   void WriteTxPowerMin(void) { WriteTxPower_W(-18); } // set minimal Tx power and setup for reception

   int ConfigureOGN(int16_t Channel, const uint8_t *Sync, bool PW=0)
   { WriteMode(RF_OPMODE_STANDBY);          // mode = STDBY
     ClearIrqFlags();
     WriteByte(  0x02, REG_DATAMODUL);      // [0x00] Packet mode, FSK, 0x02: BT=0.5, 0x01: BT=1.0, 0x03: BT=0.3
     WriteWord(PW?0x0341:0x0140, REG_BITRATEMSB); // bit rate = 100kbps
     WriteWord(PW?0x013B:0x0333, REG_FDEVMSB);    // FSK deviation = +/-50kHz
     setChannel(Channel);                   // operating channel
     WriteSYNC(8, 7, Sync);                 // SYNC pattern (setup for reception)
     WriteByte(  0x00, REG_PACKETCONFIG1);  // [0x10] Fixed size packet, no DC-free encoding, no CRC, no address filtering
     WriteByte(0x80+51, REG_FIFOTHRESH);    // [ ] TxStartCondition=FifoNotEmpty, FIFO threshold = 51 bytes
     WriteByte(  2*26, REG_PAYLOADLENGTH);  // [0x40] Packet size = 26 bytes Manchester encoded into 52 bytes
     WriteByte(  0x02, REG_PACKETCONFIG2);  // [0x02] disable encryption (it is permanent between resets !), AutoRxRestartOn=1
     WriteByte(  0x00, REG_AUTOMODES);      // [0x00] all "none"
     WriteTxPowerMin();                     // TxPower (setup for reception)
     WriteByte(  0x08, REG_LNA);            // [0x08/88] bit #7 = LNA input impedance: 0=50ohm or 1=200ohm ?
     WriteByte( 2*112, REG_RSSITHRESH);     // [0xE4] RSSI threshold = -112dBm
     WriteByte(  0x42, REG_RXBW);           // [0x86/55] +/-125kHz Rx bandwidth => p.27+67 (A=100kHz, 2=125kHz, 9=200kHz, 1=250kHz)
     WriteByte(  0x82, REG_AFCBW);          // [0x8A/8B] +/-125kHz Rx bandwidth while AFC
     WriteWord(0x4047, REG_DIOMAPPING1);    // DIO signals: DIO0=01, DIO4=01, ClkOut=OFF
                                            // RX: DIO0 = PayloadReady, DIO4 = Rssi
                                            // TX: DIO0 = TxReady,      DIO4 = TxReady
     WriteByte(  0x1B, REG_TESTLNA);        // [0x1B] 0x2D = LNA sensitivity up by 3dB, 0x1B = default
     WriteByte(  0x30, REG_TESTDAGC);       // [0x30] 0x20 when AfcLowBetaOn, 0x30 otherwise-> page 25
     WriteByte(  0x00, REG_AFCFEI);         // [0x00] AfcAutoOn=0, AfcAutoclearOn=0
     WriteByte(  0x00, REG_AFCCTRL);        // [0x00] 0x20 = AfcLowBetaOn=1 -> page 64 -> page 33
     WriteByte(   +10, REG_TESTAFC);        // [0x00] [488Hz] if AfcLowBetaOn
     return 0; }
#endif

// #ifdef WITH_RFM95
#if defined(WITH_RFM95) || defined(WITH_SX1272)

   void WriteTxPower(int8_t TxPower=0)
   { if(TxPower>17)
     { if(TxPower>20) TxPower=20;
       WriteByte(0x87, REG_PADAC);
       WriteByte(0xF0 | (TxPower-5), REG_PACONFIG); }
     else // if(TxPower>14)
     { if(TxPower<2) TxPower=2;
       WriteByte(0x84, REG_PADAC);
       WriteByte(0xF0 | (TxPower-2), REG_PACONFIG); }
     // else
     // { if(TxPower<0) TxPower=0;
     //   WriteByte(0x84, REG_PADAC);
     //   WriteByte(0x70 | (TxPower+1), REG_PACONFIG); }

     // if(TxPower<2) TxPower=2;
     // else if(TxPower>17) TxPower=17;
     // if(TxPower<=14)
     // { WriteByte(0x70 | TxPower    , REG_PACONFIG);
     // }
     // else
     // { WriteByte(0xF0 | (TxPower-2), REG_PACONFIG); }
   }

   void WriteTxPowerMin(void) { WriteTxPower(0); }

   int SetLoRa(void)                            // switch to LoRa: has to go througth the SLEEP mode
   { WriteMode(RF_OPMODE_LORA_SLEEP);
     WriteMode(RF_OPMODE_LORA_SLEEP);
     return 0; }

   int SetFSK(void)                             // switch to FSK: has to go through the SLEEP mode
   { WriteMode(RF_OPMODE_SLEEP);
     WriteMode(RF_OPMODE_SLEEP);
     return 0; }

   int ConfigureFNT(uint8_t CR=1)                 // configure for FANET/LoRa
   { WriteTxPower(0);
     WriteByte(0x00,   REG_LORA_HOPPING_PERIOD);  // disable fast-hopping
     WriteByte(0xF1,   REG_LORA_SYNC);            // SYNC for FANET
     WriteWord(0x0005, REG_LORA_PREAMBLE_MSB);    // [symbols] minimal preamble
     WriteByte(0x80 | (CR<<1), REG_LORA_MODEM_CONFIG1);   // 0x88 = 250kHz, 4+4, explicit header
     WriteByte(0x74,   REG_LORA_MODEM_CONFIG2);   // 0x74 = SF7, CRC on
     WriteByte(0xC3,   REG_LORA_DETECT_OPTIMIZE);
     WriteByte(0x0A,   REG_LORA_DETECT_THRESHOLD);
     WriteByte(0x04,   REG_LORA_MODEM_CONFIG3);   // LNA auto-gain ?
     WriteByte(0xFF,   REG_LORA_SYMBOL_TIMEOUT);  // 0x64 = default or more ?
     WriteByte(FANET_Packet::MaxBytes,   REG_LORA_PACKET_MAXLEN);   // [bytes] enough ?
     WriteByte(0x00,   REG_LORA_RX_ADDR);
     setChannel(0);                               // operating channel
     WriteWord(0x0000, REG_DIOMAPPING1);    // 001122334455___D signals: 00=DIO0 11=DIO1 22=DIO2 33=DIO3 44=DIO4 55=DIO5 D=MapPreambleDetect
                                            // DIO0: 00=RxDone, 01=TxDone, 10=CadDone
     return 0; }

   int SendPacketFNT(const uint8_t *Data, uint8_t Len)
   { // WriteMode(RF_OPMODE_LORA_STANDBY);
     // check if FIFO empty, packets could be received ?
     WriteByte(0x00, REG_LORA_FIFO_ADDR);   // tell write to FIFO at address 0x00
     WriteFIFO(Data, Len);                  // write the packet data
     WriteByte(0x00, REG_LORA_TX_ADDR);     // tell packet address in the FIFO
     WriteByte(Len, REG_LORA_PACKET_LEN);   // tell packet length
     WriteMode(RF_OPMODE_LORA_TX);          // enter transmission mode
     return 0; }                            // afterwards just wait for TX mode to stop

   int ReceivePacketFNT(FANET_RxPacket &Packet)
   { uint8_t Flags = ReadByte(REG_LORA_IRQ_FLAGS);
     if((Flags&LORA_FLAG_RX_DONE)==0) return 0;
     uint8_t Stat = ReadByte(REG_LORA_MODEM_STATUS);     // coding rate in three top bits
     uint8_t HopChan = ReadByte(REG_LORA_HOP_CHANNEL);
     Packet.CR    = Stat>>5;                             // coding rate used for this packet
     Packet.hasCRC = HopChan&0x40;                       // did this packet have CRC ? (flags should be checked for CRC error)
     Packet.badCRC = Flags&LORA_FLAG_BAD_CRC;
     Packet.SNR   = ReadByte(REG_LORA_PACKET_SNR);       // [0.25dB] read SNR
     Packet.RSSI = -157+ReadByte(REG_LORA_PACKET_RSSI);  // [dBm] read RSSI
     int32_t FreqOfs = ReadFreq(REG_LORA_FREQ_ERR_MSB);  //
     if(FreqOfs&0x00080000) FreqOfs|=0xFFF00000;         // extend the sign bit
                       else FreqOfs&=0x000FFFFF;
     Packet.FreqOfs = (FreqOfs*1718+0x8000)>>16;         //  [10Hz]
     Packet.BitErr  = 0;
     Packet.CodeErr = 0;
     int Len=ReceivePacketFNT(Packet.Byte, Packet.MaxBytes);
     // printf("ReceivePacketFNT() => %d %02X %3.1fdB %+ddBm 0x%08X=%+6.3fkHz, %02X%02X%02X%02X\n",
     //       Packet.Len, Stat, 0.25*Packet.SNR, Packet.RSSI, FreqOfs, 0.5*0x1000000/32e9*FreqOfs,
     //       Packet.Byte[0], Packet.Byte[1], Packet.Byte[2], Packet.Byte[3]);
     Packet.Len=Len;
     WriteByte(LORA_FLAG_RX_DONE | LORA_FLAG_BAD_CRC, REG_LORA_IRQ_FLAGS);
     return Len; }

   int ReceivePacketFNT(uint8_t *Data, uint8_t MaxLen)
   { uint8_t Len=ReadByte(REG_LORA_PACKET_BYTES);    // packet length
     uint8_t Ptr=ReadByte(REG_LORA_PACKET_ADDR);     // packet address in FIFO
     WriteByte(Ptr, REG_LORA_FIFO_ADDR);             // ask to read FIFO from this address
     // uint8_t Stat = ReadByte(REG_LORA_MODEM_STATUS); //
     //  int8_t SNR  = ReadByte(REG_LORA_PACKET_SNR);   // [0.25dB] read SNR
     //  int8_t RSSI = ReadByte(REG_LORA_PACKET_RSSI);  // [dBm] read RSSI
     // int32_t FreqOfs = ReadFreq(REG_LORA_FREQ_ERR_MSB); // (FreqOfs*1718+0x8000)>>16 [10Hz]
     // if(FreqOfs&0x00080000) FreqOfs|=0xFFF00000;     // extend the sign bit
     //                   else FreqOfs&=0x000FFFFF;
     uint8_t *ReadData = ReadFIFO(Len);              // read data from FIFO
     memcpy(Data, ReadData, Len);
     // printf("ReceivePacketFNT( , %d) => %d [%02X] %02X %3.1fdB %+ddBm 0x%08X=%+6.3fkHz, %02X%02X%02X%02X\n",
     //       MaxLen, Len, Ptr, Stat, 0.25*SNR, -157+RSSI, FreqOfs, 0.5*0x1000000/32e9*FreqOfs,
     //       ReadData[0], ReadData[1], ReadData[2], ReadData[3]);
     return Len; }

   int ConfigureOGN(int16_t Channel, const uint8_t *Sync, bool PW=0)
   { // WriteMode(RF_OPMODE_STANDBY);              // mode: STDBY, modulation: FSK, no LoRa
     // usleep(1000);
     WriteTxPower(0);
     ClearIrqFlags();
     WriteWord(PW?0x0341:0x0140, REG_BITRATEMSB); // bit rate = 100kbps (32MHz/100000) (0x0341 = 38.415kbps)
     WriteByte(0x00, REG_BITRATEFRAC);          // one should set exactly 38.400kbps for PW
     // ReadWord(REG_BITRATEMSB);
     WriteWord(PW?0x013B:0x0333, REG_FDEVMSB);  // FSK deviation = +/-50kHz [32MHz/(1<<19)] (0x013B = 19.226kHz)
     // ReadWord(REG_FDEVMSB);
     setChannel(Channel);                       // operating channel
     WriteSYNC(8, 7, Sync);                     // SYNC pattern (setup for reception)
     WriteByte(  0x85, REG_PREAMBLEDETECT);     // preamble detect: 1 byte, page 92 (or 0x85 ?)
     WriteByte(  0x00, REG_PACKETCONFIG1);      // Fixed size packet, no DC-free encoding, no CRC, no address filtering
     WriteByte(  0x40, REG_PACKETCONFIG2);      // Packet mode
     WriteByte(  2*26, REG_PAYLOADLENGTH);      // Packet size = 26 bytes Manchester encoded into 52 bytes
     WriteByte(    51, REG_FIFOTHRESH);         // TxStartCondition=FifoNotEmpty, FIFO threshold = 51 bytes
     WriteWord(0x3030, REG_DIOMAPPING1);        // DIO signals: DIO0=00, DIO1=11, DIO2=00, DIO3=00, DIO4=00, DIO5=11, => p.64, 99
     WriteByte(  0x02, REG_RXBW);               // +/-125kHz Rx (single-side) bandwidth => p.27,67,83,90
     WriteByte(  0x02, REG_AFCBW);              // +/-125kHz AFC bandwidth
     WriteByte(  0x49, REG_PARAMP);             // BT=0.5 shaping, 40us ramp up/down
     WriteByte(  0x0E, REG_RXCONFIG);           // => p.90 (or 0x8E ?)
     WriteByte(  0x07, REG_RSSICONFIG);         // 256 samples for RSSI, no offset, => p.90,82
     WriteByte(  0x20, REG_LNA);                // max. LNA gain, => p.89

     return 0; }

   uint8_t ReadLowBat(void)  { return ReadByte(REG_LOWBAT ); }

  void PrintReg(void (*CONS_UART_Write)(char))
  { Format_String(CONS_UART_Write, "RFM95 Mode:");
    uint8_t RxMode=ReadMode();
    Format_Hex(CONS_UART_Write, RxMode);
    CONS_UART_Write(' '); CONS_UART_Write('0'+DIO0_isOn());
    Format_String(CONS_UART_Write, " IRQ:");
    Format_Hex(CONS_UART_Write, ReadWord(REG_IRQFLAGS1));
    Format_String(CONS_UART_Write, " Pre:");
    Format_Hex(CONS_UART_Write, ReadWord(REG_PREAMBLEMSB));
    Format_String(CONS_UART_Write, " SYNC:");
    Format_Hex(CONS_UART_Write, ReadByte(REG_SYNCCONFIG));
    CONS_UART_Write('/');
    for(uint8_t Idx=0; Idx<8; Idx++)
      Format_Hex(CONS_UART_Write, ReadByte(REG_SYNCVALUE1+Idx));
    Format_String(CONS_UART_Write, " FREQ:");
    Format_Hex(CONS_UART_Write, ReadByte(REG_FRFMSB));
    Format_Hex(CONS_UART_Write, ReadByte(REG_FRFMID));
    Format_Hex(CONS_UART_Write, ReadByte(REG_FRFLSB));
    Format_String(CONS_UART_Write, " RATE:");
    Format_Hex(CONS_UART_Write, ReadWord(REG_BITRATEMSB));
    Format_String(CONS_UART_Write, " FDEV:");
    Format_Hex(CONS_UART_Write, ReadWord(REG_FDEVMSB));
    Format_String(CONS_UART_Write, " DIO:");
    Format_Hex(CONS_UART_Write, ReadWord(REG_DIOMAPPING1));
    Format_String(CONS_UART_Write, " CFG:");
    Format_Hex(CONS_UART_Write, ReadByte(REG_PREAMBLEDETECT));
    Format_Hex(CONS_UART_Write, ReadByte(REG_PACKETCONFIG1));
    Format_Hex(CONS_UART_Write, ReadByte(REG_PACKETCONFIG2));
    Format_Hex(CONS_UART_Write, ReadByte(REG_FIFOTHRESH));
    Format_Hex(CONS_UART_Write, ReadByte(REG_PAYLOADLENGTH));
    Format_Hex(CONS_UART_Write, ReadByte(REG_RXBW));
    Format_Hex(CONS_UART_Write, ReadByte(REG_RSSICONFIG));
    Format_String(CONS_UART_Write, " PA:");
    Format_Hex(CONS_UART_Write, ReadByte(REG_PARAMP));
    Format_Hex(CONS_UART_Write, ReadByte(REG_PACONFIG));
    Format_String(CONS_UART_Write, "\n"); }

#endif

     uint8_t ReadVersion(void) { chipVer=ReadByte(REG_VERSION); return chipVer; }           // 0x24 for RFM69 or 0x12 for RFM95

#ifdef WITH_RFM69
     void    TriggerRSSI(void) { WriteByte(0x01, REG_RSSICONFIG); }        // trigger measurement
     uint8_t ReadyRSSI(void)   { return ReadByte(REG_RSSICONFIG) & 0x02; } // ready ?
#endif
     uint8_t ReadRSSI(void)    { return ReadByte(REG_RSSIVALUE); }         // read value: RSS = -Value/2

#ifdef WITH_RFM69
     void    TriggerTemp(void) { WriteByte(0x08, REG_TEMP1); }             // trigger measurement
     uint8_t RunningTemp(void) { return ReadByte(REG_TEMP1) & 0x04; }      // still running ?
     int8_t ReadTemp(void)     { chipTemp=165-ReadByte(REG_TEMP2); return chipTemp; } // [deg]
#endif
#if defined(WITH_RFM95) || defined(WITH_SX1272)
     int8_t ReadTemp(void)     { chipTemp = 15-ReadByte(REG_TEMP); return chipTemp; } // [degC]
#endif
/*
     void Dump(uint8_t EndAddr=0x20)
     { printf("RFM_TRX[] =");
       for(uint8_t Addr=1; Addr<=EndAddr; Addr++)
       { printf(" %02X", ReadByte(Addr)); }
       printf("\n");
     }
*/
} ;

#endif // __RFM_H__

