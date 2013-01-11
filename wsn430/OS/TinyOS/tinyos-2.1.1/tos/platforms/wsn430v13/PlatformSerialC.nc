configuration PlatformSerialC {
  
  provides interface StdControl;
  provides interface UartStream;
  provides interface UartByte;
  
}

implementation {
  
  components new Msp430Uart0C() as UartC;
  UartStream = UartC;  
  UartByte = UartC;
  
  components Wsn430SerialP;
  StdControl = Wsn430SerialP;
  Wsn430SerialP.Msp430UartConfigure <- UartC.Msp430UartConfigure;
  Wsn430SerialP.Resource -> UartC.Resource;
  
}
