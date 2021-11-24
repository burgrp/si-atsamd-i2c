namespace atsamd::i2c {

enum AddressMode { MASK, TWO, RANGE };

const int SERCOM_COUNT = 2;

// TODO: make lookup tables constant to save RAM

volatile target::sercom::Peripheral *SERCOM_PERIPHERAL[SERCOM_COUNT] = {&target::SERCOM0, &target::SERCOM1};

target::gclk::CLKCTRL::ID SERCOM_CLKCTRL[SERCOM_COUNT] = {target::gclk::CLKCTRL::ID::SERCOM0_CORE,
                                                          target::gclk::CLKCTRL::ID::SERCOM1_CORE};

class Slave {
  volatile target::sercom::Peripheral *sercom;

  void setPerpheralMux(int pin, target::port::PMUX::PMUXE mux) {
    if (pin & 1) {
      target::PORT.PMUX[pin >> 1].setPMUXO((target::port::PMUX::PMUXO)mux);
    } else {
      target::PORT.PMUX[pin >> 1].setPMUXE(mux);
    }
  }

  int pinSDA;

public:
  int rxLength;
  int txLength;

  void init(int address1, int address2, AddressMode addressMode, int sercomIndex, target::gclk::CLKCTRL::GEN clockGen,
            int pinSDA, int pinSCL, target::port::PMUX::PMUXE peripheralMux) {

    setPerpheralMux(pinSDA, peripheralMux);
    setPerpheralMux(pinSCL, peripheralMux);

    target::PORT.PINCFG[pinSDA].setPMUXEN(true);
    target::PORT.PINCFG[pinSCL].setPMUXEN(true);

    target::PM.APBCMASK.setSERCOM(sercomIndex, true);

    target::GCLK.CLKCTRL =
        target::GCLK.CLKCTRL.bare().setID(SERCOM_CLKCTRL[sercomIndex]).setGEN(clockGen).setCLKEN(true);

    while (target::GCLK.STATUS.getSYNCBUSY())
      ;

    this->sercom = SERCOM_PERIPHERAL[sercomIndex];
    this->pinSDA = pinSDA;

    sercom->I2CS.INTENSET = sercom->I2CS.INTENSET.bare().setDRDY(true).setAMATCH(true).setPREC(true);

    sercom->I2CS.CTRLB = sercom->I2CS.CTRLB.bare().setAACKEN(false).setSMEN(false).setAMODE((int)addressMode);

    sercom->I2CS.ADDR = sercom->I2CS.ADDR.bare().setADDR(address1).setADDRMASK(address2);

    sercom->I2CS.CTRLA =
        sercom->I2CS.CTRLA.bare().setMODE(target::sercom::I2CS::CTRLA::MODE::I2C_SLAVE).setSCLSM(false).setENABLE(true);

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
