#!/usr/bin/env python3.4
#
#   Copyright 2016 - Google, Inc.
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.

import acts.jsonrpc as jsonrpc
from acts.test_utils.wifi.wifi_test_utils import WifiEnums

ACTS_CONTROLLER_CONFIG_NAME = "AP"
ACTS_CONTROLLER_REFERENCE_NAME = "access_points"

def create(configs, logger):
    results = []
    for c in configs:
        addr = c[Config.key_address.value]
        port = 80
        if Config.key_port.value in c:
            port = c[Config.key_port.value]
        results.append(AP(addr, port))
    return results

def destroy(objs):
    return

class ServerError(Exception):
    pass

class ClientError(Exception):
    pass

"""
Controller for OpenWRT routers.
"""
class AP():
    """Interface to OpenWRT using the LuCI interface.

    Works via JSON-RPC over HTTP. A generic access method is provided, as well
    as more specialized methods.

    Can also call LuCI methods generically:

        ap_instance.sys.loadavg()
        ap_instance.sys.dmesg()
        ap_instance.fs.stat("/etc/hosts")
    """
    IFACE_DEFAULTS = {"mode": "ap", "disabled": "0",
                      "encryption": "psk2", "network": "lan"}
    RADIO_DEFAULTS = {"disabled": "0"}

    def __init__(self, addr, port=80):
        self._client = jsonrpc.JSONRPCClient(
                        "http://""{}:{}/cgi-bin/luci/rpc/".format(addr, port))
        self.RADIO_NAMES = []
        keys = self._client.get_all("wireless").keys()
        if "radio0" in keys:
            self.RADIO_NAMES.append("radio0")
        if "radio1" in keys:
            self.RADIO_NAMES.append("radio1")

    def section_id_lookup(self, cfg_name, key, value):
        """Looks up the section id of a section.

        Finds the section ids of the sections that have the specified key:value
        pair in them.

        Args:
            cfg_name: Name of the configuration file to look in.
            key: Key of the pair.
            value: Value of the pair.

        Returns:
            A list of the section ids found.
        """
        section_ids = []
        sections = self._client.get_all(cfg_name)
        for section_id, section_cfg in sections.items():
            if key in section_cfg and section_cfg[key] == value:
                section_ids.append(section_id)
        return section_ids

    def _section_option_lookup(self, cfg_name, conditions, *target_keys):
        """Looks up values of options in sections that match the conditions.

        To match a condition, a section needs to have all the key:value pairs
        specified in conditions.

        Args:
            cfg_name: Name of the configuration file to look in.
            key: Key of the pair.
            value: Value of the pair.
            target_key: Key of the options we want to retrieve values from.

        Returns:
            A list of the values found.
        """
        results = []
        sections = self._client.get_all(cfg_name)
        for section_cfg in sections.values():
            if self._match_conditions(conditions, section_cfg):
                r = {}
                for k in target_keys:
                    if k not in section_cfg:
                        break
                    r[k] = section_cfg[k]
                if r:
                    results.append(r)
        return results

    @staticmethod
    def _match_conditions(conds, cfg):
        for cond in conds:
            key, value = cond
            if key not in cfg or cfg[key] != value:
                return False
        return True

    def run(self, *cmd):
        """Executes a terminal command on the AP.

        Args:
            cmd: A tuple of command strings.

        Returns:
            The terminal output of the command.
        """
        return self._client.sys("exec", *cmd)

    def apply_configs(self, ap_config):
        """Applies configurations to the access point.

        Reads the configuration file, adds wifi interfaces, and sets parameters
        based on the configuration file.

        Args:
            ap_config: A dict containing the configurations for the AP.
        """
        self.reset()
        for k, v in ap_config.items():
            if "radio" in k:
                self._apply_radio_configs(k, v)
            if "network" in k:
                # TODO(angli) Implement this.
                pass
        self.apply_wifi_changes()

    def _apply_radio_configs(self, radio_id, radio_config):
        """Applies conigurations on a radio of the AP.

        Sets the options in the radio config.
        Adds wifi-ifaces to this radio based on the configurations.
        """
        for k, v in radio_config.items():
            if k == "settings":
                self._set_options('wireless', radio_id, v,
                                  self.RADIO_DEFAULTS)
            if k == "wifi-iface":
                for cfg in v:
                    cfg["device"] = radio_id
                self._add_ifaces(v)

    def reset(self):
        """Resets the AP to a clean state.
        
        Deletes all wifi-ifaces.
        Enable all the radios.
        """
        sections = self._client.get_all("wireless")
        to_be_deleted = []
        for section_id in sections.keys():
            if section_id not in self.RADIO_NAMES:
                to_be_deleted.append(section_id)
        self.delete_ifaces_by_ids(to_be_deleted)
        for r in self.RADIO_NAMES:
            self.toggle_radio_state(r, True)

    def toggle_radio_state(self, radio_name, state=None):
        """Toggles the state of a radio.

        If input state is None, toggle the state of the radio.
        Otherwise, set the radio's state to input state.
        State True is equivalent to 'disabled':'0'

        Args:
            radio_name: Name of the radio to change state.
            state: State to set to, default is None.

        Raises:
            ClientError: If the radio specified does not exist on the AP.
        """
        if radio_name not in self.RADIO_NAMES:
            raise ClientError("Trying to change none-existent radio's state")
        cur_state = self._client.get("wireless", radio_name, "disabled")
        cur_state = True if cur_state=='0' else False
        if state == cur_state:
            return
        new_state = '1' if cur_state else '0'
        self._set_option("wireless", radio_name, "disabled", new_state)
        self.apply_wifi_changes()
        return

    def set_ssid_state(self, ssid, state):
        """Sets the state of ssid (turns on/off).

        Args:
            ssid: The ssid whose state is being changed.
            state: State to set the ssid to. Enable the ssid if True, disable
                otherwise.
        """
        new_state = '0' if state else '1'
        section_ids = self.section_id_lookup("wireless", "ssid", ssid)
        for s_id in section_ids:
            self._set_option("wireless", s_id, "disabled", new_state)

    def get_ssids(self, conds):
        """Gets all the ssids that match the conditions.

        Params:
            conds: An iterable of tuples, each representing a key:value pair
                an ssid must have to be included.

        Returns:
            A list of ssids that contain all the specified key:value pairs.
        """
        results = []
        for s in self._section_option_lookup("wireless", conds, "ssid"):
            results.append(s["ssid"])
        return results

    def get_active_ssids(self):
        """Gets the ssids that are currently not disabled.

        Returns:
            A list of ssids that are currently active.
        """
        conds = (("disabled", "0"),)
        return self.get_ssids(conds)

    def get_active_ssids_info(self, *keys):
        """Gets the specified info of currently active ssids

        If frequency is requested, it'll be retrieved from the radio section
        associated with this ssid.

        Params:
            keys: Names of the fields to include in the returned info.
                e.g. "frequency".

        Returns:
            Values of the requested info.
        """
        conds = (("disabled", "0"),)
        keys = [w.replace("frequency","device") for w in keys]
        if "device" not in keys:
            keys.append("device")
        info = self._section_option_lookup("wireless", conds, "ssid", *keys)
        results = []
        for i in info:
            radio = i["device"]
            # Skip this info the radio its ssid is on is disabled.
            disabled = self._client.get("wireless", radio, "disabled")
            if disabled != '0':
                continue
            c = int(self._client.get("wireless", radio, "channel"))
            if radio == "radio0":
                i["frequency"] = WifiEnums.channel_2G_to_freq[c]
            elif radio == "radio1":
                i["frequency"] = WifiEnums.channel_5G_to_freq[c]
            results.append(i)
        return results

    def get_radio_option(self, key, idx=0):
        """Gets an option from the configured settings of a radio.

        Params:
            key: Name of the option to retrieve.
            idx: Index of the radio to retrieve the option from. Default is 0.

        Returns:
            The value of the specified option and radio.
        """
        r = None
        if idx == 0:
            r = "radio0"
        elif idx == 1:
            r = "radio1"
        return self._client.get("wireless", r, key)

    def apply_wifi_changes(self):
        """Applies committed wifi changes by restarting wifi.

        Raises:
            ServerError: Something funny happened restarting wifi on the AP.
        """
        s = self._client.commit('wireless')
        resp = self.run('wifi')
        return resp
        # if resp != '' or not s:
        #     raise ServerError(("Exception in refreshing wifi changes, commit"
        #                        " status: ") + str(s) + ", wifi restart response: "
        #                        + str(resp))

    def set_wifi_channel(self, channel, device='radio0'):
        self.set('wireless', device, 'channel', channel)

    def _add_ifaces(self, configs):
        """Adds wifi-ifaces in the AP's wireless config based on a list of
        configuration dict.

        Args:
            configs: A list of dicts each representing a wifi-iface config.
        """
        for config in configs:
            self._add_cfg_section('wireless', 'wifi-iface',
                              config, self.IFACE_DEFAULTS)

    def _add_cfg_section(self, cfg_name, section, options, defaults=None):
        """Adds a section in a configuration file.

        Args:
            cfg_name: Name of the config file to add a section to.
                e.g. 'wireless'.
            section: Type of the secion to add. e.g. 'wifi-iface'.
            options: A dict containing all key:value pairs of the options.
                e.g. {'ssid': 'test', 'mode': 'ap'}

        Raises:
            ServerError: Uci add call returned False.
        """
        section_id = self._client.add(cfg_name, section)
        if not section_id:
            raise ServerError(' '.join(("Failed adding", section, "in",
                              cfg_name)))
        self._set_options(cfg_name, section_id, options, defaults)

    def _set_options(self, cfg_name, section_id, options, defaults):
        """Sets options in a section.

        Args:
            cfg_name: Name of the config file to add a section to.
                e.g. 'wireless'.
            section_id: ID of the secion to add options to. e.g. 'cfg000864'.
            options: A dict containing all key:value pairs of the options.
                e.g. {'ssid': 'test', 'mode': 'ap'}

        Raises:
            ServerError: Uci set call returned False.
        """
        # Fill the fields not defined in config with default values.
        if defaults:
            for k, v in defaults.items():
                if k not in options:
                    options[k] = v
        # Set value pairs defined in config.
        for k, v in options.items():
            self._set_option(cfg_name, section_id, k, v)

    def _set_option(self, cfg_name, section_id, k, v):
        """Sets an option in a config section.

        Args:
            cfg_name: Name of the config file the section is in.
                e.g. 'wireless'.
            section_id: ID of the secion to set option in. e.g. 'cfg000864'.
            k: Name of the option.
            v: Value to set the option to.

        Raises:
            ServerError: If the rpc called returned False.
        """
        status = self._client.set(cfg_name, section_id, k, v)
        if not status:
            # Delete whatever was added.
                raise ServerError(' '.join(("Failed adding option", str(k),
                                  ':', str(d), "to", str(section_id))))

    def delete_ifaces_by_ids(self, ids):
        """Delete wifi-ifaces that are specified by the ids from the AP's
        wireless config.

        Args:
            ids: A list of ids whose wifi-iface sections to be deleted.
        """
        for i in ids:
            self._delete_cfg_section_by_id('wireless', i)

    def delete_ifaces(self, key, value):
        """Delete wifi-ifaces that contain the specified key:value pair.

        Args:
            key: Key of the pair.
            value: Value of the pair.
        """
        self._delete_cfg_sections('wireless', key, value)

    def _delete_cfg_sections(self, cfg_name, key, value):
        """Deletes config sections that have the specified key:value pair.

        Finds the ids of sections that match a key:value pair in the specified
        config file and delete the section.

        Args:
            cfg_name: Name of the config file to delete sections from.
                e.g. 'wireless'.
            key: Name of the option to be matched.
            value: Value of the option to be matched.

        Raises:
            ClientError: Could not find any section that has the key:value
                pair.
        """
        section_ids = self.section_id_lookup(cfg_name, key, value)
        if not section_ids:
            raise ClientError(' '.join(("Could not find any section that has ",
                              key, ":", value)))
        for section_id in section_ids:
            self._delete_cfg_section_by_id(cfg_name, section_id)

    def _delete_cfg_section_by_id(self, cfg_name, section_id):
        """Deletes the config section with specified id.

        Args:
            cfg_name: Name of the config file to the delete a section from.
                e.g. 'wireless'.
            section_id: ID of the section to be deleted. e.g. 'cfg0d3777'.

        Raises:
            ServerError: Uci delete call returned False.
        """
        self._client.delete(cfg_name, section_id)

    def _get_iw_info(self):
        results = []
        text = self.run("iw dev").replace('\t', '')
        interfaces = text.split("Interface")
        for intf in interfaces:
            if len(intf.strip()) < 6:
                # This is a PHY mark.
                continue
            # This is an interface line.
            intf = intf.replace(', ', '\n')
            lines = intf.split('\n')
            r = {}
            for l in lines:
                if ' ' in l:
                    # Only the lines with space are processed.
                    k, v = l.split(' ', 1)
                    if k == "addr":
                        k = "bssid"
                    if "wlan" in v:
                        k = "interface"
                    if k == "channel":
                        vs = v.split(' ', 1)
                        v = int(vs[0])
                        r["frequency"] = int(vs[1].split(' ', 1)[0][1:5])
                    if k[-1] == ':':
                        k = k[:-1]
                    r[k] = v
            results.append(r)
        return results

    def get_active_bssids_info(self, radio, *args):
        wlan = None
        if radio == "radio0":
            wlan = "wlan0"
        if radio == "radio1":
            wlan = "wlan1"
        infos = self._get_iw_info()
        bssids = []
        for i in infos:
            if wlan in i["interface"]:
                r = {}
                for k,v in i.items():
                    if k in args:
                        r[k] = v
                r["bssid"] = i["bssid"].upper()
                bssids.append(r)
        return bssids

    def toggle_bssid_state(self, bssid):
        if bssid == self.get_bssid("radio0"):
            self.toggle_radio_state("radio0")
            return True
        elif bssid == self.get_bssid("radio1"):
            self.toggle_radio_state("radio1")
            return True
        return False

    def __getattr__(self, name):
        return _LibCaller(self._client, name)

class _LibCaller:
    def __init__(self, client, *args):
        self._client = client
        self._args = args

    def __getattr__(self, name):
        return _LibCaller(self._client, *self._args+(name,))

    def __call__(self, *args):
        return self._client.call("/".join(self._args[:-1]),
                                 self._args[-1],
                                 *args)
