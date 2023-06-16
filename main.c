/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

// flux control gates connected to PWM 2 - 7
// this way we can use bit masking
// 11111100
// phase A 2 3
// phase B 4 5
// phase C 6 7
#define QPINMASK 0xFC
#define QPINFIRSTPIN 2

//  Bin values to set the control gate for each commutation
// forward set
//
// WRONG NEEDS UPDATE
#define CAbc 0b00010100
#define CABc 0b10010000
#define CaBc 0b10000100
#define CaBC 0b00100100
#define CabC 0b01100000
#define CAbC 0b01001000

// PWM desired frequency
#define PWMFREQUENCY 16000
// defualt Clock speed
#define SYSTEMCLOCK 125000000
// PWM wrap
#define PWMWRAP (SYSTEMCLOCK / PWMFREQUENCY - 1)

// ADC pin for detecting the current flow
#define CURRENTSENSOR 34

// PWM for controlling current flow
#define CURRENTFLOW 9

// Hall effect sensors GPIO 10-12
// using schmitt triger to translate to digital
#define HALLEFFECTFIRSTPIN 16
#define HALLEFFECTMASK                                                         \
  ((1 << HALLEFFECTFIRSTPIN) | (1 << HALLEFFECTFIRSTPIN + 1) |                 \
   (1 << HALLEFFECTFIRSTPIN + 2))

// Source voltage Sense
#define VOLTAGESOURCESENSE = 32

// Host pwm request
#define PWMIN = 17

// enum for the current hall effect state, lower case indates a low value, and
// upper case is a high value
enum HallEState { hAbc, hABc, haBc, haBC, habC, hAbC, error };

typedef enum State {
  initialize,
  waiting,
  running

} State;

typedef enum Direction { forward, reverse, none } Direction;

typedef struct PWMpin {
  uint32_t pinNumber;
  uint32_t slice;
  uint32_t channel;
} PWMpin;

PWMpin PWMpinData(uint32_t pinNumber) {
  PWMpin p;
  p.pinNumber = pinNumber;
  p.slice = pwm_gpio_to_slice_num(pinNumber);
  p.channel = pwm_gpio_to_channel(pinNumber);
  return p;
}

typedef struct CommutationStep {
  enum HallEState state;
  PWMpin Fpins[2];
  PWMpin Rpins[2];

} CommutationStep;

uint32_t get_index_from_Halleffect(uint32_t GPIO, uint32_t hallEffectMask,
                                   uint32_t halleffectFirstPin) {
  uint32_t i = (GPIO & hallEffectMask) >> halleffectFirstPin;

  switch (i) {
  case 0:
    return 6;
  case 1:
    return 1;
  case 2:
    return 5;
  case 3:
    return 0;
  case 4:
    return 3;
  case 5:
    return 2;
  case 6:
    return 4;
  case 7:
    return 6;
  }
}

uint32_t hallEffectFlag = 0;

void hallEffect_callback(uint gpio, uint32_t events) { hallEffectFlag = 1; }

uint32_t dutyCycleToLevel(float duty) {
  return (uint32_t)((float)(PWMWRAP)*duty);
}

int main() {
  stdio_init_all();

  if (cyw43_arch_init()) {
    printf("Wi-Fi init failed");
    return -1;
  }

  PWMpin fluxPWMs[6];
  for (int i = 0; i < 6; i++) {
    gpio_set_function(QPINFIRSTPIN + i, GPIO_FUNC_PWM);
    fluxPWMs[i] = PWMpinData(QPINFIRSTPIN + i);
    pwm_set_wrap(fluxPWMs[i].slice, PWMWRAP);
    pwm_set_enabled(fluxPWMs[i].slice, true);
  }

  CommutationStep commutationList[6];
  commutationList[0].Fpins[0] = fluxPWMs[2];
  commutationList[0].Fpins[1] = fluxPWMs[5];
  commutationList[0].Rpins[0] = fluxPWMs[0];
  commutationList[0].Rpins[1] = fluxPWMs[5];

  commutationList[1].Fpins[0] = fluxPWMs[1];
  commutationList[1].Fpins[1] = fluxPWMs[2];
  commutationList[1].Rpins[0] = fluxPWMs[2];
  commutationList[1].Rpins[1] = fluxPWMs[5];

  commutationList[2].Fpins[0] = fluxPWMs[1];
  commutationList[2].Fpins[1] = fluxPWMs[4];
  commutationList[2].Rpins[0] = fluxPWMs[1];
  commutationList[2].Rpins[1] = fluxPWMs[2];

  commutationList[3].Fpins[0] = fluxPWMs[3];
  commutationList[3].Fpins[1] = fluxPWMs[4];
  commutationList[3].Rpins[0] = fluxPWMs[1];
  commutationList[3].Rpins[1] = fluxPWMs[4];

  commutationList[4].Fpins[0] = fluxPWMs[0];
  commutationList[4].Fpins[1] = fluxPWMs[3];
  commutationList[4].Rpins[0] = fluxPWMs[3];
  commutationList[4].Rpins[1] = fluxPWMs[4];

  commutationList[5].Fpins[0] = fluxPWMs[0];
  commutationList[5].Fpins[1] = fluxPWMs[5];
  commutationList[5].Rpins[0] = fluxPWMs[0];
  commutationList[5].Rpins[1] = fluxPWMs[3];

  // enable callbacks for the hall effect pins
  gpio_set_irq_enabled_with_callback(HALLEFFECTFIRSTPIN,
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, &hallEffect_callback);
  gpio_set_irq_enabled_with_callback(HALLEFFECTFIRSTPIN + 1,
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, &hallEffect_callback);
  gpio_set_irq_enabled_with_callback(HALLEFFECTFIRSTPIN + 2,
                                     GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
                                     true, &hallEffect_callback);

  // State state = initialize;
  Direction directrion = forward;
  uint32_t Tlevel = dutyCycleToLevel(.7);

  while (1) {

    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    sleep_ms(500);
    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
    sleep_ms(500);

    // check HallFlag
    if (hallEffectFlag == 1) {
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
      sleep_ms(100);
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
      sleep_ms(100);

      hallEffectFlag = 0;
      uint32_t aGPIO = gpio_get_all();
      uint32_t CIndex =
          get_index_from_Halleffect(aGPIO, HALLEFFECTMASK, HALLEFFECTFIRSTPIN);
      if (CIndex != 6) {
        CommutationStep CS = commutationList[CIndex];

        for (int i = 0; i < 6; i++) {
          pwm_set_chan_level(fluxPWMs[i].slice, fluxPWMs[i].channel,
                             0);
        }
        PWMpin p[2];
        switch (directrion) {
        case forward:
          pwm_set_chan_level(CS.Fpins[0].slice, CS.Fpins[0].channel, Tlevel);
          pwm_set_chan_level(CS.Fpins[1].slice, CS.Fpins[2].channel, Tlevel);
          break;
        case reverse:
          pwm_set_chan_level(CS.Rpins[0].slice, CS.Rpins[0].channel, Tlevel);
          pwm_set_chan_level(CS.Rpins[1].slice, CS.Rpins[2].channel, Tlevel);

          break;
        case none:
          break;
        }
      }
    }
  }
}
