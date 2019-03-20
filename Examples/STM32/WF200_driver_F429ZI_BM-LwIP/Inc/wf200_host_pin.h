#pragma once
#include "stm32f4xx_hal.h"
#include "wf200_host_api.h"

/* WF200 SPI pin configuration for NUCLEO-F429ZI */

/* SPI IRQ pin: PD15 */
#define WF200_IRQ_PORT_SPI        GPIOD
#define WF200_IRQ_GPIO_SPI        GPIO_PIN_15
/* WIRQ pin: PF12 */
#define WF200_WIRQ_PORT           GPIOF
#define WF200_WIRQ_GPIO           GPIO_PIN_12
/* Reset pin: PF13 */
#define WF200_RESET_PORT          GPIOF
#define WF200_RESET_GPIO          GPIO_PIN_13
/* Wake-up pin: PE9 */
#define WF200_WUP_PORT            GPIOE
#define WF200_WUP_GPIO            GPIO_PIN_9
/* LED1 pin: PA3 */
#define WF200_LED1_PORT           GPIOA
#define WF200_LED1_GPIO           GPIO_PIN_3
/* LED2 pin: PC0 */
#define WF200_LED2_PORT           GPIOC
#define WF200_LED2_GPIO           GPIO_PIN_0

#ifndef USE_ALTERNATIVE_SPI
/* Main SPI CLK pin: PA5 */
#define WF200_CLK_PORT_SPI        GPIOA
#define WF200_CLK_GPIO_SPI        GPIO_PIN_5
/* Main SPI MISO pin: PA6 */
#define WF200_MISO_PORT_SPI       GPIOA
#define WF200_MISO_GPIO_SPI       GPIO_PIN_6
/* Main SPI MOSI pin: PA7 */
#define WF200_MOSI_PORT_SPI       GPIOA
#define WF200_MOSI_GPIO_SPI       GPIO_PIN_7
/* Main SPI CS pin: PD14 */
#define WF200_CS_PORT_SPI         GPIOD
#define WF200_CS_GPIO_SPI         GPIO_PIN_14
#else
/* Alternative SPI CLK pin: PB3 */
#define WF200_CLK_PORT_SPI        GPIOB
#define WF200_CLK_GPIO_SPI        GPIO_PIN_3
/* Alternative SPI MISO pin: PB4 */
#define WF200_MISO_PORT_SPI       GPIOB
#define WF200_MISO_GPIO_SPI       GPIO_PIN_4
/* Alternative SPI MOSI pin: PB5 */
#define WF200_MOSI_PORT_SPI       GPIOB
#define WF200_MOSI_GPIO_SPI       GPIO_PIN_5
/* Alternative SPI CS pin: PA4 */
#define WF200_CS_PORT_SPI         GPIOA
#define WF200_CS_GPIO_SPI         GPIO_PIN_4
#endif //USE_ALTERNATIVE_SPI
