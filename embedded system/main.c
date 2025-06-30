
#pragma  GCC target("thumb")

#include <sam.h>
#define LED1 6
#define LED2 7
#define Button1 9
#define Button2 8

//extern int led_thumb;
// 핀 번호 define


void setup_gpio(void) {//gpio setup 함수
	// 출력 설정
	PORT->Group[0].PINCFG[LED1].reg = 0x0;//pin6
	PORT->Group[0].PINCFG[LED2].reg = 0x0;//pin7 register inen=0, pmuxen=0
	PORT->Group[0].DIRSET.reg = (1 << LED1) | (1 << LED2);// |연산자로 합쳐서 저장,dir==1
	//입력 설정
	PORT->Group[0].PINCFG[Button1].reg = 0x2;//pin8
	PORT->Group[0].PINCFG[Button2].reg = 0x2;//pin9 inen =1, pmuxen=0
	PORT->Group[0].DIRCLR.reg = (1 << Button1) | (1 << Button2);//합쳐서 저장,dir == 0
	PORT->Group[0].OUTSET.reg = (1 << Button1) | (1 << Button2);//out == 1
}

int main(void) {
	SystemInit();//clk set
	int a = led_thumb();
	
	setup_gpio();//gpio setting

	while (1) {
		
		if ((PORT->Group[0].IN.reg & (1 << Button1)))  // 버튼1 눌리면 LOW
			PORT->Group[0].OUTCLR.reg = (1 << LED1);   // LED ON
		else
			PORT->Group[0].OUTSET.reg = (1 << LED1);   // LED OFF

		if ((PORT->Group[0].IN.reg & (1 << Button2)))// 버튼2 눌리면 high
			PORT->Group[0].OUTCLR.reg = (1 << LED2);//led on
		else
			PORT->Group[0].OUTSET.reg = (1 << LED2);//led off
			
	//return a;
	}
}

