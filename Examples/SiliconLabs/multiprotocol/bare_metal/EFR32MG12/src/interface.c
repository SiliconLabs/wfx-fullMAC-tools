
#include <stdio.h>
#include <stdbool.h>
#include "bsp.h"
#include "retargetserial.h"
#include "bluetooth_app.h"
#include "interface.h"

#if HAL_SPIDISPLAY_ENABLE
#include "mp-ui.h"

#ifndef FEATURE_IOEXPANDER
#include "sl_sleeptimer.h"

/* Periodically called Display Polarity Inverter Function for the LCD.
   Toggles the the EXTCOMIN signal of the Sharp memory LCD panel, which prevents building up a DC
   bias according to the LCD's datasheet */
typedef void (*dispPolarityInvertFunc_t)(void *);

static sl_sleeptimer_timer_handle_t display_timer;
#endif /* FEATURE_IOEXPANDER */

#define CLEAR_DIRECTION_DELAY_MSEC        500

static sl_sleeptimer_timer_handle_t clear_direction_timer;
static uint32_t clear_direction_delay_ticks = 0;
#endif

static void apply_light_change(interface_light_trigger_src_t trigger,
                               interface_mac_t *mac,
                               uint8_t new_state);

static uint8_t led_state = 0;
static interface_light_trigger_src_t led_trigger_source = interface_light_trigger_src_button;
static interface_mac_t mac_trigger = {0};
static interface_mac_t own_mac = {0};


void interface_init (void)
{
  // Initialize UART interface
  RETARGET_SerialInit();

  // Initialize LEDs
  BSP_LedsInit();

#if HAL_SPIDISPLAY_ENABLE
  // Initialize LCD screen
  mpUiInit();
  mpUiClearMainScreen(true, true);

  sl_sleeptimer_init();

  // Compute once for all the delay to clear the direction on the LCD
  clear_direction_delay_ticks = ((uint64_t)CLEAR_DIRECTION_DELAY_MSEC * sl_sleeptimer_get_timer_frequency()) / 1000;
#endif

  // Save an empty MAC the time to initialize the BLE
  interface_light_off(interface_light_trigger_src_button, &own_mac);
  interface_display_wifi_state(false);
  interface_display_ble_state(false);

// Buttons pins are reused with the boards BRD4180/BRD4181 for the WiFi capabilities
#if ((EMBER_AF_BOARD_TYPE != BRD4180A) \
  && (EMBER_AF_BOARD_TYPE != BRD4180B) \
  && (EMBER_AF_BOARD_TYPE != BRD4181A) \
  && (EMBER_AF_BOARD_TYPE != BRD4181B))
  // Configure buttons as input and enable interrupt.
  GPIO_PinModeSet(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN, gpioModeInputPull, 1);
  GPIO_IntConfig(BSP_BUTTON0_PORT, BSP_BUTTON0_PIN, false, true, true);
  GPIO_PinModeSet(BSP_BUTTON1_PORT, BSP_BUTTON1_PIN, gpioModeInputPull, 1);
  GPIO_IntConfig(BSP_BUTTON1_PORT, BSP_BUTTON1_PIN, false, true, true);
#endif
}

void interface_light_off (interface_light_trigger_src_t trigger,
                          interface_mac_t *mac)
{
  // Turn Off the LEDs
  BSP_LedClear(0);
  BSP_LedClear(1);

  apply_light_change(trigger, mac, 0);
}

void interface_light_on (interface_light_trigger_src_t trigger,
                         interface_mac_t *mac)
{
  // Turn Off the LEDs
  BSP_LedSet(0);
  BSP_LedSet(1);

  apply_light_change(trigger, mac, 1);
}

void interface_light_toggle (interface_light_trigger_src_t trigger,
                             interface_mac_t *mac)
{
  // Toggle the LEDs
  BSP_LedToggle(0);
  BSP_LedToggle(1);

  apply_light_change(trigger, mac, !led_state);
}

void interface_light_set_state (interface_light_trigger_src_t trigger,
                                interface_mac_t *mac,
                                uint8_t new_led_state)
{
  if (new_led_state != 0) {
    interface_light_on(trigger, mac);
  } else {
    interface_light_off(trigger, mac);
  }
}

#if HAL_SPIDISPLAY_ENABLE
static void clear_direction_timer_callback (sl_sleeptimer_timer_handle_t *handle,
                                            void *data)
{
  (void)handle;
  (void)data;

  mpUiClearDirection(MP_UI_DIRECTION_PROT1);
  mpUiClearDirection(MP_UI_DIRECTION_PROT2);
}
#endif

static void apply_light_change (interface_light_trigger_src_t trigger,
                                interface_mac_t *mac,
                                uint8_t new_state)
{

#if HAL_SPIDISPLAY_ENABLE
  sl_status_t status;

  // Update the LCD
  mpUiDisplayLight(new_state);

  status = sl_sleeptimer_stop_timer(&clear_direction_timer);
  if (status == SL_STATUS_OK) {
    // The timer was running, a direction needs to be cleared !
    if (led_trigger_source == interface_light_trigger_src_wifi) {
      mpUiClearDirection(MP_UI_DIRECTION_PROT1);
    } else if (led_trigger_source == interface_light_trigger_src_bluetooth) {
      mpUiClearDirection(MP_UI_DIRECTION_PROT2);
    }
  }

  if (trigger == interface_light_trigger_src_wifi) {
    mpUiDisplayDirection(MP_UI_DIRECTION_PROT1);

    // Start a timer which will clear the direction after a while
    sl_sleeptimer_start_timer(&clear_direction_timer,
                              clear_direction_delay_ticks,
                              clear_direction_timer_callback,
                              NULL,
                              0,
                              0);

  } else if (trigger == interface_light_trigger_src_bluetooth) {
    mpUiDisplayDirection(MP_UI_DIRECTION_PROT2);

    // Start a timer which will clear the direction after a while
    sl_sleeptimer_start_timer(&clear_direction_timer,
                              clear_direction_delay_ticks,
                              clear_direction_timer_callback,
                              NULL,
                              0,
                              0);
  }
#endif

  // Send a BLE indication to update the application display
  bluetooth_app_request_send_indication();

  // Store data
  led_state = new_state;
  led_trigger_source = trigger;
  if (mac != NULL) {
    memcpy(&mac_trigger, mac, sizeof(mac_trigger));
  } else {
    // Set our own MAC in this case
    memcpy(&mac_trigger, &own_mac, sizeof(mac_trigger));
  }
}

uint8_t interface_light_get_state (void)
{
  return led_state;
}

interface_light_trigger_src_t interface_light_get_trigger (void)
{
  return led_trigger_source;
}

void interface_light_get_mac_trigger (interface_mac_t *mac)
{
  if (mac != NULL) {
    memcpy(mac, &mac_trigger, sizeof(interface_mac_t));
  }
}

#if HAL_SPIDISPLAY_ENABLE
#ifndef FEATURE_IOEXPANDER
static void display_timer_callback (sl_sleeptimer_timer_handle_t *handle,
                                    void *data)
{
  /*Toggle the the EXTCOMIN signal, which prevents building up a DC bias  within the
   * Sharp memory LCD panel */
  dispPolarityInvertFunc_t func = (dispPolarityInvertFunc_t)data;
  func(0);
}
#endif

/**************************************************************************//**
 * @brief   Register a callback function at the given frequency.
 *
 * @param[in] pFunction  Pointer to function that should be called at the
 *                       given frequency.
 * @param[in] argument   Argument to be given to the function.
 * @param[in] frequency  Frequency at which to call function at.
 *
 * @return  0 for successful or
 *         -1 if the requested frequency does not match the RTC frequency.
 *****************************************************************************/
int rtcIntCallbackRegister(void (*pFunction)(void*),
                           void* argument,
                           unsigned int frequency)
{
#ifndef FEATURE_IOEXPANDER
  uint32_t period_ticks = sl_sleeptimer_get_timer_frequency() / frequency;

  // Start timer with required frequency.
  sl_sleeptimer_start_periodic_timer(&display_timer,
                                     period_ticks,
                                     display_timer_callback,
                                     pFunction,
                                     0,
                                     0);
#endif /* FEATURE_IOEXPANDER */
  return 0;
}
#endif

void interface_display_ble_state (bool connected)
{
#if HAL_SPIDISPLAY_ENABLE
  mpUiDisplayProtocol(MP_UI_PROTOCOL2, connected);
#endif
}

void interface_display_wifi_state (bool connected)
{
#if HAL_SPIDISPLAY_ENABLE
  mpUiDisplayProtocol(MP_UI_PROTOCOL1, connected);
#endif
}

void interface_display_ble_id (uint8_t *id)
{
#if HAL_SPIDISPLAY_ENABLE
  char dev_id[9];
  // Only 5 characters are displayed correctly when the light is on
  sprintf(dev_id, "    %02X%02X", id[1], id[0]);

  mpUiDisplayId(MP_UI_PROTOCOL2, (uint8_t *)dev_id);
#endif

  // Save our own MAC as reference
  memcpy(&own_mac, id, sizeof(own_mac));
  memcpy(&mac_trigger, id, sizeof(mac_trigger));
}

void interface_display_wifi_id (uint8_t *id)
{
#if HAL_SPIDISPLAY_ENABLE
  mpUiDisplayId(MP_UI_PROTOCOL1, id);
#endif
}

