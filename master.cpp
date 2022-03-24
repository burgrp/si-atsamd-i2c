namespace atsamd::i2c {

class Master: public Common {
  volatile target::sercom::Peripheral *sercom;

public:
public:
  int rxLength;
  int rxLimit;

  int txLength;
  int txLimit;

  unsigned char *rxBufferPtr;
  unsigned int rxBufferSize;
  unsigned char *txBufferPtr;
  unsigned int txBufferSize;

  void init(volatile target::sercom::Peripheral *sercom,
            target::gclk::CLKCTRL::GEN clockGen, int pinSDA,
            target::port::PMUX::PMUXE muxSDA, int pinSCL,
            target::port::PMUX::PMUXE muxSCL,
    int genHz, int sclHz,
            unsigned char *rxBufferPtr, unsigned int rxBufferSize,
            unsigned char *txBufferPtr, unsigned int txBufferSize) {

    Common::init(sercom, clockGen, pinSDA, muxSDA, pinSCL, muxSCL);

    this->rxBufferPtr = rxBufferPtr;
    this->rxBufferSize = rxBufferSize;
    this->txBufferPtr = txBufferPtr;
    this->txBufferSize = txBufferSize;

    int baud = genHz / (2 * sclHz);
    sercom->I2CM.BAUD = sercom->I2CM.BAUD.bare().setBAUD(baud).setBAUDLOW(baud);

    sercom->I2CM.INTENSET =
        sercom->I2CM.INTENSET.bare().setMB(true).setSB(true); //.setERROR(true)

    sercom->I2CM.CTRLA =
        sercom->I2CM.CTRLA.bare()
            .setMODE(target::sercom::I2CM::CTRLA::MODE::I2C_MASTER)
            .setSCLSM(false)
            .setENABLE(true);

    while (sercom->I2CM.SYNCBUSY)
      ;

    sercom->I2CM.STATUS.setBUSSTATE(1); // force bus idle state
  }

  const int CMD_ACK_AND_READ = 2;
  const int CMD_ACK_AND_STOP = 3;

  void acknowledge(int command, bool ackFlag) {
    sercom->I2CM.CTRLB.setACKACT(!ackFlag).setCMD(command);
  }

  void interruptHandlerSERCOM() {

    if (sercom->I2CM.INTFLAG.getMB() || sercom->I2CM.INTFLAG.getSB()) {

      bool read = sercom->I2CM.ADDR & 1;

      if (sercom->I2CM.STATUS.getBUSERR() || sercom->I2CM.STATUS.getRXNACK()) {
        // errors

        if (read) {
          // read
          rxComplete(rxLength);
        } else {
          // write
          txComplete(txLength && txLength - 1);
        }

        acknowledge(CMD_ACK_AND_STOP, false);

      } else {
        if (read) {

          // read
          if (rxLength < rxLimit) {
            rxBufferPtr[rxLength++] = sercom->I2CM.DATA;
          }

          if (rxLength < rxLimit) {
            acknowledge(CMD_ACK_AND_READ, true);
          } else {
            rxComplete(rxLength);
            acknowledge(CMD_ACK_AND_STOP, false);
          }

        } else {
          // write

          if (txLength < txLimit) {
            sercom->I2CM.DATA = txBufferPtr[txLength++];
          } else {
            txComplete(txLength);
            acknowledge(CMD_ACK_AND_STOP, true);
          }
        }
      }
    }
  }

  virtual void rxComplete(int length){};
  virtual void txComplete(int length){};

  virtual void startRx(int address, int length) {
    rxLength = 0;
    rxLimit = length > rxBufferSize ? rxBufferSize : length;
    sercom->I2CM.ADDR.setADDR(address << 1 | 1);
    while (sercom->I2CM.SYNCBUSY)
      ;
  }

  virtual void startTx(int address, int length) {
    txLength = 0;
    txLimit = length > txBufferSize ? txBufferSize : length;
    sercom->I2CM.ADDR.setADDR(address << 1);
    while (sercom->I2CM.SYNCBUSY)
      ;
  }
};
} // namespace atsamd::i2c
