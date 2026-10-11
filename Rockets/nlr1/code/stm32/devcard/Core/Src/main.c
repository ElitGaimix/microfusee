/* USER CODE BEGIN Header */
/*
 * Diagnostic liaison AirPort :
 * RP1 / ELRS -> USART6 DMA -> STM32F401
 * STM32F401 -> USART2 -> PC
 *
 * USART6 : communication protocole par DMA + detection IDLE
 * USART2 : sortie printf pour les logs
 */
/* USER CODE END Header */

#include "main.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"
#include "radio/protocol.h"

#include <stdio.h>
#include <stdint.h>

void SystemClock_Config(void);
void Error_Handler(void);

/* Redirection de printf vers USART2. */
int _write(int file, char *ptr, int len)
{
    (void)file;

    if (len <= 0)
    {
        return 0;
    }

    if (HAL_UART_Transmit(&huart2,
                          (uint8_t *)ptr,
                          (uint16_t)len,
                          HAL_MAX_DELAY) != HAL_OK)
    {
        return -1;
    }

    return len;
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    MX_GPIO_Init();
    MX_DMA_Init();
    MX_USART6_UART_Init();
    MX_USART2_UART_Init();

    Protocol_Init();

    printf("\r\nDemarrage du protocole AirPort\r\n");

    while (1)
    {
        Protocol_Task();
    }
}

/*
 * Conserve ici les fonctions SystemClock_Config() et Error_Handler()
 * générées par STM32CubeMX pour ta configuration matérielle.
 */

void SystemClock_Config(void)
{
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
    RCC_OscInitStruct.HSIState = RCC_HSI_ON;
    RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
    RCC_OscInitStruct.PLL.PLLM = 16;
    RCC_OscInitStruct.PLL.PLLN = 336;
    RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
    RCC_OscInitStruct.PLL.PLLQ = 7;

    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        Error_Handler();
    }

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK |
                                  RCC_CLOCKTYPE_SYSCLK |
                                  RCC_CLOCKTYPE_PCLK1 |
                                  RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
    {
        Error_Handler();
    }
}

void Error_Handler(void)
{
    __disable_irq();

    while (1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
