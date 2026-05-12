/**
 * HC32F460 SPI TFT driver for Anycubic Kobra Neo.
 * Hardware SPI2 for SCK+MOSI, GPIO for CS and DC.
 */

#ifdef ARDUINO_ARCH_HC32

#include "../../../inc/MarlinConfig.h"

#if HAS_SPI_TFT

#include "tft_spi.h"
#include <hc32_ddl.h>

#define TFT_DC_H()   WRITE(TFT_A0_PIN, HIGH)
#define TFT_DC_L()   WRITE(TFT_A0_PIN, LOW)
#define TFT_CS_H()   WRITE(TFT_CS_PIN, HIGH)
#define TFT_CS_L()   WRITE(TFT_CS_PIN, LOW)

static uint16_t currentDataWidth = DATASIZE_16BIT;
static bool spiReady = false;

static void initSPI2() {
  if (spiReady) return;

  // GPIO pins for CS and DC
  OUT_WRITE(TFT_A0_PIN, HIGH);
  OUT_WRITE(TFT_CS_PIN, HIGH);
  #if PIN_EXISTS(TFT_BACKLIGHT)
    OUT_WRITE(TFT_BACKLIGHT_PIN, HIGH);
  #endif

  // SPI2 peripheral clock
  PWC_Fcg1PeriphClockCmd(PWC_FCG1_PERIPH_SPI2, Enable);

  // Route SCK and MOSI to SPI2 alternate function
  GPIO_SetFunc(TFT_SCK_PIN,  Func_Spi2_Sck,  Disable);
  GPIO_SetFunc(TFT_MOSI_PIN, Func_Spi2_Mosi, Disable);

  // SPI2 config — matches the fork's bsp_spi_tft.cpp exactly
  stc_spi_init_t cfg;
  MEM_ZERO_STRUCT(cfg);

  cfg.enClkDiv             = SpiClkDiv2;
  cfg.enFrameNumber        = SpiFrameNumber1;
  cfg.enDataLength         = SpiDataLengthBit8;
  cfg.enFirstBitPosition   = SpiFirstBitPositionMSB;
  cfg.enSckPolarity        = SpiSckIdleLevelLow;
  cfg.enSckPhase           = SpiSckOddSampleEvenChange;
  cfg.enReadBufferObject   = SpiReadReceiverBuffer;
  cfg.enWorkMode           = SpiWorkMode4Line;
  cfg.enTransMode          = SpiTransFullDuplex;
  cfg.enCommAutoSuspendEn  = Disable;
  cfg.enModeFaultErrorDetectEn = Disable;
  cfg.enParitySelfDetectEn = Disable;
  cfg.enParityEn           = Disable;
  cfg.enParity             = SpiParityEven;
  cfg.enMasterSlaveMode    = SpiModeMaster;

  cfg.stcDelayConfig.enSsSetupDelayOption   = SpiSsSetupDelayCustomValue;
  cfg.stcDelayConfig.enSsSetupDelayTime     = SpiSsSetupDelaySck1;
  cfg.stcDelayConfig.enSsHoldDelayOption    = SpiSsHoldDelayCustomValue;
  cfg.stcDelayConfig.enSsHoldDelayTime      = SpiSsHoldDelaySck1;
  cfg.stcDelayConfig.enSsIntervalTimeOption = SpiSsIntervalCustomValue;
  cfg.stcDelayConfig.enSsIntervalTime       = SpiSsIntervalSck6PlusPck2;
  cfg.stcSsConfig.enSsValidBit  = SpiSsValidChannel0;
  cfg.stcSsConfig.enSs0Polarity = SpiSsLowValid;

  SPI_Init(M4_SPI2, &cfg);
  SPI_Cmd(M4_SPI2, Enable);

  spiReady = true;
}

// Re-assert SPI2 pin functions if they got clobbered by Marlin's init
static void ensureSpiPins() {
  if (!spiReady) initSPI2();
  else {
    GPIO_SetFunc(TFT_SCK_PIN,  Func_Spi2_Sck,  Disable);
    GPIO_SetFunc(TFT_MOSI_PIN, Func_Spi2_Mosi, Disable);
  }
}

static inline void hwByte(uint8_t dat) {
  SPI_SendData8(M4_SPI2, dat);
  while (SPI_GetFlag(M4_SPI2, SpiFlagSendBufferEmpty) == Reset) {}
  // Flush RX to prevent receive overflow (full-duplex mode clocks in
  // garbage on MISO; if RX overflows the SPI peripheral may misbehave)
  (void)M4_SPI2->DR;
}

static void sendByte(uint8_t dat) {
  TFT_CS_L();
  hwByte(dat);
  TFT_CS_H();
}

static void send16(uint16_t dat) {
  // Each byte gets its own CS cycle — matches the fork's SPI2_NSS0
  // hardware behavior (per-frame CS toggle with SpiFrameNumber1)
  TFT_CS_L(); hwByte(dat >> 8);  TFT_CS_H();
  TFT_CS_L(); hwByte(dat & 0xFF); TFT_CS_H();
}

void spiSendByte(uint8_t data) { sendByte(data); }
void spiSend16(uint16_t data) { send16(data); }

// ----- TFT_SPI public API -----

void TFT_SPI::init() {
  initSPI2();

  // ST7789V init sequence from the Anycubic Kobra Neo fork
  #define CMD(c) do { TFT_DC_L(); sendByte(c); TFT_DC_H(); } while(0)
  #define DAT(d) sendByte(d)

  CMD(0x11); delay(120);
  CMD(0x36); DAT(0xB0);
  CMD(0x3A); DAT(0x05);
  CMD(0xB2); DAT(0x05); DAT(0x05); DAT(0x00); DAT(0x33); DAT(0x33);
  CMD(0xB7); DAT(0x35);
  CMD(0xBB); DAT(0x28);
  CMD(0xC0); DAT(0x2C);
  CMD(0xC2); DAT(0x01);
  CMD(0xC3); DAT(0x0B);
  CMD(0xC4); DAT(0x20);
  CMD(0xC6); DAT(0x0F);
  CMD(0xD0); DAT(0xA4); DAT(0xA1);
  CMD(0xE0); DAT(0xD0); DAT(0x01); DAT(0x08); DAT(0x0F); DAT(0x11);
             DAT(0x2A); DAT(0x36); DAT(0x55); DAT(0x44); DAT(0x3A);
             DAT(0x0B); DAT(0x06); DAT(0x11); DAT(0x20);
  CMD(0xE1); DAT(0xD0); DAT(0x02); DAT(0x07); DAT(0x0A); DAT(0x0B);
             DAT(0x18); DAT(0x34); DAT(0x43); DAT(0x4A); DAT(0x2B);
             DAT(0x1B); DAT(0x1C); DAT(0x22); DAT(0x1F);
  CMD(0x29); delay(50);

  #undef CMD
  #undef DAT

  // Mark pins as needing re-assertion after Marlin's remaining init runs
  spiReady = false;
}

uint32_t TFT_SPI::getID() {
  #ifdef TFT_DEFAULT_DRIVER
    return TFT_DEFAULT_DRIVER;
  #else
    return 0;
  #endif
}

uint32_t TFT_SPI::readID(const uint16_t) { return 0; }
bool TFT_SPI::isBusy() { return false; }
void TFT_SPI::abort() {}

void TFT_SPI::dataTransferBegin(uint16_t dataWidth) { currentDataWidth = dataWidth; }
void TFT_SPI::dataTransferEnd() {}
void TFT_SPI::dataTransferAbort() {}

void TFT_SPI::writeReg(const uint16_t inReg) {
  if (!spiReady) ensureSpiPins();
  TFT_DC_L();
  sendByte(inReg & 0xFF);
  TFT_DC_H();
}

void TFT_SPI::transmit(uint16_t data) {
  if (!spiReady) ensureSpiPins();
  if (currentDataWidth == DATASIZE_8BIT)
    sendByte(data & 0xFF);
  else
    send16(data);
}

void TFT_SPI::transmit(uint32_t memoryIncrease, uint16_t *data, uint16_t count) {
  if (!spiReady) ensureSpiPins();
  if (memoryIncrease) {
    for (uint16_t i = 0; i < count; i++) send16(data[i]);
  } else {
    const uint16_t val = *data;
    for (uint16_t i = 0; i < count; i++) send16(val);
  }
}

void TFT_SPI::writeSequence(uint16_t *data, uint16_t count) {
  if (!spiReady) ensureSpiPins();
  for (uint16_t i = 0; i < count; i++) send16(data[i]);
}

void TFT_SPI::writeMultiple(uint16_t color, uint32_t count) {
  if (!spiReady) ensureSpiPins();
  while (count--) send16(color);
}

#endif // HAS_SPI_TFT
#endif // ARDUINO_ARCH_HC32
