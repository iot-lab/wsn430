// Modified 06/03/2012 by LASLA Noureddine (lnoureddine4@gmail.com)

module MotePlatformC {
	provides interface Init;
}
implementation {
	command error_t Init.init() {

		atomic
		{
			// disable interrupts
			P1IE = 0;
			P2IE = 0;

			// set for rising edge trigger
			P1IES = 0;
			P2IES = 0;

			// set all to gpio function
			P1SEL = 0;
			P2SEL = 0;
			P3SEL = 0;
			P4SEL = 0;
			P5SEL = 0;
			P6SEL = 0;

			P1OUT &= ~(0x08);
			P1OUT &= ~(0x10);
			P1OUT &= ~(0x40);

			//RESET
			P1DIR |= 0x80;
			//CCA   SFD  FIFOP  FIFO
			P1DIR &= ~(0x40 | 0x20 | 0x10 | 0x08);

			P2OUT = 0x30;
			P2DIR = 0x7b;

			P3OUT = 0x00;
			//VERGEN
			P3DIR |= 0x01;

			P4OUT = 0xdd;
			P4DIR = 0xfd;

			P5OUT = 0xff;
			P5DIR = 0xff;

			P6OUT = 0x00;
			P6DIR = 0xff;

			P1IE = 0;
			P2IE = 0;
		}
		return SUCCESS;
	}
}
