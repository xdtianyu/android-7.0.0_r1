#!/bin/bash

# This script stops shill, associates with each test AP in turn,
# and harvests signal strength and quality numbers for us, then restarts
# shill so the old network can be reacquired and our test can complete

set +o posix;
shopt -s extglob;

oldstderr=;
stderrlog=;

output_to_vtx () {
  # copy stderr to the next free vt so the user can see progress
  # on the DUT, otherwise they're just sitting there with no feedback
  # (if we can, that is - if openvt isn't there, just get on with it)
  if which openvt;
  then
    stderrlog=/tmp/$RANDOM.vtx.log;
    exec {oldstderr}>&2;
    exec 2>$stderrlog;
    tail --pid $$ -f $stderrlog >&$oldstderr &
    openvt -s -w -- tail --pid $$ -f $stderrlog &
  fi
}

close_vtx () {
  if [[ -f "$stderrlog" ]]; then
    rm "$stderrlog";
  fi;
}

progress () { echo "$@" 1>&2; }

contains_modulations () {
  # check that at least one modulation in `wanted' is present in `supported'
  supported=$1;
  wanted=$2;

  case $supported in
    *[$wanted]*)
      return 0;
      ;;
  esac

  return 1;
}

# pick a WiFi interface to test
find_wifi_if () {
  iface="$1";

  if [[ -z "$iface" ]]; then
    while read _iface _ignore && test -z "$iface"; do
      iface=$_iface;
    done < <(iwconfig 2>/dev/null | grep '^[a-z]');
  fi;

  test -n "$iface";
}

wifi_status () {
  # harvest the state of the target interface: modulation, essid and so forth:
  find_wifi_if "$1";

  if_80211=;
  if_essid=;
  if_mode=;
  if_ap=;
  if_rate=;
  if_txp=;
  if_quality=;
  if_signal=;
  if_freq=;

  # iwconfig's output is a pain to parse, but is stable.
  # the newer tools are much easier to parse, but they are
  # considered unstable by the authors, who specifically forbid
  # scraping their output until the specification stabilises.
  while read data; do
    case "$data" in
      $iface*)
        if_essid=${data##*ESSID:*(\")};
        if_essid=${if_essid%\"*};
        if_80211=${data%%+( )ESSID:*};
        if_80211=${if_80211#*802.11};
        ;;
      Mode:*)
        if_mode=${data#Mode:}
        if_mode=${if_mode%% *};
        if_ap=${data##*Access Point: };
        if_ap=${if_ap%% *};
        if_freq=${data##*Frequency:};
        if_freq=${if_freq%%+( )Access Point:*};
        if [[ "$if_ap" = "Not-Associated" ]]; then
          if_txp=${data##*Tx-Power=};
          fi
          ;;
      Bit\ Rate*)
        if_rate=${data%%+( )Tx-*};
        if_rate=${if_rate#Bit Rate=};
        if [[ -z $"if_txp" ]]; then
          if_txp=${data##*Tx-Power=};
          fi;
          ;;
      Link*)
        if_quality=${data%%+( )Signal*};
        if_quality=${if_quality#Link Quality=};
        if_signal=${data##*Signal level=};
        ;;
    esac;
  done < <(iwconfig $iface)
}

wifi_scan () {
  iface=$1;

  # Trigger a wifi scan. The DUT doesn't necessarily scan all frequencies
  # or remember APs on frequencies it isn't currently on, so we need to do
  # this at times to make sure we can interact with the test AP:
  progress Bringing up $iface;
  ifconfig $iface up 1>&2;
  progress Scanning for CrOS test ESSIDs;

  lofreq_aps="";
  hifreq_aps="";

  cell_freq=;
  cell_essid=;

  while read scan; do
    if [[ -z "$cell_freq" || -z "$cell_essid" ]]; then
      case "$scan" in
        Frequency:*)
          cell_freq=${scan##*Frequency:}
          cell_freq=${cell_freq%% *};
          cell_freq=${cell_freq:-0};
          ;;
        ESSID:*)
          cell_essid=${scan#*ESSID:\"};
          cell_essid=${cell_essid%\"*};
      esac;
    else
      if [[ "${cell_essid#CrOS-test-}" != "$cell_essid" ]]; then
        progress "Found test ESSID $cell_essid (Frequency: $cell_freq)";
        case "$cell_freq" in
          2*)
            lofreq_aps=${lofreq_aps}${lofreq_aps:+ }${cell_essid};
            ;;
          [45]*)
            hifreq_aps=${hifreq_aps}${hifreq_aps:+ }${cell_essid};
            ;;
        esac;
      else
        progress "Ignoring ESSID $cell_essid (Frequency: $cell_freq)";
      fi;
      cell_essid="";
      cell_freq="";
    fi;
  done < <(iwlist $iface scan);
}

wifi_find_essid () {
  iface=$1;
  target=$2;

  progress Bringing up $iface;
  ifconfig $iface up 1>&2;
  progress Scanning for ESSID $target;
  iwlist $iface scan | grep -q "ESSID:\"$target\"";
}

wifi_strength () {
  iface=$1;
  result=;
  macaddr=$(cat /sys/class/net/$iface/address)
  gateway=$(ip route show dev $iface to match 0/0|\
            if read x x g x; then echo $g; fi);

  progress Allowing link $gateway strength/quality readings to stabilise;
  ping -n -w 5 -c 5 $gateway 1>&2;

  progress Contacting AP at "/dev/tcp/$gateway/80" to collect TX strength;
  if exec {http}<>/dev/tcp/$gateway/80; then
    echo -e "GET /cgi-bin/txinfo HTTP/1.0\r\n\r" >&$http;

    while read mac strength other;
    do
      if [[ x${mac,,*} = x${macaddr,,*} ]]; then result=$strength; fi;
    done <&$http;
  fi;

  tx_db=${result:--100}" dBm";
}

wifi_associate () {
  wifi_status $iface;

  essid=${2:-"NO-ESSID-SUPPLIED"};

  if wifi_find_essid $iface $essid; then
    SECONDS=0;
    iwconfig $iface essid "$essid" 1>&2;

    until [[ x$if_essid = x$essid && x$if_ap != x'Not-Associated' ]]; do
      wifi_status $iface;
      progress "$SECONDS: $if_essid/$if_ap (want $essid)";
      sleep 2;
      if [[ $SECONDS -ge 30 ]]; then if_ap=failed; fi;
    done;
  else
    if_ap="Not-Found";
  fi

  test "$if_essid" = "$essid";
}

wifi_dhcp () {
  iface=$1;
  dhclient $iface \
    -sf /usr/local/sbin/dhclient-script \
    -lf /tmp/dhclient.leases 1>&2;
}

emit_result () {
  test=$2;

  if [[ "$1" = success ]]; then

    cat - <<EOF;
802.11$test freq $if_freq quality $if_quality rx $if_signal tx $tx_db
EOF

  else

    cat - <<EOF;
802.11$test freq 0 quality 0/70 rx -100 dBm tx -100 dBm
EOF

  fi;
}

test_association () {
  wlan_if=$1;
  ap_ssid=$2;
  mods=$3;

  if wifi_associate $wlan_if $ap_ssid; then
    wifi_dhcp     $wlan_if;
    wifi_strength $wlan_if;
    emit_result success $mods;
  else
    progress "WiFi Association failed for $wlan_if [$ap_ssid vs $if_ap]";
    emit_result failure $mods;
  fi;
}

output_to_vtx;

wifi_status $1; # this will figure out all our initial if_… values
modulations=$if_80211;

progress "Start: $iface ($if_mode/$if_80211) ap $if_ap essid '$if_essid'";
progress "Shutting down shill";
stop shill 1>&2

progress "Looking for test APs";
wifi_scan $iface;

progress "2.x GHz APs: $lofreq_aps";
progress "4+  GHz APs: $hifreq_aps";

if contains_modulations $modulations bg; then
  for ap in $lofreq_aps; do test_association $iface $ap bg; done;
fi

if contains_modulations $modulations an; then
  for ap in $hifreq_aps; do test_association $iface $ap an; done;
fi

start shill 1>&2;

close_vtx;
