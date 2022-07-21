/********************************** (C) COPYRIGHT *******************************
 * File Name          : main.c
 * Author             : ExplodingONC
 * Version            : V1.0.0
 * Date               : 2022/03/24
 * Description        : Main program body.
 * Copyright
 * SPDX-License-Identifier: Apache-2.0
 *******************************************************************************/

/*
 *@Note
 本例程演示使用 MPU6050来搞事
*/

#include "math.h"
#include "debug.h"
#include "timer.h"
#include "gpio.h"
#include "imu_mpu6050.h"
#include "lcd_st7789.h"
#include "images.h"

/* Global typedef */

/* Global define */
#define TRUE 1
#define FALSE 0

uint32_t sqrt32(uint32_t n);

/* Global Variable */
uint8_t tick = 0;
uint32_t time, time_stamp_trigger, time_stamp_record;
uint8_t refresh_flag = 0;

int16_t raw_acc[3], raw_gyro[3];
float acc[3], gyro[3];
int32_t raw_acc_total, raw_gyro_total;
int32_t acc_total_100x, gyro_total_10x;
int32_t acc_record_100x;
int32_t acc_threshold_warning_100x = 2500;
int32_t acc_threshold_crack_100x = 10000;

uint8_t warning_flag = 0, crack_flag = 0, crack_halt_flag = 0, crack_repair_flag = 0;

/*********************************************************************
 * @fn      main
 *
 * @brief   Main program.
 *
 * @return  none
 */
int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    Delay_Init();
    USART_Printf_Init(115200);

    // report clock rate
    u32 SystemCoreClock_MHz = SystemCoreClock / 1000000;
    u32 SystemCoreClock_kHz = (SystemCoreClock % 1000000) / 1000;
    printf("System Clock Speed = %d.%03dMHz\r\n", SystemCoreClock_MHz, SystemCoreClock_kHz);

    LCD_Init();
    LCD_SetBrightness(75);
    crack_repair_flag = 1;

    IMU_Init();
    IMU_SetGyroFsr(0x02);
    IMU_SetAccelFsr(0x03);
    IMU_SetRate(1000);
    IMU_SetLPF(500);

    tick = 1;
    time = 0;
    refresh_flag = 1;
    Button_INT_Init(); // wakeup button
    Tick_TIM_Init(1000 - 1, SystemCoreClock_MHz - 1);
    Tick_TIM_INT_Init(); // 1kHz tick & time
    Refresh_TIM_Init(33333 - 1, SystemCoreClock_MHz - 1);
    Refresh_TIM_INT_Init(); // 30Hz refresh_flag
    printf("Display: Timer & IMU data\r\n");
    printf("\r\n");

    while (1)
    {
        Delay_Us(10);

        // executed every 1ms
        if (tick)
        {
            if (!crack_flag) // pause data collection if crack happens
            {
                // data collection
                IMU_GetAccelerometer(&acc[0], &acc[1], &acc[2]);
                IMU_GetGyroscope(&gyro[0], &gyro[1], &gyro[2]);
                acc_total_100x = sqrt32((int)(10000 * (acc[0] * acc[0] + acc[1] * acc[1] + acc[2] * acc[2])));
                gyro_total_10x = sqrt32((int)(100 * (gyro[0] * gyro[0] + gyro[1] * gyro[1] + gyro[2] * gyro[2])));
                // IMU warning
                if (acc_total_100x > acc_threshold_warning_100x)
                {
                    time_stamp_trigger = time;
                    warning_flag = 1;
                    if ((acc_total_100x > acc_record_100x) || (time - time_stamp_record > 1000))
                    {
                        time_stamp_record = time;
                        acc_record_100x = acc_total_100x;
                    }
                }
                else if (time - time_stamp_trigger >= 1000)
                {
                    warning_flag = 0;
                    acc_record_100x = 0;
                }
                // IMU glass crack
                if ((acc_total_100x > acc_threshold_crack_100x))
                {
                    // direction record (for crack rotation)
                    if (abs(acc[0]) > abs(acc[1]))
                    {
                        if (acc[0] > 0)
                        { // east
                            crack_flag = 0x10;
                            if (acc[1] > 0)
                                crack_flag |= 0x02; // east-north
                            else
                                crack_flag |= 0x08; // east-south
                        }
                        else
                        { // west
                            crack_flag = 0x40;
                            if (acc[1] > 0)
                                crack_flag |= 0x02; // west-north
                            else
                                crack_flag |= 0x08; // west-south
                        }
                    }
                    else
                    {
                        if (acc[1] > 0)
                        { // north
                            crack_flag = 0x20;
                            if (acc[0] > 0)
                                crack_flag |= 0x01; // north-east
                            else
                                crack_flag |= 0x04; // north-west
                        }
                        else
                        { // south
                            crack_flag = 0x80;
                            if (acc[0] > 0)
                                crack_flag |= 0x01; // south-east
                            else
                                crack_flag |= 0x04; // south-west
                        }
                    }
                }
            }
            // clear flag
            tick = 0;
        }

        // executed every 33ms
        if (refresh_flag && !crack_halt_flag)
        {
            // IMU glass cracking repair
            // display before warning message
            if (crack_repair_flag)
            {
                LCD_Clear(BLACK);
                LCD_SetColor(0x18E3, GRAY);
                LCD_DrawRectangle(40, 80, 200, 160, TRUE);
                crack_repair_flag = 0;
            }

            // normal data refresh
            // IMU data
            LCD_SetColor(BLACK, GREEN);
            for (u8 i = 0; i < 3; i++)
            {
                int32_t acc10x = (int)(acc[i] * 10);
                if (acc10x > 9999)
                    acc10x = 9999;
                if (acc10x < -9999)
                    acc10x = -9999;
                LCD_ShowString(16, 16 * (i + 1), 16, TRUE, "%c:%+4d.%01d", 'X' + i, acc10x / 10, abs(acc10x % 10));
                LCD_ShowString(84, 16 * (i + 1), 16, TRUE, "m/s2");
                int32_t gyro10x = (int)(gyro[i] * 10);
                if (gyro10x > 9999)
                    gyro10x = 9999;
                if (gyro10x < -9999)
                    gyro10x = -9999;
                LCD_ShowString(128, 16 * (i + 1), 16, TRUE, "G%c:%+4d.%01d", 'X' + i, gyro10x / 10, abs(gyro10x % 10));
                LCD_ShowString(204, 16 * (i + 1), 16, TRUE, "d/s");
            }
            // Timer
            LCD_SetColor(BLACK, YELLOW);
            LCD_ShowString(16, 180, 16, TRUE, "There has been NO ACCIDENT");
            LCD_ShowString(20, 200, 24, TRUE, "since");
            LCD_ShowString(84, 200, 24, TRUE, ":%7ds", time / 1000);
            LCD_ShowString(196, 204, 16, TRUE, "ago");
            // Warning display
            if (warning_flag)
                LCD_SetColor(0x18E3, RED);
            else
                LCD_SetColor(0x18E3, GRAY);
            LCD_DrawRectangle(40, 80, 200, 160, FALSE);
            LCD_ShowString(64, 120, 32, TRUE, "WARNING");
            // IMU total data
            if (warning_flag)
            {
                LCD_SetColor(0x18E3, BLUE);
                if (acc_record_100x > 99999)
                    acc_record_100x = 99999;
                LCD_ShowString(56, 92, 24, TRUE, "%3d.%02d m/s2", acc_record_100x / 100, abs(acc_record_100x % 100));
            }
            else
            {
                LCD_SetColor(0x18E3, CYAN);
                if (acc_total_100x > 99999)
                    acc_total_100x = 99999;
                LCD_ShowString(56, 92, 24, TRUE, "%3d.%02d m/s2", acc_total_100x / 100, abs(acc_total_100x % 100));
            }

            // IMU glass cracking display
            // display after warning message
            if (crack_flag)
            {
                switch (crack_flag)
                {
                case 0x12: // east-north
                    LCD_OverlayImage(0, 0, LCD_W, LCD_H, 0, TRUE, img_crack);
                    break;
                case 0x18: // east-south
                    LCD_OverlayImage(0, 0, LCD_W, LCD_H, 0, FALSE, img_crack);
                    break;
                case 0x42: // west-north
                    LCD_OverlayImage(0, 0, LCD_W, LCD_H, 2, FALSE, img_crack);
                    break;
                case 0x48: // west-south
                    LCD_OverlayImage(0, 0, LCD_W, LCD_H, 2, TRUE, img_crack);
                    break;
                case 0x21: // north-east
                    LCD_OverlayImage(0, 0, LCD_W, LCD_H, 1, FALSE, img_crack);
                    break;
                case 0x24: // north-west
                    LCD_OverlayImage(0, 0, LCD_W, LCD_H, 1, TRUE, img_crack);
                    break;
                case 0x81: // south-east
                    LCD_OverlayImage(0, 0, LCD_W, LCD_H, 3, TRUE, img_crack);
                    break;
                case 0x84: // south-west
                    LCD_OverlayImage(0, 0, LCD_W, LCD_H, 3, FALSE, img_crack);
                    break;
                default:
                    LCD_OverlayImage(0, 0, LCD_W, LCD_H, 0, FALSE, img_crack);
                }
                LCD_SetColor(BLACK, CYAN);
                LCD_ShowString(16, 156, 16, FALSE, "Press            to repair");
                LCD_ShowString(64, 152, 24, FALSE, "Wake_Up");
                crack_halt_flag = 1;
            }
            // clear flag
            refresh_flag = 0;
        }
    }
}

// square root function for int32
uint32_t sqrt32(uint32_t n)
{
    uint32_t c = 0x8000;
    uint32_t g = 0x8000;
    for (;;)
    {
        if (g * g > n)
            g ^= c;
        c >>= 1;
        if (c == 0)
            return g;
        g |= c;
    }
    return 0;
}
