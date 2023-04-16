/**
 * Copyright (c) 2022 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include <inttypes.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"

#include "lwip/pbuf.h"
#include "lwip/udp.h"

#include "protocal.h"

char ssid[] = "232 wifi";
char pass[] = "barkcbask";

#define UDP_PORT 6401
#define BEACON_MSG_LEN_MAX 100
#define BEACON_TARGET "255.255.255.255"
#define BEACON_INTERVAL_MS 1000

void run_udp_beacon() {
  struct udp_pcb *pcb = udp_new();

  int s = 12;
  char buf[s];
  int name = 32;

  encodeBroadcast(name, buf, s);

  ip_addr_t addr;
  ipaddr_aton(BEACON_TARGET, &addr);

  int counter = 0;
  while (true) {
    struct pbuf *p =
        pbuf_alloc(PBUF_TRANSPORT, BEACON_MSG_LEN_MAX + 1, PBUF_RAM);
    // p->payload = buf;

    char *req = (char *)p->payload;

    memset(req, 0, BEACON_MSG_LEN_MAX + 1);
    memcpy(req, buf, s);

    // memset(req, 0, BEACON_MSG_LEN_MAX + 1);
    // snprintf(req, BEACON_MSG_LEN_MAX, "%d\n", counter);

    err_t er = udp_sendto(pcb, p, &addr, UDP_PORT);
    pbuf_free(p);
    if (er != ERR_OK) {
      printf("Failed to send UDP packet! error=%d", er);
      while (true) {
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
        sleep_ms(500);
        cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
        sleep_ms(500);
      }

    } else {
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
      sleep_ms(1000);
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
      sleep_ms(1000);
      counter++;
    }

    // Note in practice for this simple UDP transmitter,
    // the end result for both background and poll is the same

#if PICO_CYW43_ARCH_POLL
    // if you are using pico_cyw43_arch_poll, then you must poll periodically
    // from your main loop (not from a timer) to check for Wi-Fi driver or lwIP
    // work that needs to be done.
    cyw43_arch_poll();
    sleep_ms(BEACON_INTERVAL_MS);
#else
    // if you are not using pico_cyw43_arch_poll, then WiFI driver and lwIP work
    // is done via interrupt in the background. This sleep is just an example of
    // some (blocking) work you might be doing.
    sleep_ms(BEACON_INTERVAL_MS);
#endif
  }
}

int main() {
  stdio_init_all();
  if (cyw43_arch_init_with_country(CYW43_COUNTRY_USA)) {
    printf("Wi-Fi init failed");
    return 1;
  }

  cyw43_arch_enable_sta_mode();

  if (cyw43_arch_wifi_connect_timeout_ms(ssid, pass, CYW43_AUTH_WPA2_AES_PSK,
                                         100000)) {
    while (true) {
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
      sleep_ms(100);
      cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 0);
      sleep_ms(100);
    }
  } else {
    //
    //    cyw43_arch_gpio_put(CYW43_WL_GPIO_LED_PIN, 1);
    //    sleep_ms(250);
    run_udp_beacon();
  }
}
