
#include "sam.h"
#include <stdbool.h>

void GCLK_setup();
void USART_setup();
void TC3_setup();//왼쪽바퀴
void TC4_setup();//오른쪽바퀴
void uart_print(const char* str);//uart 출력
void DIR_setup() {//in1~4 방향 설정
	PORT->Group[0].DIRSET.reg = (1 << 14) | (1 << 9) | (1 << 8) | (1 << 15);//2,3,4,5 port
}

void set_forward() {// 앞방향
	// IN1 = HIGH, IN2 = LOW
	PORT->Group[0].OUTSET.reg = (1 << 14);   // PA14 (IN1) 핀 2
	PORT->Group[0].OUTCLR.reg = (1 << 9);    // PA09 (IN2)3

	// IN3 = HIGH, IN4 = LOW
	PORT->Group[0].OUTSET.reg = (1 << 8);    // PA08 (IN3)4
	PORT->Group[0].OUTCLR.reg = (1 << 15);   // PA15 (IN4)5
}

void set_backward() {//뒷방향
	// IN1 = LOW, IN2 = HIGH
	PORT->Group[0].OUTCLR.reg = (1 << 14);   // PA14 (IN1)2
	PORT->Group[0].OUTSET.reg = (1 << 9);    // PA09 (IN2)3

	// IN3 = LOW, IN4 = HIGH
	PORT->Group[0].OUTCLR.reg = (1 << 8);    // PA08 (IN3)4
	PORT->Group[0].OUTSET.reg = (1 << 15);   // PA15 (IN4)5
}
#define STEP 50;//변화량

int main() {
	
    unsigned char rx_data;//uart 입력
    bool rxc_flag;// 수신 여부
	
    int left_speed = 100;//왼쪽 
    int right_speed = 100;//오른쪽
	
    SystemInit();//시스템초기화
    GCLK_setup();//타이머클럭
    USART_setup();//uart
    TC3_setup();//왼쪽 바퀴
    TC4_setup();// 오른쪽 바퀴
	DIR_setup();//방향 설정
	
	
    uart_print("----------------- Vehicle Control ----------------\r\n");//teraterm에 출력
    uart_print("1. Forward     2. Backward\r\n");
    uart_print("3. Speed Up    4. Speed Down\r\n");
    uart_print("5. Left turn   6. Right turn   7. Stop\r\n");
    uart_print("--------------------------------------------------\r\n");
    uart_print("Select: ");

while(1) { // 계속 루프
	rxc_flag = SERCOM5->USART.INTFLAG.bit.RXC; // 수신 여부check
	if (rxc_flag) {
		rx_data = SERCOM5->USART.DATA.reg; // 수신 데이터
		SERCOM5->USART.DATA.reg = rx_data; // echo back

		switch (rx_data) {//switch로 입력 처리
			case '1':
			set_forward();//앞방향
			TC3->COUNT16.CC[1].reg = left_speed;//왼쪽바퀴 속도 설정
			TC4->COUNT16.CC[1].reg = right_speed;//오른쪽바퀴
			break;
			case '2':
			set_backward();//뒷방향
			TC3->COUNT16.CC[1].reg = left_speed;
			TC4->COUNT16.CC[1].reg = right_speed;
			break;
			case '3':
			left_speed += STEP;//속도 증가
			right_speed += STEP;//
			TC3->COUNT16.CC[1].reg = left_speed;
			TC4->COUNT16.CC[1].reg = right_speed;
			break;
			case '4':
			left_speed -= STEP;//속도 감소
			right_speed -= STEP;
			TC3->COUNT16.CC[1].reg = left_speed;
			TC4->COUNT16.CC[1].reg = right_speed;
			break;
			case '5'://좌회전
			right_speed += STEP;
			left_speed -= STEP;
			TC4->COUNT16.CC[1].reg = right_speed;//오른쪽을 빠르게-> 왼쪽 회전
			TC4->COUNT16.CC[1].reg = left_speed;//왼쪽도 느리게
			break;
			case '6'://우회전
			left_speed += STEP;
			right_speed -= STEP;//
			TC3->COUNT16.CC[1].reg = left_speed;//왼쪽빠르게 -> 오른쪽으로 회전		
			TC4->COUNT16.CC[1].reg = right_speed;//오른쪽 느리게
			break;
			case '7':
			TC3->COUNT16.CC[1].reg = 0;//속도 0
		    TC4->COUNT16.CC[1].reg = 0;
			left_speed = 100;//기본 값 초기화
			right_speed = 100;
			break;
			default:
			break;
		}
		uart_print("\r\nSelect: "); // 다음 입력 요청
	}
}
return 0;
}

void uart_print(const char* str) {//teraterm에 출력
    while (*str) {
        while (!(SERCOM5->USART.INTFLAG.bit.DRE));
        SERCOM5->USART.DATA.reg = *str++;
    }
}






void GCLK_setup() {
	
	// OSC8M
	//SYSCTRL->OSC8M.bit.ENABLE = 0; // Disable
	SYSCTRL->OSC8M.bit.PRESC = 0;  // prescalar to 1
	SYSCTRL->OSC8M.bit.ONDEMAND = 0;
	//SYSCTRL->OSC8M.bit.ENABLE = 1; // Enable

	//
	// Generic Clock Controller setup for TC3
	// * TC3 ID: #0x1B
	// * Generator #0 is feeding TC3
	// * Generator #0 is taking the clock source #6 (OSC8M: 8MHz clock input) as an input
	//
	GCLK->GENCTRL.bit.ID = 0; // Generator #0
	GCLK->GENCTRL.bit.SRC = 6; // OSC8M
	GCLK->GENCTRL.bit.OE = 1 ;  // Output Enable: GCLK_IO
	GCLK->GENCTRL.bit.GENEN = 1; // Generator Enable
	
	GCLK->CLKCTRL.bit.ID = 0x1B; // ID #ID (TCC2, TC3)
	GCLK->CLKCTRL.bit.GEN = 0; // Generator #0 selected for TCC2, TC3
	GCLK->CLKCTRL.bit.CLKEN = 1; // Now, clock is supplied to TCC2, TC3
	
	GCLK->CLKCTRL.bit.ID = 0x1C; // ID #ID (TC4, TC5)
	GCLK->CLKCTRL.bit.GEN = 0; // Generator #0 selected for TC4, TC5
	GCLK->CLKCTRL.bit.CLKEN = 1; // Now, clock is supplied to TC4, TC5
	

}




void TC3_setup() {

	//
	// PORT setup for PA18 ( TC3's WO[0] )
	//
	PORT->Group[0].PINCFG[18].reg = 0x41;		// peripheral mux: DRVSTR=1, PMUXEN = 1
	PORT->Group[0].PMUX[9].bit.PMUXE = 0x04;	// peripheral function E selected

	//
	// PORT setup for PA19 ( TC3's WO[1] )
	//
	PORT->Group[0].PINCFG[19].reg = 0x41;		// peripheral mux: DRVSTR=1, PMUXEN = 1
	PORT->Group[0].PMUX[9].bit.PMUXO = 0x04;	// peripheral function E selected

	// Power Manager
	PM->APBCMASK.bit.TC3_ = 1 ; // Clock Enable (APBC clock) for TC3

	//
	// TC3 setup: 16-bit Mode
	//

	TC3->COUNT16.CTRLA.bit.MODE = 0;  // Count16 mode
	TC3->COUNT16.CTRLA.bit.WAVEGEN = 3 ; // Match PWM (MPWM)
	TC3->COUNT16.CTRLA.bit.PRESCALER = 6; // Timer Counter clock 31,250 Hz = 8MHz / 256
	//TC3->COUNT16.CC[0].reg = 30000;  // CC0 defines the period
	//TC3->COUNT16.CC[1].reg = 10000;  // CC1 match pulls down WO[1]
	TC3->COUNT16.CC[0].reg = 1000;  // CC0 defines the period
	TC3->COUNT16.CC[1].reg = 200;  // CC1 match pulls down WO[1]
	TC3->COUNT16.CTRLA.bit.ENABLE = 1 ;
}


void TC4_setup() {

	//
	// PORT setup for PA22 ( TC4's WO[0] )
	//
	PORT->Group[0].PINCFG[22].reg = 0x41;		// peripheral mux: DRVSTR=1, PMUXEN = 1
	PORT->Group[0].PMUX[11].bit.PMUXE = 0x04;	// peripheral function E selected

	//
	// PORT setup for PA23 ( TC4's WO[1] )
	//
	PORT->Group[0].PINCFG[23].reg = 0x41;		// peripheral mux: DRVSTR=1, PMUXEN = 1
	PORT->Group[0].PMUX[11].bit.PMUXO = 0x04;	// peripheral function E selected

	// Power Manager
	PM->APBCMASK.bit.TC4_ = 1 ; // Clock Enable (APBC clock) for TC3

	//
	// TC4 setup: 16-bit Mode
	//

	TC4->COUNT16.CTRLA.bit.MODE = 0;  // Count16 mode
	TC4->COUNT16.CTRLA.bit.WAVEGEN = 3 ; // Match PWM (MPWM)
	TC4->COUNT16.CTRLA.bit.PRESCALER = 6; // Timer Counter clock 31,250 Hz = 8MHz / 256
	//TC3->COUNT16.CC[0].reg = 30000;  // CC0 defines the period
	//TC3->COUNT16.CC[1].reg = 10000;  // CC1 match pulls down WO[1]
	TC4->COUNT16.CC[0].reg = 1000;  // CC0 defines the period
	TC4->COUNT16.CC[1].reg = 200;  // CC1 match pulls down WO[1]
	TC4->COUNT16.CTRLA.bit.ENABLE = 1 ;
}



void USART_setup() {
	
	//
	// PORT setup for PB22 and PB23 (USART)
	//
	PORT->Group[1].PINCFG[22].reg = 0x41; // peripheral mux: DRVSTR=1, PMUXEN = 1
	PORT->Group[1].PINCFG[23].reg = 0x41; // peripheral mux: DRVSTR=1, PMUXEN = 1

	PORT->Group[1].PMUX[11].bit.PMUXE = 0x03; // peripheral function D selected
	PORT->Group[1].PMUX[11].bit.PMUXO = 0x03; // peripheral function D selected

	// Power Manager
	PM->APBCMASK.bit.SERCOM5_ = 1 ; // Clock Enable (APBC clock) for USART
	
	//
	// * SERCOM5: USART
	// * Generator #0 is feeding USART as well
	//
	GCLK->CLKCTRL.bit.ID = 0x19; // ID #0x19 (SERCOM5: USART): GCLK_SERCOM3_CORE
	GCLK->CLKCTRL.bit.GEN = 0; // Generator #0 selected for USART
	GCLK->CLKCTRL.bit.CLKEN = 1; // Now, clock is supplied to USART!

	GCLK->CLKCTRL.bit.ID = 0x13; // ID #0x13 (SERCOM5: USART): GCLK_SERCOM_SLOW
	GCLK->CLKCTRL.bit.GEN = 0; // Generator #0 selected for USART
	GCLK->CLKCTRL.bit.CLKEN = 1; // Now, clock is supplied to USART!
	
	//
	// USART setup
	//
	SERCOM5->USART.CTRLA.bit.MODE = 1 ; // Internal Clock
	SERCOM5->USART.CTRLA.bit.CMODE = 0 ; // Asynchronous UART
	SERCOM5->USART.CTRLA.bit.RXPO = 3 ; // PAD3
	SERCOM5->USART.CTRLA.bit.TXPO = 1 ; // PAD2
	SERCOM5->USART.CTRLB.bit.CHSIZE = 0 ; // 8-bit data
	SERCOM5->USART.CTRLA.bit.DORD = 1 ; // LSB first
	//SERCOM5->USART.CTRLA.bit.SAMPR = 1 ; //

	SERCOM5->USART.BAUD.reg = 0Xc504 ; // 115,200 bps (baud rate) with 8MHz input clock
	//SERCOM5->USART.BAUD.reg = 0Xe282 ; // 115,200 bps (baud rate) with 16MHz input clock
	//SERCOM5->USART.BAUD.reg = 0Xec57 ; // 115,200 bps (baud rate) with 24MHz input clock
	//SERCOM5->USART.BAUD.reg = 0Xf62b ; // 115,200 bps (baud rate) with 48MHz input clock4

	SERCOM5->USART.CTRLB.bit.RXEN = 1 ;
	SERCOM5->USART.CTRLB.bit.TXEN = 1 ;

	SERCOM5->USART.CTRLA.bit.ENABLE = 1;
	
}