    .syntax unified
    .cpu cortex-m0plus
    .thumb

    .section .text
    .global main

main:
    bl  SystemInit     // 시스템 초기화 호출
    bl  GCLK_setup     // 클록 설정
    bl  USART_setup    // UART 설정
    bl  PORT_setup     // 포트 및 초음파 트리거 설정
    bl  DIR_setup      // 모터 제어 핀 방향 설정
    bl  TC3_setup      // TC3 PWM 설정
    bl  TC4_setup      // TC4 PWM 설정

    ldr r0, =TC3_BASE  // TC3 베이스 주소 로드
    movs r1, #0        // 듀티 0
    strh r1, [r0, #0x18]  // CC[1] 레지스터에 저장
    ldr r0, =TC4_BASE  // TC4 베이스 주소 로드
    strh r1, [r0, #0x18]  // CC[1] 레지스터에 저장


main_loop:
    // 트리거 펄스 시작: PA7 = 0
    ldr r0, =PORT_BASE // 포트 베이스 주소 ldr
    movs r1, #0        // 0
    str  r1, [r0, #0x10] // OUT 레지스터
    bl   RTC_setup     // RTC 타이머 10us 비교

1:  ldr r0, =RTC_BASE  // RTC 베이스 주소 로드
    ldr r1, [r0, #0x18] // INTFLAG 레지스터 읽기
    tst r1, #1         // CMP0 플래그 검사
    beq 1b             // 0이면 대기
    movs r1, #1        // 플래그 클리어 값 1
    str  r1, [r0, #0x18] // 플래그 클리어
    movs r1, #0x80     // PA7 = 1
    str  r1, [r0, #0x10] // OUT 레지스터에 쓰기

2:  ldr r0, =RTC_BASE  
    ldr r1, [r0, #0x18] // INTFLAG 읽기
    tst r1, #1         // CMP0 flag
    beq 2b             // 대기
    movs r1, #1        // 플래그 clr
    str  r1, [r0, #0x18] // INTFLAG clr
    movs r1, #0        // PA7 = 0 (LOW)
    str  r1, [r0, #0x10] // OUT 레지스터에 쓰기

    bl   Distance      // 에코 측정
    mov  r4, r0        //거리 저장

    bl   set_forward   // 전진 설정

    movs r0, #10       // 10cm 이하
    cmp  r4, r0        // 거리 <= 10
    ble  _stop         // 정지
    movs r0, #20       // 10~20
    cmp  r4, r0        // 거리 <= 20
    ble  _low          // 저속 분기
    movs r0, #30       // 20~30
    cmp  r4, r0        // 거리 <= 30
    ble  _mid          // 중속 분기
    b    _high         //이외

 _stop://정지
    ldr r0, =TC3_BASE 
    movs r1, #0       // 듀티 0
    strh r1, [r0, #0x18] // TC3 CC1
    ldr r0, =TC4_BASE // TC4
    strh r1, [r0, #0x18] // 듀티 0 설정
    b    _delay       // 지연 후 루프

_low:// 저속 200
    ldr r0, =TC3_BASE // 왼쪽 모터
    movs r1, #200     // 듀티 값
    strh r1, [r0, #0x18] // CC1 2byte 저장
    ldr r0, =TC4_BASE // 오른쪽 모터
    strh r1, [r0, #0x18] // CC1 설정
    b    _delay       // 1초

_mid:
    // 중속 400
    ldr r0, =TC3_BASE
    movs r1, #400
    strh r1, [r0, #0x18]
    ldr r0, =TC4_BASE
    strh r1, [r0, #0x18]
    b    _delay

_high:
    // 고속 600
    ldr r0, =TC3_BASE
    movs r1, #600
    strh r1, [r0, #0x18] // TC3 CC1

_delay:
    //1초 지연
    movs r5, #0      // 루프 카운터 초기화
_delay_loop:
    adds r5, #1      // 카운터 +1
    cmp  r5, #250000 // 250K
    bne  _delay_loop // 반복
    b    main_loop   // 메인 루프로 복귀


GCLK_setup:
    // OSC8M PRESC=0, ONDEMAND=0
    ldr  r0, =SYSCTRL_OSC8M
    movs r1, #0
    str  r1, [r0]
    // GCLK0: SRC=6(8MHz), OE=1, GENEN=1
    ldr  r0, =GCLK_GENCTRL
    movs r1, #((6<<8)|(1<<16)|(1<<0))
    str  r1, [r0]
    // 채널 0x1B (TC3/4), 채널 0x1C (TC4/5) clk 공급
    ldr  r0, =GCLK_CLKCTRL
    movs r1, #(0x1B|(1<<7))
    strh r1, [r0]
    movs r1, #(0x1C|(1<<7))
    strh r1, [r0]
    bx   lr

/* USART_setup */
USART_setup: // UART 통신 설정
    ldr     r0, =0x41008114
    ldr     r1, =0xC504 // 115200bps @ 8MHz
    strh    r1, [r0]

    ldr     r0, =0x41008102
    ldr     r1, =0x30001C01 // MODE=1, RXPO=3, TXPO=1, DORD=1
    str     r1, [r0]

    ldr     r0, =0x41008104
    ldr     r1, =0x00030000 // RXEN=1, TXEN=1
    str     r1, [r0]

    ldr     r0, =0x41008100
    ldr     r1, =0x00000002 // ENABLE=1
    str     r1, [r0]
    bx      lr



// 포트 설정: PA7 트리거 출력, PA6 에코 입력
PORT_setup:
    // PA7 PINCFG=0 (GPIO), DIRSET=1<<7, OUT=0
    ldr  r0, =PORT_BASE+0x40+7
    movs r1, #0
    strb r1, [r0]
    ldr  r0, =PORT_BASE+0x08
    movs r1, #0x80
    str  r1, [r0]
    bx   lr

DIR_setup: // IN1~IN4 방향 출력 설정
    ldr     r0, =0x41004400
    ldr     r1, =0x0000E200 // PA14, PA09, PA08, PA15 출력 설정
    str     r1, [r0]
    bx      lr

/* TC3_setup */
TC3_setup: // TC3 타이머 설정 왼쪽
    ldr     r0, =0x42002C00
    ldr     r1, =0x00000603 // PRESCALER=6, WAVEGEN=3
    strh    r1, [r0]

    ldr     r0, =0x42002C18
    movs    r1, #232 // 주기 (CC[0])
    strh    r1, [r0]

    ldr     r0, =0x42002C1A
    movs    r1, #200 // 듀티 (CC[1])
    strh    r1, [r0]

    ldr     r0, =0x42002C00
    ldr     r1, =0x00000002
    strh    r1, [r0]
    bx      lr

TC4_setup: // TC4 타이머 설정 오른쪽
    ldr     r0, =0x42003000
    ldr     r1, =0x00000603
    strh    r1, [r0]

    ldr     r0, =0x42003018
    movs    r1, #232
    strh    r1, [r0]

    ldr     r0, =0x4200301A
    movs    r1, #200
    strh    r1, [r0]

    ldr     r0, =0x42003000
    ldr     r1, =0x00000002
    strh    r1, [r0]
    bx      lr

RTC_setup:
    ldr  r0, =RTC_BASE+0x00
    // Disable, MODE=0, MATCHCLR=1
    movs r1, #0x04
    str  r1, [r0]
    // COMP 레지스터에 80 (10usec)
    ldr  r0, =RTC_BASE+0x1C
    movs r1, #80
    str  r1, [r0]
    // COUNT 클리어
    ldr  r0, =RTC_BASE+0x10
    movs r1, #0
    str  r1, [r0]
    // ENABLE
    ldr  r0, =RTC_BASE+0x00
    movs r1, #1
    str  r1, [r0]
    bx   lr

Distance:
    // 에코 대기기
    ldr  r0, =PORT_BASE+0x14
_wait_high:
    ldr  r1, [r0]
    tst  r1, #0x40
    beq  _wait_high
    // 타이머 시작값 저장
    ldr  r2, =RTC_BASE+0x10
    ldr  r3, [r2]
    // 에코 하강 대기
_wait_low:
    ldr  r1, [r0]
    tst  r1, #0x40
    bne  _wait_low
    // 타이머 종료값 저장
    ldr  r4, [r2]
    // Δ = end - start
    subs r4, r4, r3
    // usec = rtc_ticks/8, 거리 카운트 = usec/2
    lsrs r4, r4, #4   // (divide by 16 for scale)
    mov  r0, r4       // 반환값 r0
    bx   lr
set_forward://forward 함수
    bl      set_forward//먼저 방향
    ldr     r0, =left_speed//속도 설정
    ldr     r1, [r0]//
    bl      set_TC3_speed//속도 설정
    ldr     r0, =right_speed
    ldr     r1, [r0]
    bl      set_TC4_speed
    b       loop
