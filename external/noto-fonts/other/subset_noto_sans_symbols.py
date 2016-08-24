#!/usr/bin/python
# coding=UTF-8
#
# Copyright 2014 Google Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

"""Create a curated subset of NotoSansSymbols for Android."""

__author__ = 'roozbeh@google.com (Roozbeh Pournader)'

import sys

from nototools import subset
from nototools import unicode_data

# Unicode blocks that we want to include in the font
BLOCKS_TO_INCLUDE = """
20D0..20FF; Combining Diacritical Marks for Symbols
2100..214F; Letterlike Symbols
2190..21FF; Arrows
2200..22FF; Mathematical Operators
2300..23FF; Miscellaneous Technical
2400..243F; Control Pictures
2440..245F; Optical Character Recognition
2460..24FF; Enclosed Alphanumerics
2500..257F; Box Drawing
2580..259F; Block Elements
25A0..25FF; Geometric Shapes
2600..26FF; Miscellaneous Symbols
2700..27BF; Dingbats
27C0..27EF; Miscellaneous Mathematical Symbols-A
27F0..27FF; Supplemental Arrows-A
2800..28FF; Braille Patterns
2A00..2AFF; Supplemental Mathematical Operators
"""

# One-off characters to be included, needed for backward compatibility and
# supporting various character sets, including ARIB sets and black and white
# emoji
ONE_OFF_ADDITIONS = {
    0x27D0, # ⟐ WHITE DIAMOND WITH CENTRED DOT
    0x2934, # ⤴ ARROW POINTING RIGHTWARDS THEN CURVING UPWARDS
    0x2935, # ⤵ ARROW POINTING RIGHTWARDS THEN CURVING DOWNWARDS
    0x2985, # ⦅ LEFT WHITE PARENTHESIS
    0x2986, # ⦆ RIGHT WHITE PARENTHESIS
    0x2B05, # ⬅ LEFTWARDS BLACK ARROW
    0x2B06, # ⬆ UPWARDS BLACK ARROW
    0x2B07, # ⬇ DOWNWARDS BLACK ARROW
    0x2B24, # ⬤ BLACK LARGE CIRCLE
    0x2B2E, # ⬮ BLACK VERTICAL ELLIPSE
    0x2B2F, # ⬯ WHITE VERTICAL ELLIPSE
    0x2B56, # ⭖ HEAVY OVAL WITH OVAL INSIDE
    0x2B57, # ⭗ HEAVY CIRCLE WITH CIRCLE INSIDE
    0x2B58, # ⭘ HEAVY CIRCLE
    0x2B59, # ⭙ HEAVY CIRCLED SALTIRE
}

# letter-based characters, provided by Roboto
LETTERLIKE_CHARS_IN_ROBOTO = {
    0x2100, # ℀ ACCOUNT OF
    0x2101, # ℁ ADDRESSED TO THE SUBJECT
    0x2103, # ℃ DEGREE CELSIUS
    0x2105, # ℅ CARE OF
    0x2106, # ℆ CADA UNA
    0x2109, # ℉ DEGREE FAHRENHEIT
    0x2113, # ℓ SCRIPT SMALL L
    0x2116, # № NUMERO SIGN
    0x2117, # ℗ SOUND RECORDING COPYRIGHT
    0x211E, # ℞ PRESCRIPTION TAKE
    0x211F, # ℟ RESPONSE
    0x2120, # ℠ SERVICE MARK
    0x2121, # ℡ TELEPHONE SIGN
    0x2122, # ™ TRADE MARK SIGN
    0x2123, # ℣ VERSICLE
    0x2125, # ℥ OUNCE SIGN
    0x2126, # Ω OHM SIGN
    0x212A, # K KELVIN SIGN
    0x212B, # Å ANGSTROM SIGN
    0x212E, # ℮ ESTIMATED SYMBOL
    0x2132, # Ⅎ TURNED CAPITAL F
    0x213B, # ℻ FACSIMILE SIGN
    0x214D, # ⅍ AKTIESELSKAB
    0x214F, # ⅏ SYMBOL FOR SAMARITAN SOURCE
}

# default emoji characters in the BMP, based on
# http://www.unicode.org/draft/Public/emoji/1.0/emoji-data.txt
# We exclude these, so we don't block color emoji.
BMP_DEFAULT_EMOJI = {
    0x231A, # ⌚ WATCH
    0x231B, # ⌛ HOURGLASS
    0x23E9, # ⏩ BLACK RIGHT-POINTING DOUBLE TRIANGLE
    0x23EA, # ⏪ BLACK LEFT-POINTING DOUBLE TRIANGLE
    0x23EB, # ⏫ BLACK UP-POINTING DOUBLE TRIANGLE
    0x23EC, # ⏬ BLACK DOWN-POINTING DOUBLE TRIANGLE
    0x23F0, # ⏰ ALARM CLOCK
    0x23F3, # ⏳ HOURGLASS WITH FLOWING SAND
    0x25FD, # ◽ WHITE MEDIUM SMALL SQUARE
    0x25FE, # ◾ BLACK MEDIUM SMALL SQUARE
    0x2614, # ☔ UMBRELLA WITH RAIN DROPS
    0x2615, # ☕ HOT BEVERAGE
    0x2648, # ♈ ARIES
    0x2649, # ♉ TAURUS
    0x264A, # ♊ GEMINI
    0x264B, # ♋ CANCER
    0x264C, # ♌ LEO
    0x264D, # ♍ VIRGO
    0x264E, # ♎ LIBRA
    0x264F, # ♏ SCORPIUS
    0x2650, # ♐ SAGITTARIUS
    0x2651, # ♑ CAPRICORN
    0x2652, # ♒ AQUARIUS
    0x2653, # ♓ PISCES
    0x267F, # ♿ WHEELCHAIR SYMBOL
    0x2693, # ⚓ ANCHOR
    0x26A1, # ⚡ HIGH VOLTAGE SIGN
    0x26AA, # ⚪ MEDIUM WHITE CIRCLE
    0x26AB, # ⚫ MEDIUM BLACK CIRCLE
    0x26BD, # ⚽ SOCCER BALL
    0x26BE, # ⚾ BASEBALL
    0x26C4, # ⛄ SNOWMAN WITHOUT SNOW
    0x26C5, # ⛅ SUN BEHIND CLOUD
    0x26CE, # ⛎ OPHIUCHUS
    0x26D4, # ⛔ NO ENTRY
    0x26EA, # ⛪ CHURCH
    0x26F2, # ⛲ FOUNTAIN
    0x26F3, # ⛳ FLAG IN HOLE
    0x26F5, # ⛵ SAILBOAT
    0x26FA, # ⛺ TENT
    0x26FD, # ⛽ FUEL PUMP
    0x2705, # ✅ WHITE HEAVY CHECK MARK
    0x270A, # ✊ RAISED FIST
    0x270B, # ✋ RAISED HAND
    0x2728, # ✨ SPARKLES
    0x274C, # ❌ CROSS MARK
    0x274E, # ❎ NEGATIVE SQUARED CROSS MARK
    0x2753, # ❓ BLACK QUESTION MARK ORNAMENT
    0x2754, # ❔ WHITE QUESTION MARK ORNAMENT
    0x2755, # ❕ WHITE EXCLAMATION MARK ORNAMENT
    0x2757, # ❗ HEAVY EXCLAMATION MARK SYMBOL
    0x2795, # ➕ HEAVY PLUS SIGN
    0x2796, # ➖ HEAVY MINUS SIGN
    0x2797, # ➗ HEAVY DIVISION SIGN
    0x27B0, # ➰ CURLY LOOP
    0x27BF, # ➿ DOUBLE CURLY LOOP
    0x2B1B, # ⬛ BLACK LARGE SQUARE
    0x2B1C, # ⬜ WHITE LARGE SQUARE
    0x2B50, # ⭐ WHITE MEDIUM STAR
    0x2B55, # ⭕ HEAVY LARGE CIRCLE
}

# Characters we have decided we are doing as emoji-style in Android,
# despite UTR#51's recommendation
ANDROID_EMOJI = {
    0x2600, # ☀ BLACK SUN WITH RAYS
    0x2601, # ☁ CLOUD
    0X260E, # ☎ BLACK TELEPHONE
    0x261D, # ☝ WHITE UP POINTING INDEX
    0x263A, # ☺ WHITE SMILING FACE
    0x2660, # ♠ BLACK SPADE SUIT
    0x2663, # ♣ BLACK CLUB SUIT
    0x2665, # ♥ BLACK HEART SUIT
    0x2666, # ♦ BLACK DIAMOND SUIT
    0x270C, # ✌ VICTORY HAND
    0x2744, # ❄ SNOWFLAKE
    0x2764, # ❤ HEAVY BLACK HEART
}

def main(argv):
    """Subset the Noto Symbols font.

    The first argument is the source file name, and the second argument is
    the target file name.
    """

    target_coverage = set()
    # Add all characters in BLOCKS_TO_INCLUDE
    for first, last, _ in unicode_data._parse_code_ranges(BLOCKS_TO_INCLUDE):
        target_coverage.update(range(first, last+1))

    # Add one-off characters
    target_coverage |= ONE_OFF_ADDITIONS
    # Remove characters preferably coming from Roboto
    target_coverage -= LETTERLIKE_CHARS_IN_ROBOTO
    # Remove characters that are supposed to default to emoji
    target_coverage -= BMP_DEFAULT_EMOJI | ANDROID_EMOJI

    # Remove dentistry symbols, as their main use appears to be for CJK:
    # http://www.unicode.org/L2/L2000/00098-n2195.pdf
    target_coverage -= set(range(0x23BE, 0x23CC+1))

    # Remove COMBINING ENCLOSING KEYCAP. It's needed for Android's color emoji
    # mechanism to work properly
    target_coverage.remove(0x20E3)

    source_file_name = argv[1]
    target_file_name = argv[2]
    subset.subset_font(
        source_file_name,
        target_file_name,
        include=target_coverage)


if __name__ == '__main__':
    main(sys.argv)
