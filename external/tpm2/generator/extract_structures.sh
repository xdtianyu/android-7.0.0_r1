#!/bin/sh
# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Pull out basic typdefs.
cat $1 |
  # Mark types tables and section boundaries.
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

# Pull out constant values.
cat $1 |
  # Mark constants tables and section boundaries.
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
    /^_CONSTANTS \(\w*\) .*$/ { print "_OLD_TYPE " $2;
                                print "_NEW_TYPE " $NF; }
  ' |
  # Mark names and error return type.
  sed 's/^\(TPM_[_A-Z0-9a]*\)$/_NAME \1/' |
  sed 's/^\(TPM_CC_[_A-Z0-9a-z]*\)$/_NAME \1/' |
  sed 's/^\(RC_[_A-Z0-9]*\)$/_NAME \1/' |
  sed 's/^\(PT_[_A-Z0-9]*\)$/_NAME \1/' |
  sed 's/^\(HR_[_A-Z0-9]*\)$/_NAME \1/' |
  sed 's/^\([_A-Z0-9]*FIRST\)$/_NAME \1/' |
  sed 's/^\([_A-Z0-9]*LAST\)$/_NAME \1/' |
  sed 's/^\(PLATFORM_PERSISTENT\)$/_NAME \1/' |
  sed 's/^\(#TPM_RC[_A-Z0-9]*\)$/_RETURN \1/' |
  # Keep only names and return types
  awk '
    BEGIN { last_line_was_name = 0;
            return_defined = 1;
            FS = " |#"; }
    /^_BEGIN_CONSTANTS$/ { print $0; }
    /^_OLD_TYPE .*$/ { if (!return_defined) {  print "_RETURN TPM_RC_VALUE"; }
                       return_defined = 0;
                       print $0 }
    /^_NEW_TYPE .*$/ { print $0; }
    /^_NAME .*$/ { if (last_line_was_name) {
                     last_line_was_name = 0;
                     if ($0 !~ /[A-Z_0-9x +]*/) { print "_ERROR: Invalid value"; } }
                   else { last_line_was_name = 1; print $0; } }
    /^_RETURN .*$/ { print "_RETURN " $3;
                     return_defined = 1; }
    /^[^_].*$/   { if (last_line_was_name) {
                     last_line_was_name = 0;
                     if ($0 !~ /[A-Z_0-9x +]*/) { print "_ERROR: Invalid value"; } } }
    /^_END$/ { if (!return_defined) { print "_RETURN TPM_RC_VALUE"; }
               print $0; }
  ' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||TableBlock*||End
  # Begin:_BEGIN_CONSTANTS\n
  # End:_END\n
  # TableBlock:Typedef||Name*||Return
  # Name:_NAME <name>\n
  # Return:_RETURN <name>\n
  # Typedef:OldType||NewType
  # OldType:_OLD_TYPE <type>\n
  # NewType:_NEW_TYPE <type>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN_CONSTANTS\n(_OLD_TYPE[^\n]*\n_NEW_TYPE[^\n]*\n(_NAME[^\n]*\n)*_RETURN[^\n]*\n)*_END/ {
      print "_ERROR: Format check failed."; }
    { print $0; }
  '

# Pull out attribute structs.
cat $1 |
  # Mark reserved bits.
  sed 's/^\([0-9]\+:\?[0-9]* Reserved\)$/_RESERVED \1/' |
  awk '
    BEGIN { print "_BEGIN_ATTRIBUTE_STRUCTS";
            FS = "[ ()]+|:";
            in_attribute = 0; }
    /^Table [0-9]* . Definition of \([A-Z_0-9]*\) TPM[A-Z_]* Bits[^.]*$/ {
      print "_OLD_TYPE " $6;
      print "_NEW_TYPE " $7;
      in_attribute = 1; }
    /^_RESERVED .*$/ { if (in_attribute && NF == 4) {
                         print "_RESERVED " $3 "_" $2; }
                       else if (in_attribute) {
                         print "_RESERVED " $2; } }
    END { print "_END"; }
  ' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||TableBlock*||End
  # Begin:_BEGIN_ATTRIBUTE_STRUCTS\n
  # End:_END\n
  # TableBlock:Typedef||Reserved*
  # Reserved:_RESERVED <value>(_<value>)?\n
  # Typedef:OldType||NewType
  # OldType:_OLD_TYPE <type>\n
  # NewType:_NEW_TYPE <type>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN_ATTRIBUTE_STRUCTS\n(_OLD_TYPE[^\n]*\n_NEW_TYPE[^\n]*\n(_RESERVED[^\n]*\n)*)*_END/ {
      print "_ERROR: Format check failed."; }
    { print $0; }
  '

# Pull out interface types.
cat $1 |
  # Mark interface tables and section boundaries.
  sed 's/^[0-9][0-9]*\([.][0-9][0-9]*\)\+.*$/_SECTION_BOUNDARY/' |
  sed 's/^Table [0-9]* . Definition of \(.*\) Type[^.s]*$/_INTERFACES \1/' |
  # Keep only table sections.
  awk '/^_INTERFACES .*$/,/^_SECTION_BOUNDARY$/ { print $0; }' |
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
  # Mark begin and end.
  awk '
    BEGIN { print "_BEGIN_INTERFACES"; }
    { print $0; }
    END { print "_END"; }
  ' |
  # Mark type.
  awk '
    BEGIN { FS = "[ ()]+"; }
    { print $0; }
    /^_INTERFACES \(\w*\) .*$/ { print "_OLD_TYPE " $2;
                                print "_NEW_TYPE " $NF; }
    /^_INTERFACES {[A-Z0-9]*} \(\w*\) .*$/ { print "_OLD_TYPE " $3;
                                          print "_NEW_TYPE " $NF; }
  ' |
  # Mark names, bounds, conditional values, and return values.
  sed 's/^\(TPM_[_A-Z0-9a]*\)$/_NAME \1/' |
  sed 's/^\(TPM_CC_[_A-Z0-9a-z]*\)$/_NAME \1/' |
  sed 's/^\(RC_[_A-Z0-9]*\)$/_NAME \1/' |
  sed 's/^\(PT_[_A-Z0-9]*\)$/_NAME \1/' |
  sed 's/^\(HR_[_A-Z0-9]*\)$/_NAME \1/' |
  sed 's/^\([_A-Z0-9]*FIRST\)$/_NAME \1/' |
  sed 's/^\([_A-Z0-9]*LAST\)$/_NAME \1/' |
  sed 's/^\(PLATFORM_PERSISTENT\)$/_NAME \1/' |
  sed 's/^\(NO\)$/_NAME \1/' |
  sed 's/^\(YES\)$/_NAME \1/' |
  sed 's/^\({[_A-Z0-9]*:[_A-Z0-9]*}\)$/_BOUND \1/' |
  sed 's/^\(+[_A-Z0-9]*\)$/_CONDITIONAL \1/' |
  sed 's/^\(#TPM_RC[_A-Z0-9]*\)$/_RETURN \1/' |
  sed 's/^\(\$.*\)$/_SUBSTITUTE \1/' |
  awk '
    BEGIN { print "_BEGIN_INTERFACES";
            FS = " |:|{|}|#|+";
            ret = 1; }
    /^_OLD_TYPE .*$/ { if (!ret) { print "_RETURN TPM_RC_VALUE"; }
                       ret = 0; }
    /^_.*_TYPE .*$/ { print $0; }
    /^_NAME .*$/ { print $0; }
    /^_SUBSTITUTE .*$/ { $2 = substr($2, 2, length($2)-1);
                         print $0; }
    /^_BOUND .*$/ { print "_MIN " $3;
                    print "_MAX " $4; }
    /^_CONDITIONAL .*$/ { print "_CONDITIONAL " $3; }
    /^_RETURN .*$/ { print "_RETURN " $3;
                     ret = 1; }
    END { print "_END"; }
  ' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||Tableblock*||End
  # Begin:_BEGIN_INTERFACES\n
  # End:_END\n
  # Tableblock: Typedef||(Subval?|Name*)||Bound*||Conditional?||Return
  # Name:_NAME <name>\n
  # Subval:_SUBSTITUTE <name>\n
  # Bound:Min||Max
  # Min:_MIN <name>\n
  # Max:_MAX <name>\n
  # Conditional:_CONDITIONAL <name>\n
  # Return:_RETURN <name>\n
  # Typedef:OldType||NewType
  # OldType:_OLD_TYPE <type>\n
  # NewType:_NEW_TYPE <type>\n
  awk '
    BEGIN { RS = ""; }
    $0 !~ /_BEGIN_INTERFACES\n(_OLD_TYPE[^\n]*\n_NEW_TYPE[^\n]*\n(_NAME[^\n]*\n)*(_SUBSTITUTE[^\n]*\n)?(_MIN[^\n]*\n_MAX[^\n]*\n)*(_CONDITIONAL[^\n]*\n)?(_RETURN[^\n]*\n)?)*_END/ {
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
  # Mark field names and associated decorations
  awk '
    BEGIN { last_line = "";
            FS = ":| |{|}|+|#"; }
    /^_.*$/      { if ($1 != "_TYPE") { print $0; } }
    /^_TYPE \+[A-Z0-9_]*$/ { print $1 " " $3;
                             prefix = substr($3, 1, 4);
                             if (last_line != "" && prefix == "TPMI") {
                               print "_NAME " last_line " _PLUS";
                             } else if (last_line != "") {
                               print "_NAME " last_line;
                             } else { print "_ERROR: Type with no name"; }
                             last_line = ""; }
    /^_TYPE [A-Z0-9_]*\+$/ { print $1 " " $2;
                             prefix = substr($2, 1, 4);
                             if (last_line != "" && prefix == "TPMI") {
                               print "_NAME " last_line " _PLUS";
                             } else if (last_line != "") {
                               print "_NAME " last_line;
                             } else { print "_ERROR: Type with no name"; }
                             last_line = ""; }
    /^_TYPE [A-Z0-9_]*$/ { print $0;
                           if (last_line != "") { print "_NAME " last_line ; }
                           else { print "_ERROR: Type with no name"; }
                           if (extra_line != "") { print extra_line; }
                           last_line = "";
                           extra_line = ""; }
    /^[^_].*$/   { last_line = $0; }
    /^\[[^]]*\].*$/ { $1 = substr($1, 2, length($1)-1);
                      sub(/\]/, " ", $0);
                      last_line = $2 " _UNION " $1; }
    /^.*\[[^]]*\]$/ { $1 = substr($1, 1, length($1)-1);
                      sub(/\]/, " " , $0);
                      last_line = $1 " _ARRAY " $2; }
    /^.*\{[A-Z0-9_]*:\}$/ { last_line = $1;
                            extra_line = "_MIN " $1 " " $3; }
    /^.*\{:[A-Z0-9_]*\}$/ { last_line = $1;
                            extra_line = "_MAX " $1 " " $3; }
    /^.*\[[^]]*\] \{:[a-zA-Z0-9_()\/]*\}$/ { $2 = substr($2, 2, length($2)-2);
                                   last_line = $1 " _ARRAY " $2;
                                   extra_line = "_MAX " $2 " " $5 ; }
    /^tag \{[A-Z_]*(, [A-Z_]*)*\}$/ { last_line = "tag";
                                      extra_line = "_VALID " $3;
                                      for (i = 4; i <= NF-1; i++)
                                        extra_line = extra_line "\n_VALID " $i;
                                      sub(/\,/, "", extra_line); }
    /^size\=$/ { last_line = "size _CHECK" }
    /^#.*$/ { print "_RETURN " $2; }
  ' |
  # Strip off structure modifiers
  sed 's/^_STRUCTURE \((.*) \)*{.*} \(.*\)/_STRUCTURE \2/' |
  # Sanity check.  The format should now follow this grammar:
  # Format:Begin||Tableblock*||End
  # Begin:_BEGIN_STRUCTURES\n
  # End:_END\n
  # Tableblock: Structure||(Field|Min|Max)*||Return?
  # Structure:_STRUCTURE <name>\n
  # Min:_MIN <name> <value>\n
  # Max:_Max <name> <value>\n
  # Return:_RETURN <name>\n
  # Field: Type||Name||Valid*
  # Type:_TYPE <name>\n
  # Valid:_VALID <value>\n
  # Name:_NAME <name>((( _UNION | _ARRAY )<value>)| _PLUS)?\n
  # No sanity check here. Format is checked during generator.py
  awk '
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
    /^.* \[[^]]*\]$/ { $2 = substr($2, 2, length($2)-2);
                     last_line = $1 " _ARRAY " $2; }
  ' |
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

exit 0
