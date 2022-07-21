/*
 * gpio.h
 *
 *  Created on: Mar 24, 2022
 *      Author: ExplodingONC
 */

#ifndef USER_GPIO_H_
#define USER_GPIO_H_

#include "debug.h"
#include "stdlib.h"

void Button_INT_Init(void);
void EXTI0_IRQHandler(void);

#endif /* USER_GPIO_H_ */
