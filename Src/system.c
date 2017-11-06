#include "stm32f1xx_hal.h"
#include "system.h"

/** System Clock Configuration
*/
extern uint32_t    __vector_table[];

void SYS_init(void)
{
    /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
    SCB->VTOR = (uint32_t)__vector_table;
        
    HAL_Init();
    
    RCC_OscInitTypeDef RCC_OscInitStruct;
    RCC_ClkInitTypeDef RCC_ClkInitStruct;

    /**Initializes the CPU, AHB and APB busses clocks 
    */
    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.PLL2.PLL2State = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
    {
        while(1);
    }

    /**Initializes the CPU, AHB and APB busses clocks 
    */
    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSE;
    RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
    {
        while(1);
    }

    /**Configure the Systick interrupt time 
    */
    HAL_SYSTICK_Config(HAL_RCC_GetHCLKFreq()/1000);

    /**Configure the Systick 
    */
    HAL_SYSTICK_CLKSourceConfig(SYSTICK_CLKSOURCE_HCLK);

    /**Configure the Systick interrupt time 
    */
    __HAL_RCC_PLLI2S_ENABLE();

    /* SysTick_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(SysTick_IRQn, 15, 0);

    GPIO_InitTypeDef GPIO_InitStruct;

    /* GPIO Ports Clock Enable */
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /*Configure GPIO pins : SW1_1_Pin SW1_2_Pin SW1_3_Pin SW1_4_Pin */
    GPIO_InitStruct.Pin = SW1_1_Pin|SW1_2_Pin|SW1_3_Pin|SW1_4_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);
}

void SYS_reset(void)
{
    #define NVIC_AIRCR_VECTKEY    (0x5FA << 16)   /*!< AIRCR Key for write access   */
    #define NVIC_SYSRESETREQ            2         /*!< System Reset Request         */

    //__DSB();                                                                                /* Ensure completion of memory access */              
    SCB->AIRCR  = (NVIC_AIRCR_VECTKEY | (SCB->AIRCR & (0x700)) | (1<<NVIC_SYSRESETREQ));    /* Keep priority group unchanged */
    //__DSB();                                                                                /* Ensure completion of memory access */              
  while(1);                                                                                 /* wait until reset */
}

uint8_t SYS_getModelID(void)
{
    uint8_t xModel = 0;
    
    if (HAL_GPIO_ReadPin(SW1_1_GPIO_Port, SW1_1_Pin) == GPIO_PIN_RESET)
    {
        xModel |= 0x08;
    }

    if (HAL_GPIO_ReadPin(SW1_2_GPIO_Port, SW1_2_Pin) == GPIO_PIN_RESET)
    {
        xModel |= 0x04;
    }

    if (HAL_GPIO_ReadPin(SW1_3_GPIO_Port, SW1_3_Pin) == GPIO_PIN_RESET)
    {
        xModel |= 0x02;
    }

    if (HAL_GPIO_ReadPin(SW1_4_GPIO_Port, SW1_4_Pin) == GPIO_PIN_RESET)
    {
        xModel |= 0x01;
    }

    return  xModel;
}

const char* pModelName[] =
{
    "",
    "106/329DD",
    "P-30",
    "",
    "P-624",
    "JH-700UDS",
    "P-506",
    "",
    "J-305"
    
};

const char*   SYS_getModelName(uint8_t xModel)
{
    if (xModel < sizeof(pModelName) / sizeof(char *))
    {
        return  pModelName[xModel];
    }
    
    return  "";
}

