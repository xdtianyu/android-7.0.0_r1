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
 * This is an example to display text on the SparkFun OLED
 * Display panel.
 */

#include <eboled.h>
#include <mraa.hpp>

#include <getopt.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string>

#define DEFAULT_DISPLAY_TEXT "hello world"

// Structure to hold the decoded command line options
struct pgm_options {
  uint8_t      invert_color;
  std::string  display_text;
};

// Be sure to keep the options for longopts and shortopts in the same order
// so that Usage() is correct.
static struct option longopts[] = {
  {"help",  no_argument,        NULL,   '?'},
  {"text",  required_argument,  NULL,   't'},
  {"invert",no_argument,        NULL,   'i'},
  {NULL,    0,                  NULL,    0 }
};
static char shortopts[] = "?t:i";

// Describes the options for this program.
void Usage(char *pgm_name) {
  printf("Usage: %s [options...]\n", pgm_name);
  printf("Prints a message on the SparkFun OLED Display\n");
  printf("Options:\n");
  for (int i = 0, j = 0; longopts[i].name; i++, j++) {
    if (shortopts[j] == ':')
      j++;
    printf(" --%-6s or -%c\n", longopts[i].name, shortopts[j]);
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
      case 'i':
        options->invert_color = true;
        break;
      case 't':
        options->display_text = optarg;
        break;
      default:
        Usage(argv[0]);
        return -1;
    }
  }
  argc -= optind;
  argv += optind;

  if (options->display_text.length() == 0)
    options->display_text = DEFAULT_DISPLAY_TEXT;

  return 0;
}

int main(int argc, char* argv[]) {
  pgm_options options = {false, ""};

  if (ReadOpts(argc, argv, &options) < 0)
    return 1;

  upm::EBOLED *display = new upm::EBOLED(
      mraa_get_default_i2c_bus(MRAA_MAIN_PLATFORM_OFFSET),
      EBOLED_DEFAULT_CD, EBOLED_DEFAULT_RESET);

  if (options.invert_color) {
    display->setTextColor(upm::COLOR_BLACK);
    display->fillScreen(upm::COLOR_WHITE);
  }

  display->write(options.display_text);
  display->refresh();
  sleep(5);
  delete display;

  return 0;
}
