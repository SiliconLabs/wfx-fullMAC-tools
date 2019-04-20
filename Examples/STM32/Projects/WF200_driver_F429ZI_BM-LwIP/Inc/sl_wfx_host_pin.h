#pragma once
#include "stm32f4xx_hal.h"
#include "sl_wfx_host_api.h"

/* SL_WFX SPI pin configuration for NUCLEO-F429ZI */

/* SPI IRQ pin: PD15 */
#define SL_WFX_IRQ_PORT_SPI        GPIOD
#define SL_WFX_IRQ_GPIO_SPI        GPIO_PIN_15
/* WIRQ pin: PF12 */
#define SL_WFX_WIRQ_PORT           GPIOF
#define SL_WFX_WIRQ_GPIO           GPIO_PIN_12
/* Reset pin: PF13 */
#define SL_WFX_RESET_PORT          GPIOF
#define SL_WFX_RESET_GPIO          GPIO_PIN_13
/* Wake-up pin: PE9 */
#define SL_WFX_WUP_PORT            GPIOE
#define SL_WFX_WUP_GPIO            GPIO_PIN_9
/* LED1 pin: PC0 */
#define SL_WFX_LED1_PORT           GPIOC
#define SL_WFX_LED1_GPIO           GPIO_PIN_0
/* LED2 pin: PA3 */
#define SL_WFX_LED2_PORT           GPIOA
#define SL_WFX_LED2_GPIO           GPIO_PIN_3


#ifndef USE_ALTERNATIVE_SPI
/* Main SPI CLK pin: PA5 */
#define SL_WFX_CLK_PORT_SPI        GPIOA
#define SL_WFX_CLK_GPIO_SPI        GPIO_PIN_5
/* Main SPI MISO pin: PA6 */
#define SL_WFX_MISO_PORT_SPI       GPIOA
#define SL_WFX_MISO_GPIO_SPI       GPIO_PIN_6
/* Main SPI MOSI pin: PA7 */
#define SL_WFX_MOSI_PORT_SPI       GPIOA
#define SL_WFX_MOSI_GPIO_SPI       GPIO_PIN_7
/* Main SPI CS pin: PD14 */
#define SL_WFX_CS_PORT_SPI         GPIOD
#define SL_WFX_CS_GPIO_SPI         GPIO_PIN_14
#else
/* Alternative SPI CLK pin: PB3 */
#define SL_WFX_CLK_PORT_SPI        GPIOB
#define SL_WFX_CLK_GPIO_SPI        GPIO_PIN_3
/* Alternative SPI MISO pin: PB4 */
#define SL_WFX_MISO_PORT_SPI       GPIOB
#define SL_WFX_MISO_GPIO_SPI       GPIO_PIN_4
/* Alternative SPI MOSI pin: PB5 */
#define SL_WFX_MOSI_PORT_SPI       GPIOB
#define SL_WFX_MOSI_GPIO_SPI       GPIO_PIN_5
/* Alternative SPI CS pin: PA4 */
#define SL_WFX_CS_PORT_SPI         GPIOA
#define SL_WFX_CS_GPIO_SPI         GPIO_PIN_4
#endif //USE_ALTERNATIVE_SPI
