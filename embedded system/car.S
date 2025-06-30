
    .section .text
    .global main

main:
    bl      SystemInit//시스템 초기화
    bl      GCLK_setup//클럭 설정
    bl      USART_setup//UART 통신 설정
    bl      TC3_setup //왼쪽 바퀴용 타이머 설정
    bl      TC4_setup// 오른쪽 바퀴
    bl      DIR_setup//방향 제어용 GPIO 출력 설정

    ldr     r0, =init_msg //메시지 출력
    bl      uart_print

loop:
    ldr     r1, =SERCOM5_USART_INTFLAG//RXC 플래그 확인
    ldrb    r2, [r1]
    tst     r2, #0x01//
    beq     loop

    ldr     r1, =SERCOM5_USART_DATA //수신 데이터 읽기
    ldrb    r3, [r1]//r3에 불러오고 1byte
    strb    r3, [r1]//echo back

    cmp     r3, #'1' // 전진
    beq     cmd_forward//분기
    cmp     r3, #'2'// 후진
    beq     cmd_backward//
    cmp     r3, #'3'//빠르게
    beq     cmd_speed_up
    cmp     r3, #'4'//느리게
    beq     cmd_speed_down
    cmp     r3, #'5' //좌회전
    beq     cmd_turn_left
    cmp     r3, #'6'// 우회전
    beq     cmd_turn_right
    cmp     r3, #'7' // 정지
    beq     cmd_stop//stop으로
    b       loop//계속 반복

cmd_forward://forward 함수
    bl      set_forward//먼저 방향
    ldr     r0, =left_speed//속도 설정
    ldr     r1, [r0]//
    bl      set_TC3_speed//속도 설정
    ldr     r0, =right_speed
    ldr     r1, [r0]
    bl      set_TC4_speed
    b       loop

cmd_backward:
    bl      set_backward//뒤로 설정
    ldr     r0, =left_speed//속도
    ldr     r1, [r0]
    bl      set_TC3_speed
    ldr     r0, =right_speed//속도
    ldr     r1, [r0]
    bl      set_TC4_speed
    b       loop

cmd_speed_up://빠르게
    ldr     r0, =left_speed//r0에 left주소
    ldr     r1, [r0]//저장
    add     r1, r1, #50//50 빠르게
    str     r1, [r0]//저장
    bl      set_TC3_speed

    ldr     r0, =right_speed//오른쪽도 똑같이
    ldr     r1, [r0]
    add     r1, r1, #50
    str     r1, [r0]
    bl      set_TC4_speed
    b       loop

cmd_speed_down:
    ldr     r0, =left_speed//50 느리게
    ldr     r1, [r0]//r0주소
    subs    r1, r1, #50//빼기
    str     r1, [r0]//저장
    bl      set_TC3_speed//tc3로 가서 바꾸기

    ldr     r0, =right_speed//
    ldr     r1, [r0]
    subs    r1, r1, #50
    str     r1, [r0]
    bl      set_TC4_speed
    b       loop

cmd_turn_left://좌회전
    ldr     r0, =left_speed        // 왼쪽 속도 줄이기
    ldr     r1, [r0]//주소 불러오고
    subs    r1, r1, #50//50빼기
    str     r1, [r0]
    bl      set_TC3_speed//변환

    ldr     r0, =right_speed       // 오른쪽 속도 올리기
    ldr     r1, [r0]//주소
    add     r1, r1, #50//50더하기
    str     r1, [r0]//저장
    bl      set_TC4_speed//변환
    b       loop

cmd_turn_right://우회전 좌회전이랑 반대로
    ldr     r0, =right_speed       // 오른쪽 속도 줄이기
    ldr     r1, [r0]
    subs    r1, r1, #50
    str     r1, [r0]
    bl      set_TC4_speed

    ldr     r0, =left_speed        // 왼쪽 속도 올리기
    ldr     r1, [r0]
    add     r1, r1, #50
    str     r1, [r0]
    bl      set_TC3_speed
    b       loop

cmd_stop://정지
    movs    r1, #0//속도 0으로 설정
    bl      set_TC3_speed
    bl      set_TC4_speed
    ldr     r0, =left_speed//left값에
    movs    r1, #100//기본 속도 100
    str     r1, [r0]//저장
    ldr     r0, =right_speed//right값에
    str     r1, [r0]//저장
    b       loop



    // ----- Setup Functions 구현 시작 -----

GCLK_setup: // GCLK 설정
    ldr     r0, =0x40000C20
    ldr     r1, =0x000600E0 // SRC = 6(OSC8M), GENEN = 1, OE = 1
    str     r1, [r0]

    ldr     r0, =0x40000C2C
    ldr     r1, =0x401B // TC3 (ID=0x1B), GEN=0, CLKEN=1
    strh    r1, [r0]
    ldr     r1, =0x401C // TC4 (ID=0x1C), GEN=0, CLKEN=1
    strh    r1, [r0]
    bx      lr

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

DIR_setup: // IN1~IN4 방향 출력 설정
    ldr     r0, =0x41004400
    ldr     r1, =0x0000E200 // PA14, PA09, PA08, PA15 출력 설정
    str     r1, [r0]
    bx      lr

set_TC3_speed: // r1 = 속도 값
    ldr     r0, =0x42002C1A      // TC3->CC[1] 주소
    strh    r1, [r0]             // r1 값을 하위 16비트로 저장
    bx      lr

set_TC4_speed: // r1 = 속도 값
    ldr     r0, =0x4200301A      // TC4->CC[1] 주소
    strh    r1, [r0]
    bx      lr