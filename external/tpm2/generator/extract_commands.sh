#!/bin/sh
# Copyright 2014 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

cat $1 |
  # Mark descriptions and actions in the body.
  sed 's/^[0-9. ]*Command and Response$/_COMMAND_SECTION/' |
  sed 's/^[0-9. ]*Detailed Actions$/_ACTIONS_SECTION/' |
  # Keep only command and response sections.
  awk '/^_COMMAND_SECTION$/,/^_ACTIONS_SECTION$/ { print $0; }' |
  sed 's/^_COMMAND_SECTION$//' |
  sed 's/^_ACTIONS_SECTION$//' |
  # Remove headers and footers.
  sed 's/^.*Trusted Platform Module Library.*$//' |
  sed 's/^.*Part 3: Commands.*$//' |
  sed 's/^.*Family .2.0..*$//' |
  sed 's/^.*Level 00 Revision.*$//' |
  sed 's/^.*Published.*$//' |
  sed 's/^.*Copyright.*$//' |
  sed 's/^.*Page [0-9].*$//' |
  sed 's/^.*October 31, 2013.*$//' |
  # Remove table headers.
  sed 's/^Type$//' | sed 's/^Name$//' | sed 's/^Description$//' |
  # Remove leading spaces.
  sed 's/^[ ][ ]*//' |
  # Remove empty lines.
  sed '/^$/d' |
  # Mark begin and end.
  awk '
    BEGIN { print "_BEGIN"; }
    { print $0; }
    END { print "_END"; }
  ' |
  # Mark command / response tables.
  sed 's/^Table [0-9]* . \(.*\) Command$/_INPUT_START \1/' |
  sed 's/^Table [0-9]* . \(.*\) Response$/_OUTPUT_START \1/' |
  # Mark argument types.
  sed 's/^\(TPM[_A-Z0-9+]*\)$/_TYPE \1/' |
  sed 's/^\(U*INT[0-9]*\)$/_TYPE \1/' |
  # Filter out a few special cases that look like types but are not.
  sed 's/^_TYPE TPM_ST_NO_SESSIONS$/TPM_ST_NO_SESSIONS/' |
  sed 's/^_TYPE TPM_ALG_NULL$/TPM_ALG_NULL/' |
  sed 's/^_TYPE TPM_CC_HMAC$/TPM_CC_HMAC/' |
  sed 's/^_TYPE TPM_GENERATED_VALUE$/TPM_GENERATED_VALUE/' |
  sed 's/^_TYPE \(TPM_RH_[A-Z+]*\)$/\1/' |
  # Mark argument names.
  awk '
    BEGIN { last_line_was_type = 0; }
    /^_.*$/      { print $0; }
    /^_TYPE .*$/ { last_line_was_type = 1; }
    /^[^_].*$/   { if (last_line_was_type) {
                     last_line_was_type = 0;
                     print "_NAME " $0;
                     if ($0 !~ /^[@a-zA-Z0-9]*$/) {
                       print "_ERROR: Invalid name."; } }
                   else { print $0; } }
  ' |
  # Consolidate comments to a single line and mark.
  awk '
    BEGIN { comment = ""; }
    /^_.*$/    { if (comment != "") { print "_COMMENT " comment; comment = ""; }
                 print $0; }
    /^[^_].*$/ { if (comment != "") { comment = comment " " $0; }
                 else { comment = $0 } }
  ' |
  # Strip off @name modifier, keep TYPE+ modifier for for conditional unmarshaling.
  sed 's/^_NAME @\(.*\)$/_NAME \1/' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||CommandBlock*||End
  # Begin:_BEGIN\n
  # End:_END\n
  # CommandBlock:InputTag||Argument*||OutputTag||Argument*
  # InputTag:_INPUT_START <command>\n
  # OutputTag:_OUTPUT_START <command>\n
  # Argument:Type||Name[||Comment]
  # Type:_TYPE <type>\n
  # Name:_NAME <name>\n
  # Comment:_COMMENT <comment>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN\n(_INPUT_START[^\n]*\n(_TYPE[^\n]*\n_NAME[^\n]*\n(_COMMENT[^\n]*\n)*)*_OUTPUT_START[^\n]*\n(_TYPE[^\n]*\n_NAME[^\n]*\n(_COMMENT[^\n]*\n)*)*)*_END/ {
      print "_ERROR: Format check failed."; }
    { print $0; }
  '
exit 0
