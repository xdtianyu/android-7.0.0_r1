#!/bin/sh
#
# Copyright (C) 2014 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

# Pull out types.
cat $1 |
  # Mark tables and section boundaries.
  sed 's/^[0-9][0-9]*\([.][0-9][0-9]*\)\+.*$/_SECTION_BOUNDARY/' |
  sed 's/^Table [0-9]* . Definition of .*Types[^.]*$/_TYPES/' |
  # Keep only table sections.
  awk '/^_TYPES$/,/^_SECTION_BOUNDARY$/ { print $0; }' |
  sed 's/^_TYPES$//' |
  sed 's/^_SECTION_BOUNDARY$//' |
  # Remove headers and footers.
  sed 's/^.*Trusted Platform Module Library.*$//' |
  sed 's/^.*Part 2: Structures.*$//' |
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
  # Mark begin and end and filter types.
  awk '
    BEGIN { print "_BEGIN_TYPES"; state = 0; }
    /^[^ ]*$/ { if (!state) { print "_OLD_TYPE " $0; state = 1; }
                else { print "_NEW_TYPE " $0; state = 0; } }
    END { print "_END"; }
  ' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||Typedef*||End
  # Begin:_BEGIN_TYPES\n
  # End:_END\n
  # Typedef:OldType||NewType
  # OldType:_OLD_TYPE <type>\n
  # NewType:_NEW_TYPE <type>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN_TYPES\n(_OLD_TYPE[^\n]*\n_NEW_TYPE[^\n]*\n)*_END/ {
      print "_ERROR: Format check failed."; }
    { print $0; }
  '

# Pull out interface types.
cat $1 |
  awk '
    BEGIN { print "_BEGIN_TYPES"; FS = "[ ()]+"; }
    /^Table [0-9]* . Definition of \([A-Z_]*\) TPMI_.* Type[^.]*$/ {
      print "_OLD_TYPE " $6;
      print "_NEW_TYPE " $7; }
    /^Table [0-9]* . Definition of \{.*\} \([A-Z_]*\) TPMI_.* Type[^.]*$/ {
      print "_OLD_TYPE " $7;
      print "_NEW_TYPE " $8; }
    /^Table [0-9]* . Definition of \([A-Z_]*\) \{.*\} TPMI_.* Type[^.]*$/ {
      print "_OLD_TYPE " $6;
      print "_NEW_TYPE " $8; }
    END { print "_END"; }
  ' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||Typedef*||End
  # Begin:_BEGIN_TYPES\n
  # End:_END\n
  # Typedef:OldType||NewType
  # OldType:_OLD_TYPE <type>\n
  # NewType:_NEW_TYPE <type>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN_TYPES\n(_OLD_TYPE[^\n]*\n_NEW_TYPE[^\n]*\n)*_END/ {
      print "_ERROR: Format check failed."; }
    { print $0; }
  '

# Pull out attribute types.
cat $1 |
  awk '
    BEGIN { print "_BEGIN_TYPES"; FS = "[ ()]+"; }
    /^Table [0-9]* . Definition of \([A-Z_0-9]*\) TPM[A-Z_]* Bits[^.]*$/ {
      print "_OLD_TYPE " $6;
      print "_NEW_TYPE " $7; }
    END { print "_END"; }
  ' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||Typedef*||End
  # Begin:_BEGIN_TYPES\n
  # End:_END\n
  # Typedef:OldType||NewType
  # OldType:_OLD_TYPE <type>\n
  # NewType:_NEW_TYPE <type>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN_TYPES\n(_OLD_TYPE[^\n]*\n_NEW_TYPE[^\n]*\n)*_END/ {
      print "_ERROR: Format check failed."; }
    { print $0; }
  '

# Pull out constant values.
cat $1 |
  # Mark tables and section boundaries.
  sed 's/^[0-9][0-9]*\([.][0-9][0-9]*\)\+.*$/_SECTION_BOUNDARY/' |
  sed 's/^Table [0-9]* . Definition of \(.*\) Constants[^.]*$/_CONSTANTS \1/' |
  # Keep only table sections.
  awk '/^_CONSTANTS .*$/,/^_SECTION_BOUNDARY$/ { print $0; }' |
  sed 's/^_SECTION_BOUNDARY$//' |
  # Remove headers and footers.
  sed 's/^.*Trusted Platform Module Library.*$//' |
  sed 's/^.*Part 2: Structures.*$//' |
  sed 's/^.*Family .2.0..*$//' |
  sed 's/^.*Level 00 Revision.*$//' |
  sed 's/^.*Published.*$//' |
  sed 's/^.*Copyright.*$//' |
  sed 's/^.*Page [0-9].*$//' |
  sed 's/^.*October 31, 2013.*$//' |
  # Remove table headers.
  sed 's/^Name$//' | sed 's/^Value$//' | sed 's/^Comments$//' |
  # Remove leading spaces.
  sed 's/^[ ][ ]*//' |
  # Remove empty lines.
  sed '/^$/d' |
  # Mark begin and end.
  awk '
    BEGIN { print "_BEGIN_CONSTANTS"; }
    { print $0; }
    END { print "_END"; }
  ' |
  # Mark type.
  awk '
    BEGIN { FS = "[ ()]+"; }
    { print $0; }
    /^_CONSTANTS \(\w*\) .*$/ { print "_TYPE " $2; }
  ' |
  # Mark names.
  sed 's/^\(TPM_[_A-Z0-9a]*\)$/_NAME \1/' |
  sed 's/^\(TPM_CC_[_A-Z0-9a-z]*\)$/_NAME \1/' |
  sed 's/^\(RC_[_A-Z0-9]*\)$/_NAME \1/' |
  sed 's/^\(PT_[_A-Z0-9]*\)$/_NAME \1/' |
  sed 's/^\(HR_[_A-Z0-9]*\)$/_NAME \1/' |
  sed 's/^\([_A-Z0-9]*FIRST\)$/_NAME \1/' |
  sed 's/^\([_A-Z0-9]*LAST\)$/_NAME \1/' |
  sed 's/^\(PLATFORM_PERSISTENT\)$/_NAME \1/' |
  # Mark values and throw away everything else.
  awk '
    BEGIN { last_line_was_name = 0; }
    /^_.*$/      { if ($0 !~ /^_NAME .*$/) { print $0; } }
    /^_NAME .*$/ { if (last_line_was_name) {
                     last_line_was_name = 0;
                     if ($0 !~ /[A-Z_0-9x +]*/) { print "_ERROR: Invalid value"; }
                     print "_VALUE " $2; }
                   else { last_line_was_name = 1; print $0; } }
    /^[^_].*$/   { if (last_line_was_name) {
                     last_line_was_name = 0;
                     if ($0 !~ /[A-Z_0-9x +]*/) { print "_ERROR: Invalid value"; }
                     print "_VALUE " $0; } }
  ' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||TableBlock*||End
  # Begin:_BEGIN_CONSTANTS\n
  # End:_END\n
  # TableBlock:TableTag||Type||Constant*
  # TableTag:_CONSTANTS <name>\n
  # Constant:Name||Value
  # Type:_TYPE <type>\n
  # Name:_NAME <name>\n
  # Value:_VALUE <value>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN_CONSTANTS\n(_CONSTANTS[^\n]*\n_TYPE[^\n]*\n(_NAME[^\n]*\n_VALUE[^\n]*\n)*)*_END/ {
      print "_ERROR: Format check failed."; }
    { print $0; }
  '

# Pull out structures.
cat $1 |
  # Mark tables and section boundaries.
  sed 's/^[0-9][0-9]*\([.][0-9][0-9]*\)\+.*$/_SECTION_BOUNDARY/' |
  sed 's/^Table [0-9]* . Definition of \(.*\) Structure[^.]*$/_STRUCTURE \1/' |
  # Keep only table sections.
  awk '/^_STRUCTURE .*$/,/^_SECTION_BOUNDARY$/ { print $0; }' |
  sed 's/^_SECTION_BOUNDARY$//' |
  # Remove headers and footers.
  sed 's/^.*Trusted Platform Module Library.*$//' |
  sed 's/^.*Part 2: Structures.*$//' |
  sed 's/^.*Family .2.0..*$//' |
  sed 's/^.*Level 00 Revision.*$//' |
  sed 's/^.*Published.*$//' |
  sed 's/^.*Copyright.*$//' |
  sed 's/^.*Page [0-9].*$//' |
  sed 's/^.*October 31, 2013.*$//' |
  # Remove table headers.
  sed 's/^Parameter$//' | sed 's/^Type$//' | sed 's/^Description$//' |
  # Remove leading spaces.
  sed 's/^[ ][ ]*//' |
  # Remove empty lines.
  sed '/^$/d' |
  # Mark begin and end.
  awk '
    BEGIN { print "_BEGIN_STRUCTURES"; }
    { print $0; }
    END { print "_END"; }
  ' |
  # Mark field types.
  sed 's/^\(+*TPM[_A-Z0-9+]*\)$/_TYPE \1/' |
  sed 's/^\(UINT[0-9]*\)$/_TYPE \1/' |
  sed 's/^\(BYTE*\)$/_TYPE \1/' |
  # Mark field names and throw away everything else.
  awk '
    BEGIN { last_line = ""; }
    /^_.*$/      { print $0; }
    /^_TYPE .*$/ { if (last_line != "") { print "_NAME " last_line; }
                   else { print "_ERROR: Type with no name"; }
                   last_line = ""; }
    /^[^_].*$/   { last_line = $0; }
  ' |
  # Change 'value [size] {:max}' to 'value[max]'.
  sed 's/^\(_NAME \w*\).*\[\w*\].*[{][:] *\([a-zA-Z0-9_()/]*\)[}]$/\1[\2]/' |
  # Strip off other modifiers.
  sed 's/^\(_NAME \w*\)[ {].*/\1/' |
  sed 's/^_TYPE +\(.*\)$/_TYPE \1/' |
  sed 's/^_TYPE \(.*\)+$/_TYPE \1/' |
  sed 's/^_NAME \/*\[.*\] *\(.*\)$/_NAME \1/' |
  sed 's/^\(_NAME \w*\)=/\1/' |
  sed 's/^_STRUCTURE \((.*) \)*{.*} \(.*\)/_STRUCTURE \2/' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||TableBlock*||End
  # Begin:_BEGIN_STRUCTURES\n
  # End:_END\n
  # TableBlock:TableTag||Field*
  # TableTag:_STRUCTURE <name>\n
  # Field:Type||Name
  # Type:_TYPE <type>\n
  # Name:_NAME <name>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN_STRUCTURES\n(_STRUCTURE[^\n]*\n(_TYPE[^\n]*\n_NAME[^\n]*\n)*)*_END/ {
      print "_ERROR: Format check failed."; }
    { print $0; }
  '

# Pull out unions.
cat $1 |
  # Mark tables and section boundaries.
  sed 's/^[0-9][0-9]*\([.][0-9][0-9]*\)\+.*$/_SECTION_BOUNDARY/' |
  sed 's/^Table [0-9]* . Definition of \(.*\) Union[^.]*$/_UNION \1/' |
  # Keep only table sections.
  awk '/^_UNION .*$/,/^_SECTION_BOUNDARY$/ { print $0; }' |
  sed 's/^_SECTION_BOUNDARY$//' |
  # Remove headers and footers.
  sed 's/^.*Trusted Platform Module Library.*$//' |
  sed 's/^.*Part 2: Structures.*$//' |
  sed 's/^.*Family .2.0..*$//' |
  sed 's/^.*Level 00 Revision.*$//' |
  sed 's/^.*Published.*$//' |
  sed 's/^.*Copyright.*$//' |
  sed 's/^.*Page [0-9].*$//' |
  sed 's/^.*October 31, 2013.*$//' |
  # Remove table headers.
  sed 's/^Parameter$//' | sed 's/^Type$//' | sed 's/^Selector$//' |
  sed 's/^Description$//' |
  # Remove leading spaces.
  sed 's/^[ ][ ]*//' |
  # Remove empty lines.
  sed '/^$/d' |
  # Mark begin and end.
  awk '
    BEGIN { print "_BEGIN_UNIONS"; }
    { print $0; }
    END { print "_END"; }
  ' |
  # Mark field types.
  sed 's/^\(+*TPM[_A-Z0-9+a]*\)$/_TYPE \1/' |
  sed 's/^\(UINT[0-9]*\)$/_TYPE \1/' |
  sed 's/^\(BYTE*\)$/_TYPE \1/' |
  # Mark field names and throw away everything else.
  awk '
    BEGIN { last_line = ""; }
    /^_.*$/      { if ($0 !~ /^_TYPE .*$/) { last_line = ""; print $0; } }
    /^_TYPE .*$/ { if (last_line !~ /^_TYPE .*$/) {
                     if (last_line != "" &&
                         last_line != "null" &&
                         last_line != "NOTE") {
                       print $0;
                       print "_NAME " last_line; }
                     else if (last_line == "") {
                       print "_ERROR: Type with no name"; }
                     last_line = $0; } }
    /^[^_].*$/   { last_line = $0; }
  ' |
  # Change 'value [size] {:max}' to 'value[max]'.
  sed 's/^\(_NAME \w*\).*\[\w*\].*[{][:] *\([a-zA-Z0-9_()/]*\)[}]$/\1[\2]/' |
  # Strip off other modifiers.
  sed 's/^\(_NAME \w*\) \(\[.*\]\)/\1\2/' |
  sed 's/^\(_NAME \w*\)\( {|{\).*/\1/' |
  sed 's/^_TYPE +\(.*\)$/_TYPE \1/' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||TableBlock*||End
  # Begin:_BEGIN_UNIONS\n
  # End:_END\n
  # TableBlock:TableTag||Field*
  # TableTag:_UNION <name>\n
  # Field:Type||Name
  # Type:_TYPE <type>\n
  # Name:_NAME <name>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN_UNIONS\n(_UNION[^\n]*\n(_TYPE[^\n]*\n_NAME[^\n]*\n)*)*_END/ {
      print "_ERROR: Format check failed."; }
    { print $0; }
  '

# Pull out default defines.
cat $1 |
  sed 's/^[A-Z]\([.][0-9][0-9]*\)\+.*$/_SECTION_BOUNDARY/' |
  sed 's/^Table [0-9]* . Defines for \([^.]*\)$/_DEFINES \1/' |
  sed 's/^_DEFINES Implemented Commands$//' |
  sed 's/^_DEFINES Implemented Algorithms$//' |
  awk '/^_DEFINES .*$/,/^_SECTION_BOUNDARY$/ { print $0; }' |
  sed 's/^_SECTION_BOUNDARY$//' |
  # Remove headers and footers.
  sed 's/^.*Trusted Platform Module Library.*$//' |
  sed 's/^.*Part 2: Structures.*$//' |
  sed 's/^.*Family .2.0..*$//' |
  sed 's/^.*Level 00 Revision.*$//' |
  sed 's/^.*Published.*$//' |
  sed 's/^.*Copyright.*$//' |
  sed 's/^.*Page [0-9].*$//' |
  sed 's/^.*October 31, 2013.*$//' |
  # Remove table headers.
  sed 's/^Name$//' | sed 's/^Value$//' | sed 's/^Description$//' |
  # Remove most comments.
  sed 's/^.*[g-wyz?]\+.*$//' |
  # Remove leading spaces.
  sed 's/^[ ][ ]*//' |
  # Remove empty lines.
  sed '/^$/d' |
  # Mark begin and end.
  awk '
    BEGIN { print "_BEGIN_DEFINES"; }
    { print $0; }
    END { print "_END"; }
  ' |
  # Mark define names.
  sed 's/^\([A-Z][_A-Z0-9]*\)$/_NAME \1/' |
  sed 's/^_NAME NOTE$//' |
  # Mark values and throw away everything else.
  awk '
    BEGIN { last_line_was_name = 0; }
    /^_.*$/      { if ($0 !~ /^_NAME .*$/) { print $0; } }
    /^_NAME .*$/ { if (last_line_was_name) {
                     last_line_was_name = 0;
                     if ($0 !~ /[A-Z_0-9x +()]*/) {
                       print "_ERROR: Invalid value"; }
                     print "_VALUE " $2; }
                   else { last_line_was_name = 1; print $0; } }
    /^[^_].*$/   { if (last_line_was_name) {
                     last_line_was_name = 0;
                     if ($0 !~ /[A-Z_0-9x +()]*/) {
                       print "_ERROR: Invalid value"; }
                     print "_VALUE " $0; } }
  ' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||Define*||End
  # Begin:_BEGIN_DEFINES\n
  # End:_END\n
  # Define:Name||Value
  # Name:_NAME <name>\n
  # Value:_VALUE <value>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN_DEFINES\n(_NAME[^\n]*\n_VALUE[^\n]*\n)*_END/ {
      print "_ERROR: Format check failed."; }
    { print $0; }
  '

exit 0
