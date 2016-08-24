/*
 * Copyright (C) 2015 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * This is an example to flash a number of LED lights in a pattern.
 * Connect your LEDs to appropriate GPIOs (as many as you like)
 * then modify gpio[] and m_gpio[] to specify the pins.
 */
#include <unistd.h>
#include <stdio.h>

#include <mraa.h>

int main(int argc, char* argv[]) {
  const unsigned gpio[] = {0, 1, 2};
  const int gpio_count = sizeof(gpio)/sizeof(*gpio);
  mraa_gpio_context m_gpio[] = {NULL, NULL, NULL};

  mraa_init();
  for (int i = 0; i < gpio_count; i++) {
    m_gpio[i] = mraa_gpio_init(gpio[i]);
    if (!m_gpio[i]) {
      fprintf(stderr, "Unable to initialize GPIO %d, invalid pin number?\n",
          gpio[i]);
      return 1;
    }
    mraa_gpio_dir(m_gpio[i], MRAA_GPIO_OUT);
  }

  for (int iterations = 0; iterations < 3; iterations++) {
    for (int i = 0; i < gpio_count; i++) {
    mraa_gpio_write(m_gpio[i], 1);
      sleep(1);
    }
    for (int i = gpio_count-1; i >= 0; i--) {
      mraa_gpio_write(m_gpio[i], 0);
      sleep(1);
    }
  }

  for (int i = 0; i < gpio_count; i++)
    mraa_gpio_close(m_gpio[i]);
  return 0;
}
