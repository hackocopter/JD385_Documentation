#ifndef RFM70_H
#define RFM70_H
// Funktionen, um mit einem RFM70 Modul die Funktionen einer JD-385
// Fernsteuerung kontrollieren/simulieren zu können.

// Entstanden aus einer Lib von Daniel Weber,
// http:://projects.web4clans.com, daniel.weber@web4clans.com

// Version für Atmel Studio, ATMega328 oder ATMega1284p. Für
// andere Typen müssen die Pinbelegungen angepasst werden.

// Hans Georg Giese, DF2AU
// mailto: df2au@gmx.de

// Copyright: Freeware

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <avr/pgmspace.h>

// PIN Zuordnungen
#if defined(__AVR_ATmega1284P__)
#define DDR_SPI     DDRB
#define PORT_SPI    PORTB
#define CE          PB3
#define CSN         PB4
#define SCK         PB7
#define MISO        PB6
#define MOSI        PB5
#define DDR_IRQ     DDRA
#define IRQ         PA0
#elif defined(__AVR_ATmega328P__)
#define DDR_SPI     DDRB
#define PORT_SPI    PORTB
#define CE          PB1
#define CSN         PB2
#define SCK         PB5
#define MISO        PB4
#define MOSI        PB3
#define DDR_IRQ     DDRD
#define IRQ         PD2
#else
#error Prozessor nicht definiert
#endif

#define USE_IRQ     0
#define SPI_CLOCK   RFM77_SPI_CLOCK_DIV2


//************************RFM function parameter constants********************************//
#define WITH_ACK     0x01 // parameter for sendPayload(..): send with ack expectation
#define NO_ACK       0x00 // parameter for sendPayload(..): send without ack expectation
#define MODE_PTX     0x00 // parameter for setMode(mode): set to transmitter
#define MODE_PRX     0x01 // parameter for setMode(mode): set to receiver
#define EN_AA        0x01 // parameter for configRxPipe(..): enable pipe auto ack
#define NO_AA        0x00 // parameter for configRxPipe(..): disable pipe auto ack
#define TX_DPL       0x01 // parameter for configTxPipe(..): enable dynamic payload for PTX
#define TX_SPL       0x00 // parameter for configTxPipe(..): enable static payload for PTX
#define CRC0         0x00 // parameter for configCRC(crc): disable CRC
#define CRC1         0x01 // parameter for configCRC(crc): 1 byte CRC
#define CRC2         0x02 // parameter for configCRC(crc): 2 byte CRC
#define MBPS1        0x01 // parameter for configSpeed(speed): 1Mbps
#define MBPS2        0x02 // parameter for configSpeed(speed): 2Mbps
#define DBMM10       0x00 // parameter for confRfPwr(pwr): -10 dBm
#define DBMM5        0x01 // parameter for confRfPwr(pwr): -5 dBm
#define DBM0         0x02 // parameter for confRfPwr(pwr): 0 dBm
#define DBM5         0x03 // parameter for confRfPwr(pwr): +5 dBm
#define ADR_WIDTH3   0x03 // parameter for confAdrWidth(width): 3 byte
#define ADR_WIDTH4   0x03 // parameter for confAdrWidth(width): 4 byte
#define ADR_WIDTH5   0x03 // parameter for confAdrWidth(width): 5 byte
#define PWR_OFF      0x00 // parameter for setPower(pwr): off
#define PWR_ON       0x01 // parameter for setPower(pwr): on


//************************RFM Definitions************************************************//
#define RFM70_MAX_PACKET_LEN 32// max value is 32
#define RFM70_BEGIN_INIT_WAIT_MS 1000 // pause before Init Registers
#define RFM70_END_INIT_WAIT_MS 100 // pause after init registers
#define RFM70_CS_DELAY 0 // wait ms after CS pin state change

//************************RFM COMMAND and REGISTER****************************************//
// SPI(RFM70) commands
#define RFM70_CMD_READ_REG 0x00 // Define read command to register
#define RFM70_CMD_WRITE_REG 0x20 // Define write command to register
#define RFM70_CMD_RD_RX_PLOAD 0x61 // Define RX payload command
#define RFM70_CMD_WR_TX_PLOAD 0xA0 // Define TX payload command
#define RFM70_CMD_FLUSH_TX 0xE1 // Define flush TX register command
#define RFM70_CMD_FLUSH_RX 0xE2 // Define flush RX register command
#define RFM70_CMD_REUSE_TX_PL 0xE3 // Define reuse TX payload register command
#define RFM70_CMD_W_TX_PAYLOAD_NOACK 0xb0 // Define TX payload NOACK command
#define RFM70_CMD_W_ACK_PAYLOAD 0xa8 // Define Write ack command
#define RFM70_CMD_ACTIVATE 0x50 // Define feature activation command
#define RFM70_CMD_RX_PL_WID 0x60 // Define received payload width command
#define RFM70_CMD_NOP_NOP 0xFF // Define No Operation, might be used to read status register

// SPI(RFM70) registers(addresses)
#define RFM70_REG_CONFIG 0x00 // 'Config' register address
#define RFM70_REG_EN_AA 0x01 // 'Enable Auto Acknowledgment' register address
#define RFM70_REG_EN_RXADDR 0x02 // 'Enabled RX addresses' register address
#define RFM70_REG_SETUP_AW 0x03 // 'Setup address width' register address
#define RFM70_REG_SETUP_RETR 0x04 // 'Setup Auto. Retrans' register address
#define RFM70_REG_RF_CH 0x05 // 'RF channel' register address
#define RFM70_REG_RF_SETUP 0x06 // 'RF setup' register address
#define RFM70_REG_STATUS 0x07 // 'Status' register address
#define RFM70_REG_OBSERVE_TX 0x08 // 'Observe TX' register address
#define RFM70_REG_CD 0x09 // 'Carrier Detect' register address
#define RFM70_REG_RX_ADDR_P0 0x0A // 'RX address pipe0' register address
#define RFM70_REG_RX_ADDR_P1 0x0B // 'RX address pipe1' register address
#define RFM70_REG_RX_ADDR_P2 0x0C // 'RX address pipe2' register address
#define RFM70_REG_RX_ADDR_P3 0x0D // 'RX address pipe3' register address
#define RFM70_REG_RX_ADDR_P4 0x0E // 'RX address pipe4' register address
#define RFM70_REG_RX_ADDR_P5 0x0F // 'RX address pipe5' register address
#define RFM70_REG_TX_ADDR 0x10 // 'TX address' register address
#define RFM70_REG_RX_PW_P0 0x11 // 'RX payload width, pipe0' register address
#define RFM70_REG_RX_PW_P1 0x12 // 'RX payload width, pipe1' register address
#define RFM70_REG_RX_PW_P2 0x13 // 'RX payload width, pipe2' register address
#define RFM70_REG_RX_PW_P3 0x14 // 'RX payload width, pipe3' register address
#define RFM70_REG_RX_PW_P4 0x15 // 'RX payload width, pipe4' register address
#define RFM70_REG_RX_PW_P5 0x16 // 'RX payload width, pipe5' register address
#define RFM70_REG_FIFO_STATUS 0x17 // 'FIFO Status Register' register address
#define RFM70_REG_DYNPD 0x1c // 'Enable dynamic payload length' register address
#define RFM70_REG_FEATURE 0x1d // 'Feature' register address

//************************RFM Debug Tokens******************************************//
#define RFM70_DEBUG_WRONG_CHIP_ID 0x01
#define RFM70_DEBUG_FIFO_FULL 0x02

//************************RFM SPI Constants****************************************//
#define RFM77_SPI_CLOCK_DIV4 0x00
#define RFM77_SPI_CLOCK_DIV16 0x01
//#define RFM77_SPI_CLOCK_DIV64 0x02
#define RFM77_SPI_CLOCK_DIV128 0x03
#define RFM77_SPI_CLOCK_DIV2 0x04
#define RFM77_SPI_CLOCK_DIV8 0x05
#define RFM77_SPI_CLOCK_DIV32 0x06
#define RFM77_SPI_CLOCK_DIV64 0x07
#define SPI_CLOCK_MASK 0x03 // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01 // SPI2X = bit 0 on SPSR


//interrupt status
#define RFM70_IRQ_STATUS_RX_DR 0x40 // Status bit RX_DR IRQ
#define RFM70_IRQ_STATUS_TX_DS 0x20 // Status bit TX_DS IRQ
#define RFM70_IRQ_STATUS_MAX_RT 0x10 // Status bit MAX_RT IRQ

#define RFM70_IRQ_STATUS_TX_FULL 0x01 


#define RFM70_PIN_PRIM_RX 0x01
#define RFM70_PIN_POWER 0x02

//FIFO_STATUS
#define RFM70_FIFO_STATUS_TX_REUSE 0x40
#define RFM70_FIFO_STATUS_TX_FULL 0x20
#define RFM70_FIFO_STATUS_TX_EMPTY 0x10

#define RFM70_FIFO_STATUS_RX_FULL 0x02
#define RFM70_FIFO_STATUS_RX_EMPTY 0x01


void initSPI(uint8_t clk_div);
void initHardware(uint8_t irq);
uint8_t initRegisters(void);
uint8_t transmitSPI(uint8_t val);
uint8_t readRegVal(uint8_t cmd);
uint8_t writeRegVal(uint8_t cmd, uint8_t val);
uint8_t writeRegPgmBuf(uint8_t * cmdbuf, uint8_t len);
void readRegBuf(uint8_t reg, uint8_t * buf, uint8_t len);
void selectBank(uint8_t bank);
void setModeTX(void);
void setModeRX(void);
void setMode(uint8_t mode);
uint8_t getMode(void);
void setChannel(uint8_t cnum);
uint8_t getChannel(void);
uint8_t configRxPipe(uint8_t pipe_nr, uint8_t * adr, uint8_t plLen, uint8_t en_aa);
void enableRxPipe(uint8_t pipe_nr);
void debug(uint8_t token);
void setModeRX(void);
void spiSetClockDivider(uint8_t rate);
uint8_t RFM70_init(void);
uint8_t writeRegCmdBuf(uint8_t cmd, uint8_t * buf, uint8_t len);
void disableRxPipe(uint8_t pipe_nr);
void configTxPipe(uint8_t * adr, uint8_t pltype);
void flushTxFIFO();
void flushRxFIFO();
uint8_t receivePayload(uint8_t *payload);
uint8_t sendPayload(uint8_t * payload, uint8_t len, uint8_t toAck);
void configRfPower(uint8_t pwr);

#endif
