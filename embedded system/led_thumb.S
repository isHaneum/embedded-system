.syntax unified
.thumb

.equ LED1,         6              // equ는 #define 역할
.equ LED2,         7
.equ Button1,      9
.equ Button2,      8            

.extern SystemInit                // 클럭 설정

.section .text
.global led_thumb

.thumb_func

led_thumb:
    bl  SystemInit              // clk set
    bl  setup_gpio             // gpio
loop://while(1)역할
    ldr r0, =0x41004400        // r0 = port->group[0] 베이스주소 0x41004400
    ldr r1, [r0, #0x20]       // r1에 in.reg , 버튼 상태 확인

    //button1
	movs r2, #1//r2에 1저장
	lsls r2, r2, #Button1//bit연산으로 register값
    tst r1, r2    // and 연산으로 버튼1 확인
    bne  b1_high // z값 확인 후 분기 z= 0 -> low z= 1 -> high
b1_low:                         // LOW 라면
    ldr r2, =(1 << LED1)	//r2에 shift를 통해 pin match
    str r2, [r0, #0x18]   // led1 on #0x14 == 00010100
    b   check_b2
b1_high:                        // HIGH (버튼 안 눌림)
    ldr r2, =(1 << LED1)
    str r2, [r0, #0x14]   // LED1 off 00011000

	//button2
check_b2:
    movs r2, #1//위와 동일
	lsls r2, r2, #Button2
    tst r1, r2    // button2확인
    bne  b2_high//0
b2_low:                         // LOW
    ldr r2, =(1 << LED2)
    str r2, [r0, #0x18]   // led2 on #0x14 == 00010100
    b   loop
b2_high:                        // HIGH
    ldr r2, =(1 << LED2)
    str r2, [r0, #0x14]   // LED2 off
    b   loop                    // 끝날 때까지 루프

//
setup_gpio://gpio setting
    ldr r0, = 0x41004400//port base 주소

    // LED 출력 DIR = 1
    ldr r1, =((1 << LED1) | (1 << LED2))//led1,led2 합
    str r1, [r0, #0x08]   // 0x08 = DIRSET = 00001000

    //버튼은 입력DIR = 0
    ldr r1, =((1 << Button1) | (1 << Button2))
    str r1, [r0, #0x0C]   // 0x0C = DIRCLR = 00001100

	mov r3, r0//r3에 portbase 저장
	adds r3, #0x40//r3에 40더해서 pincfg base로 이동
    // PINCFG LED inen=0, pmuxen=0
    movs r1, #0
    strb r1, [r3, #LED1]//0x40 = pincfg base = 00100000
    strb r1, [r3, #LED2]

    // PINCFG button inen=1, pmuxen=0
    movs r1, #0x3//
    strb r1, [r3, #Button1]//1byte만
    strb r1, [r3, #Button2]

    // 내부 풀업: OUTSET = 1
    ldr r1, =((1 << Button1) | (1 << Button2))//r1에 button1,2 합연산 저장
    str r1, [r0, #0x18] //0x18 = outset 

    bx lr                        //to lr