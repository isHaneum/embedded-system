
#pragma GCC target ("thumb")

#include "sam.h"
#include "uart_print.h"
void GCLK_setup();//8MHz 내부 osc-> rtc,tc
void USART_setup();//uart mapping
void PORT_setup();//pa7 출력, pa6 echo 입력
void DIR_setup() {//in1~4 2,3,4,5 -> pa14,9,8,15
	PORT->Group[0].DIRSET.reg = (1 << 14) | (1 << 9) | (1 << 8) | (1 << 15);//2,3,4,5 port 출력으로
}
void TC3_setup();//왼쪽 바퀴 출력 타이머 16bit mode on
void TC4_setup();//오른쪽 바퀴
void EIC_setup();//외부 interrupt 설정
void EIC_Handler();//초음파 echo 핀 두 번 호출, 거리 계산
void set_forward();//앞 방향으로 설정
void TC5_setup();//1초 timer setup
void TC5_Handler();//1초마다 10us pulse 발생시키는 interrupt

volatile uint32_t count_start, count_end;
volatile int distance_ready = 0;
volatile int last_distance_cm = 0;


int main()
{
	SystemInit();
	GCLK_setup();
	USART_setup();
	PORT_setup();
	DIR_setup();
	TC3_setup();
	TC4_setup();
	EIC_setup();
	RTC_setup();     // RTC 초기 설정
	TC5_setup();

	TC3->COUNT16.CC[1].reg = 0; //왼쪽 속도 0
	TC4->COUNT16.CC[1].reg = 0; //오른쪽 속도 0

	set_forward();   //앞으로 설정

	while (1) {
		if (distance_ready) {  // 인터럽트 발생 후에만 처리
			int dist = last_distance_cm; 

			if (dist <= 10) {
				TC3->COUNT16.CC[1].reg = 0;//정지
				TC4->COUNT16.CC[1].reg = 0;
			}
			else if (dist <= 20) {
				TC3->COUNT16.CC[1].reg = 200;//저속
				TC4->COUNT16.CC[1].reg = 200;
			}
			else if (dist <= 30) {
				TC3->COUNT16.CC[1].reg = 400;//중속
				TC4->COUNT16.CC[1].reg = 400;
			}
			else {
				TC3->COUNT16.CC[1].reg = 600;//고속
				TC4->COUNT16.CC[1].reg = 600;
			}
			while (TC4->COUNT16.STATUS.bit.SYNCBUSY);

			distance_ready = 0;  //처리 후 초기화
		}
	}
	return 0;
}
void set_forward() { // 자동차 앞으로
	PORT->Group[0].OUTSET.reg = 1 << 14;//14 on
	PORT->Group[0].OUTSET.reg = 1 << 8;//8 on
	PORT->Group[0].OUTCLR.reg = 1 << 9;// 9 off
	PORT->Group[0].OUTCLR.reg = 1 << 15;// 15 off

}
void GCLK_setup() {
	
	// OSC8M
	//SYSCTRL->OSC8M.bit.ENABLE = 0; // Disable
	SYSCTRL->OSC8M.bit.PRESC = 0;  // prescaler to 1
	SYSCTRL->OSC8M.bit.ONDEMAND = 0;
	//SYSCTRL->OSC8M.bit.ENABLE = 1; // Enable

	//
	// Generic Clock Controller setup for RTC
	// * RTC ID: #4
	// * Generator #0 is feeding RTC
	// * Generator #0 is taking the clock source #6 (OSC8M: 8MHz clock input) as an input

	GCLK->GENCTRL.bit.ID = 0; // Generator #0
	GCLK->GENCTRL.bit.SRC = 6; // OSC8M
	GCLK->GENCTRL.bit.OE = 1 ;  // Output Enable: GCLK_IO
	GCLK->GENCTRL.bit.GENEN = 1; // Generator Enable
	
	GCLK->CLKCTRL.bit.ID = 4; // ID #4 (RTC)
	GCLK->CLKCTRL.bit.GEN = 0; // Generator #0 selected for RTC
	GCLK->CLKCTRL.bit.CLKEN = 1; // Now, clock is supplied to RTC!
	
	GCLK->CLKCTRL.bit.ID = 0x1B; // ID #ID (TCC2, TC3)
	GCLK->CLKCTRL.bit.GEN = 0; // Generator #0 selected for TCC2, TC3
	GCLK->CLKCTRL.bit.CLKEN = 1; // Now, clock is supplied to TCC2, TC3
	
	GCLK->CLKCTRL.bit.ID = 0x1C; // ID #ID (TC4, TC5)
	GCLK->CLKCTRL.bit.GEN = 0; // Generator #0 selected for TC4, TC5
	GCLK->CLKCTRL.bit.CLKEN = 1; // Now, clock is supplied to TC4, TC5
	while (GCLK->STATUS.bit.SYNCBUSY);

}

void PORT_setup() {
	

	PORT->Group[0].PINCFG[7].reg = 0x0;		// PMUXEN = 0
	PORT->Group[0].DIRSET.reg = 0x1 << 7;		// Direction: Output
	PORT->Group[0].OUT.reg = 0 << 7 ;          // Set the Trigger to 0

	//
	// PORT setup for PA19 to take the echo input from Ultrasonic sensor
	//
	PORT->Group[0].PINCFG[6].reg = 0x2;		// INEN = 1, PMUXEN = 0
	PORT->Group[0].DIRCLR.reg = 0x1 << 6;		// Direction: Input

}

void TC3_setup() {
	// PORT setup for PA18 ( TC3's WO[0] )
	PORT->Group[0].PINCFG[18].reg = 0x41;		// peripheral mux: DRVSTR=1, PMUXEN = 1
	PORT->Group[0].PMUX[9].bit.PMUXE = 0x04;	// peripheral function E selected

	// PORT setup for PA19 ( TC3's WO[1] )
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

	TC4->COUNT16.CC[0].reg = 1000;  // CC0 defines the period
	TC4->COUNT16.CC[1].reg = 200;  // CC1 match pulls down WO[1]
	TC4->COUNT16.CTRLA.bit.ENABLE = 1 ;
	
}
void RTC_setup() {
	//
	// RTC setup: MODE0 (32-bit counter) with COMPARE 0
	//
	RTC->MODE0.CTRL.bit.ENABLE = 0; // Disable first
	RTC->MODE0.CTRL.bit.MODE = 0; // Mode 0
	RTC->MODE0.CTRL.bit.MATCHCLR = 1; // match clear
	
	// 8MHz RTC clock  --> 10 usec when 80 is counted
	RTC->MODE0.COMP->reg = 80; // compare register to set up 10usec interval
	RTC->MODE0.COUNT.reg = 0x0; // initialize the counter to 0
	RTC->MODE0.CTRL.bit.ENABLE = 1; // Enable
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

	SERCOM5->USART.BAUD.reg = 0Xc504 ; // 115,200 bps (baud rate) with 8MHz input clock
	SERCOM5->USART.CTRLB.bit.RXEN = 1 ;
	SERCOM5->USART.CTRLB.bit.TXEN = 1 ;

	SERCOM5->USART.CTRLA.bit.ENABLE = 1;
}

void EIC_setup() {
	PORT->Group[0].PINCFG[6].reg |= 0x02; // PA6 INEN = 1
	PORT->Group[0].PINCFG[6].reg |= 0x01; // PMUXEN = 1
	PORT->Group[0].PMUX[3].bit.PMUXE = 0; // PA6 → EXTINT[6], Function A

	GCLK->CLKCTRL.bit.ID = 0x05;     // EIC ID
	GCLK->CLKCTRL.bit.GEN = 0;       // Generator 0 사용
	GCLK->CLKCTRL.bit.CLKEN = 1;

	EIC->CONFIG[0].bit.SENSE6 = 0x3; // Both edges 감지
	EIC->CONFIG[0].bit.FILTEN6 = 1;  // Noise filter ON
	EIC->INTENSET.bit.EXTINT6 = 1;   // EXTINT6 활성화
	EIC->CTRL.bit.ENABLE = 1;

	NVIC->ISER[0] = 1 << EIC_IRQn;   // NVIC 등록
}

void EIC_Handler(void) {
	if (EIC->INTFLAG.bit.EXTINT6) {  // PA6 인터럽트
		EIC->INTFLAG.bit.EXTINT6 = 1; // 인터럽트 flag

		if (!distance_ready) {        // 이전 거리 측정 확인
			static int state = 0;     // 상태 저장
			if (state == 0) {
				count_start = RTC->MODE0.COUNT.reg; // 시작 시점 저장
				state = 1;                           // 다음번엔 끝 저장
				} else {
				count_end = RTC->MODE0.COUNT.reg;   // 끝 시점 저장
				uint32_t usec = (count_end - count_start) / 8; // tick
				last_distance_cm = (usec / 2) * 0.034;         // /2

				print_decimal(last_distance_cm / 100);        //100
				print_decimal((last_distance_cm / 10) % 10);  //10
				print_decimal(last_distance_cm % 10);         //1
				print_enter();                                //\n

				distance_ready = 1;   // 거리 측정 완료 표시
				state = 0;            // 다음 측정 준비
			}
		}
	}
}

void TC5_setup() {
	PM->APBCMASK.bit.TC5_ = 1;  // TC5 clk 공급

	GCLK->CLKCTRL.bit.ID = 0x1C; //
	GCLK->CLKCTRL.bit.GEN = 0;   // Generator 0
	GCLK->CLKCTRL.bit.CLKEN = 1; // 클럭 set

	TC5->COUNT16.CTRLA.reg =
	TC_CTRLA_MODE_COUNT16 |        // 16비트
	TC_CTRLA_WAVEGEN_MFRQ |        // 정해진 주기로 인터럽트 발생
	TC_CTRLA_PRESCALER_DIV1024;    // 8MHz / 1024

	TC5->COUNT16.CC[0].reg = 7812; // 1초에 한 번 인터럽트 나오게 설정
	while (TC5->COUNT16.STATUS.bit.SYNCBUSY); // 동기화 기다림

	TC5->COUNT16.INTENSET.bit.MC0 = 1; // CC0 비교 매치 인터럽트 허용
	NVIC->ISER[0] = 1 << TC5_IRQn;    // NVIC에 TC5 인터럽트 등록

	TC5->COUNT16.CTRLA.bit.ENABLE = 1; // TC5 시작
	while (TC5->COUNT16.STATUS.bit.SYNCBUSY); // 동기화 대기
}

void TC5_Handler(void) {
	TC5->COUNT16.INTFLAG.bit.MC0 = 1; // 인터럽트 플래그 clr

	PORT->Group[0].OUTSET.reg = 1 << 7;   // PA7 HIGH
	for (volatile int i = 0; i < 80; i++); //10us 대기 (80 loop)
	PORT->Group[0].OUTCLR.reg = 1 << 7;   // PA7 LOW

	// RTC 초기화 → Echo 신호 시간 측정용
	RTC->MODE0.CTRL.bit.ENABLE = 0;     // 꺼줌
	RTC->MODE0.CTRL.bit.MATCHCLR = 0;   // matchclr off
	RTC->MODE0.COUNT.reg = 0;           // 0으로 초기화
	RTC->MODE0.CTRL.bit.ENABLE = 1;     // re on

	distance_ready = 0; // reset
}