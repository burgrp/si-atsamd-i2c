namespace atsamd::i2c {

enum AddressMode { MASK, TWO, RANGE };

const int SERCOM_COUNT = 2;

// TODO: make lookup tables constant to save RAM

target::gclk::CLKCTRL::ID SERCOM_CLKCTRL[SERCOM_COUNT] = {
    target::gclk::CLKCTRL::ID::SERCOM0_CORE,
    target::gclk::CLKCTRL::ID::SERCOM1_CORE};

class Common {

public:
  volatile target::sercom::Peripheral *sercom;

  static void setPerpheralMux(int pin, target::port::PMUX::PMUXE mux) {
    if (pin & 1) {
      target::PORT.PMUX[pin >> 1].setPMUXO((target::port::PMUX::PMUXO)mux);
    } else {
      target::PORT.PMUX[pin >> 1].setPMUXE(mux);
    }
  }

  void init(volatile target::sercom::Peripheral *sercom,
            target::gclk::CLKCTRL::GEN clockGen, int pinSDA,
            target::port::PMUX::PMUXE muxSDA, int pinSCL,
            target::port::PMUX::PMUXE muxSCL) {

    int sercomOffset =
        (int)(void *)&target::SERCOM1 - (int)(void *)&target::SERCOM0;
    int sercomIndex =
        ((int)(void *)sercom - (int)(void *)&target::SERCOM0) / sercomOffset;

    this->sercom = sercom;

    setPerpheralMux(pinSDA, muxSDA);
    setPerpheralMux(pinSCL, muxSCL);

    target::PORT.PINCFG[pinSDA].setPMUXEN(true);
    target::PORT.PINCFG[pinSCL].setPMUXEN(true);

    target::PM.APBCMASK.setSERCOM(sercomIndex, true);

    target::GCLK.CLKCTRL = target::GCLK.CLKCTRL.bare()
                               .setID(SERCOM_CLKCTRL[sercomIndex])
                               .setGEN(clockGen)
                               .setCLKEN(true);

    while (target::GCLK.STATUS.getSYNCBUSY())
      ;
  }
};
} // namespace atsamd::i2c
