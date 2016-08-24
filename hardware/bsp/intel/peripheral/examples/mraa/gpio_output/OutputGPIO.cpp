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
 * This is an example to set/clear a GPIO pin using the MRAA library.
 * This may be tested with any of many GPIO devices such as an LED
 * directly connected to the GPIO pin and ground,
 * the Grove buzzer or Grove LED board.
 *
 * The on-board LED on the Edison Arudino expansion board may be
 * accessed with Digital I/O 13 (mapped from Linux GPIO 243):
 *   example-gpio-output -p 13 -s
 *
 * See the following link for a table to map from the numbers on the
 * board silk screen to the libmraa GPIO numbers:
 *   https://learn.sparkfun.com/tutorials/installing-libmraa-on-ubilinux-for-edison
 */
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>

#include <mraa.h>

// Structure to hold the decoded command line options
struct pgm_options {
  int        pin;
  bool       set;
  bool       clear;
};

// Be sure to keep the options for longopts and shortopts in the same order
// so that Usage() is correct.
static struct option longopts[] = {
  {"help",  no_argument,        NULL,   '?'},
  {"pin",   no_argument,        NULL,   'p'},
  {"set",   no_argument,        NULL,   's'},
  {"clear", no_argument,        NULL,   'c'},
  {NULL,    0,                  NULL,    0 }
};
static char shortopts[] = "?p:sc";

// Describes the options for this program.
void Usage(char *pgm_name) {
  printf("Usage: %s [options...]\n", pgm_name);
  printf("Manipulate a GPIO pin\n");
  printf("Options:\n");
  for (int i = 0, j = 0; longopts[i].name; i++, j++) {
    if (shortopts[j] == ':')
      j++;
    printf(" --%-6s%s or -%c%s""\n",
        longopts[i].name,
        (shortopts[j+1] == ':') ? " <arg> " : "",
        shortopts[j],
        (shortopts[j+1] == ':') ? " <arg> " : "");
  }
  return;
}

// Processes all command line options.
//   sets the options members for commnd line options
//   returns (0) on success, -1 otherwise.
int ReadOpts(int argc, char **argv, struct pgm_options *options) {
  int  ch = 0;

  while ((ch = getopt_long(argc, argv, shortopts, longopts, NULL)) != -1) {
    switch (ch) {
      case 'p':
        options->pin = atoi(optarg);
        break;
      case 's':
        options->set = true;
        break;
      case 'c':
        options->clear = true;
        break;
      default:
        Usage(argv[0]);
        return -1;
    }
  }
  argc -= optind;
  argv += optind;

  return 0;
}

int main(int argc, char* argv[]) {
  const unsigned kDefaultPinGPIO = 7;
  pgm_options options = {kDefaultPinGPIO, false, false};

  if (ReadOpts(argc, argv, &options) < 0)
    return 1;

  mraa_init();
  mraa_gpio_context m_gpio = mraa_gpio_init(options.pin);
  if (!m_gpio) {
    fprintf(stderr, "Unable to initialize GPIO, invalid pin number?\n");
    return 1;
  }
  mraa_gpio_dir(m_gpio, MRAA_GPIO_OUT);

  if (options.set)
    mraa_gpio_write(m_gpio, 1);

  // If both options were specified, wait a few seconds before clearing
  if (options.clear && options.set)
    sleep(5);

  if (options.clear)
    mraa_gpio_write(m_gpio, 0);

  mraa_gpio_close(m_gpio);
  return 0;
}
