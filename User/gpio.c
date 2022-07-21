/*
 * gpio.c
 *
 *  Created on: Mar 24, 2022
 *      Author: ExplodingONC
 */

#include "gpio.h"

void Button_INT_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure = {0};
    EXTI_InitTypeDef EXTI_InitStructure = {0};
    NVIC_InitTypeDef NVIC_InitStructure = {0};

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* GPIOA ----> EXTI_Line0 */
    GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);
    EXTI_InitStructure.EXTI_Line = EXTI_Line0;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Rising; //按下为高电平，用上升沿
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_Init(&EXTI_InitStructure);

    NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1; //抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;        //子优先级
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

extern uint8_t warning_flag;
extern uint8_t crack_flag;
extern uint8_t crack_halt_flag;
extern uint8_t crack_repair_flag;
extern uint32_t time;
extern uint32_t time_stamp_trigger;
extern uint32_t time_stamp_record;
extern int32_t acc_record_100x;
void EXTI0_IRQHandler(void) __attribute__((interrupt("WCH-Interrupt-fast")));
void EXTI0_IRQHandler(void)
{
    EXTI_ClearFlag(EXTI_Line0); // 置中断标志位为零
    if (crack_halt_flag)
    {
        warning_flag = 0;
        crack_flag = 0;
        crack_halt_flag = 0;
        crack_repair_flag = 1;
        time = 0;
        time_stamp_trigger = 0;
        time_stamp_record = 0;
        acc_record_100x = 0;
    }
}
