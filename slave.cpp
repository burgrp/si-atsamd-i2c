namespace atsamd::i2c {

class Slave : public Common {

  int pinSDA;

public:
  int rxLength;
  int txLength;

  void init(int address1, int address2, AddressMode addressMode,
            volatile target::sercom::Peripheral *sercom,
            target::gclk::CLKCTRL::GEN clockGen, int pinSDA,
            target::port::PMUX::PMUXE muxSDA, int pinSCL,
            target::port::PMUX::PMUXE muxSCL) {

    Common::init(sercom, clockGen, pinSDA, muxSDA, pinSCL, muxSCL);

    this->pinSDA = pinSDA;

    sercom->I2CS.INTENSET =
        sercom->I2CS.INTENSET.bare().setDRDY(true).setAMATCH(true).setPREC(
            true);

    sercom->I2CS.CTRLB =
        sercom->I2CS.CTRLB.bare().setAACKEN(false).setSMEN(false).setAMODE(
            (int)addressMode);

    sercom->I2CS.ADDR =
        sercom->I2CS.ADDR.bare().setADDR(address1).setADDRMASK(address2);

    sercom->I2CS.CTRLA =
        sercom->I2CS.CTRLA.bare()
            .setMODE(target::sercom::I2CS::CTRLA::MODE::I2C_SLAVE)
            .setSCLSM(false)
            .setENABLE(true);

    while (sercom->I2CS.SYNCBUSY)
      ;
  }

  const int CMD_END = 2;
  const int CMD_CONTINUE = 3;

  void interruptHandlerSERCOM() {

    target::sercom::I2CS::INTFLAG::Register flags = sercom->I2CS.INTFLAG.copy();
    target::sercom::I2CS::STATUS::Register status = sercom->I2CS.STATUS.copy();

    if (flags.getPREC()) {
      sercom->I2CS.INTFLAG.setPREC(true);
    }

    if (flags.getAMATCH()) {
      sercom->I2CS.CTRLB.setACKACT(0);
      sercom->I2CS.INTFLAG.setAMATCH(true);

      if (status.getDIR()) {
        // master read
        txLength = 0;
      } else {
        // master write
        rxLength = 0;
      }
    }

    if (flags.getDRDY()) {

      if (status.getDIR()) {

        // master read
        sercom->I2CS.DATA = getTxByte(txLength++);

      } else {

        // master write
        int byte = sercom->I2CS.DATA;
        bool ack = setRxByte(rxLength++, byte);
        if (ack) {
          sercom->I2CS.CTRLB.setACKACT(0).setCMD(CMD_CONTINUE);
        } else {
          sercom->I2CS.CTRLB.setACKACT(1).setCMD(CMD_END);
        }
      }
    }
  }

  virtual int getTxByte(int index) { return -1; }

  virtual bool setRxByte(int index, int value) { return false; }
};
} // namespace atsamd::i2c
