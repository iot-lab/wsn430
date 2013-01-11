module Wsn430SerialP {
    provides interface StdControl;
    provides interface Msp430UartConfigure;
    uses interface Resource;
}

implementation {
  
    msp430_uart_union_config_t msp430_uart_wsn430_config = { 
            {ubr: 0x0008,               //Baud rate (115200 @ 1MHz)
            umctl: 0x5B,                //Modulation (115200 @ 1MHz)
            ssel: 0x02,                 //Clock source (00=UCLKI; 01=ACLK; 10=SMCLK; 11=SMCLK)
            pena: 0,                    //Parity enable (0=disabled; 1=enabled)
            pev: 0,                     //Parity select (0=odd; 1=even)
            spb: 0,                     //Stop bits (0=one stop bit; 1=two stop bits)
            clen: 1,                    //Character length (0=7-bit data; 1=8-bit data)
            listen: 0,                  //Listen enable (0=disabled; 1=enabled, feed tx back to receiver)
            mm: 0,                      //Multiprocessor mode (0=idle-line protocol; 1=address-bit protocol)
            ckpl: 0,                    //Clock polarity (0=normal; 1=inverted)
            urxse: 0,                   //Receive start-edge detection (0=disabled; 1=enabled)
            urxeie: 0,                  //Erroneous-character receive (0=rejected; 1=recieved and URXIFGx set)
            urxwie: 0,                  //Wake-up interrupt-enable (0=all characters set URXIFGx; 1=only address sets URXIFGx)
            utxe : 1,                   //1:enable tx module
            urxe : 1}                   //1:enable rx module
    };
    
    
    command error_t StdControl.start(){
        return call Resource.immediateRequest();
    }
    
    command error_t StdControl.stop(){
        call Resource.release();
        return SUCCESS;
    }
    
    event void Resource.granted(){}

    async command msp430_uart_union_config_t* Msp430UartConfigure.getConfig() {
        return &msp430_uart_wsn430_config;
    }
}
