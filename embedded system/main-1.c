#pragma GCC target ("thumb")

#include "sam.h"

void GCLK_setup();
void PORT_setup();
void TC3_setup();

int main()//1ms 
{

    SystemInit();
	
	GCLK_setup();
	
	TC3_setup(); 
	
	while (1) {
	
	};


	return (0);
}



void GCLK_setup() {
	
	SYSCTRL->OSC8M.bit.PRESC = 0;  // prescalar to 1
	SYSCTRL->OSC8M.bit.ONDEMAND = 0;
	SYSCTRL->OSC8M.bit.ENABLE = 1; // Enable


	GCLK->GENCTRL.bit.ID = 0; // Generator #0
	GCLK->GENCTRL.bit.SRC = 6; // OSC8M
	GCLK->GENCTRL.bit.OE = 1 ;  // Output Enable: GCLK_IO
	GCLK->GENCTRL.bit.GENEN = 1; // Generator Enable
	
	GCLK->CLKCTRL.bit.ID = 0x1B; // ID #ID (TcC3)
	GCLK->CLKCTRL.bit.GEN = 0; // Generator #0 selected for TC3
	GCLK->CLKCTRL.bit.CLKEN = 1; // Now, clock is supplied to TC3

}


void TC3_setup() {
	PORT->Group[0].PINCFG[18].reg = 0x41;		// peripheral mux: DRVSTR=1, PMUXEN = 1
	PORT->Group[0].PMUX[9].bit.PMUXE = 0x04;	// peripheral function E selected

	PM->APBCMASK.bit.TC3_ = 1 ; // Clock Enable (APBC clock) for TC3


	TC3->COUNT16.CTRLA.bit.MODE = 0;  // Count16 mode
	TC3->COUNT16.CTRLA.bit.WAVEGEN = 1 ; // Match PWM (MPWM)
	TC3->COUNT16.CTRLA.bit.PRESCALER = 4; // Timer Counter clock 500000 Hz = 8MHz / 16 thus tick is 2us
	TC3->COUNT16.CC[0].reg = 250;  // 2us*250 == 0.5 ms
	TC3->COUNT16.CTRLA.bit.ENABLE = 1 ;
}
