#
# Copyright (C) 2012 The Android Open Source Project
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

# DEPRECATED
# Do not use flimflam.py in future development.
# Extend / migrate to shill_proxy suite of scripts instead.

import logging, time

import dbus

DEFAULT_CELLULAR_TIMEOUT = 60

def make_dbus_boolean(value):
    value = value.upper()
    if value in ["ON", "TRUE"]:
        return dbus.Boolean(1)
    elif value in ["OFF", "FALSE"]:
        return dbus.Boolean(0)
    else:
        return dbus.Boolean(int(value))

#
# Convert a DBus value to a printable value; used
# to print properties returned via DBus
#
def convert_dbus_value(value, indent=0):
    # DEPRECATED
    spacer = ' ' * indent
    if value.__class__ == dbus.Byte:
        return int(value)
    elif value.__class__ == dbus.Boolean:
        return bool(value)
    elif value.__class__ == dbus.Dictionary:
        valstr = "{"
        for key in value:
            valstr += "\n" + spacer + "    " + \
                key + ": " + str(convert_dbus_value(value[key], indent + 4))
        valstr += "\n" + spacer + "}"
        return valstr
    elif value.__class__ == dbus.Array:
        valstr = "["
        for val in value:
            valstr += "\n" + spacer + "    " + \
                str(convert_dbus_value(val, indent + 4))
        valstr += "\n" + spacer + "]"
        return valstr
    else:
        return str(value)

class FlimFlam(object):
    # DEPRECATED

    SHILL_DBUS_INTERFACE = "org.chromium.flimflam"
    UNKNOWN_METHOD = 'org.freedesktop.DBus.Error.UnknownMethod'
    UNKNOWN_OBJECT = 'org.freedesktop.DBus.Error.UnknownObject'

    DEVICE_WIMAX = 'wimax'
    DEVICE_CELLULAR = 'cellular'

    @staticmethod
    def _GetContainerName(kind):
        """Map shill element names to the names of their collections."""
        # For example, Device - > Devices.
        # Just pulling this out so we can use a map if we start
        # caring about "AvailableTechnologies"
        return kind + "s"

    @staticmethod
    def WaitForServiceState(service, expected_states, timeout,
                            ignore_failure=False, property_name="State"):
        """Wait until service enters a state in expected_states or times out.
        Args:
          service: service to watch
          expected_states: list of exit states
          timeout: in seconds
          ignore_failure: should the failure state be ignored?
          property_name: name of service property

        Returns: (state, seconds waited)

        If the state is "failure" and ignore_failure is False we return
        immediately without waiting for the timeout.
        """

        state = None
        start_time = time.time()
        timeout = start_time + timeout
        while time.time() < timeout:
            properties = service.GetProperties(utf8_strings = True)
            state = properties.get(property_name, None)
            if ((state == "failure" and not ignore_failure) or
                state in expected_states):
                break
            time.sleep(.5)

        config_time = time.time() - start_time
        # str() to remove DBus boxing
        return (str(state), config_time)

    @staticmethod
    def DisconnectService(service, wait_timeout=15):
        try:
            service.Disconnect()
        except dbus.exceptions.DBusException, error:
            if error.get_dbus_name() not in [
                    FlimFlam.SHILL_DBUS_INTERFACE + ".Error.InProgress",
                    FlimFlam.SHILL_DBUS_INTERFACE + ".Error.NotConnected", ]:
                raise error
        return FlimFlam.WaitForServiceState(service, ['idle'], wait_timeout)

    def __init__(self, bus=None):
        if not bus:
            bus = dbus.SystemBus()
        self.bus = bus
        shill = bus.get_object(FlimFlam.SHILL_DBUS_INTERFACE, "/")
        self.manager = dbus.Interface(
                shill,
                FlimFlam.SHILL_DBUS_INTERFACE + ".Manager")

    def _FindDevice(self, device_type, timeout):
        """ Return the first device object that matches a given device type.

            Wait until the device type is avilable or until timeout

            Args:
                device_type: string format of the type of device.
                timeout: in seconds

            Returns: Device or None
        """
        timeout = time.time() + timeout
        device_obj = None
        while time.time() < timeout:
            device_obj = self.FindElementByPropertySubstring('Device',
                                                             'Type',
                                                             device_type)
            if device_obj:
                break
            time.sleep(1)
        return device_obj

    def FindCellularDevice(self, timeout=DEFAULT_CELLULAR_TIMEOUT):
        return self._FindDevice(self.DEVICE_CELLULAR, timeout)

    def FindWimaxDevice(self, timeout=30):
        return self._FindDevice(self.DEVICE_WIMAX, timeout)

    def _FindService(self, device_type, timeout):
        """Return the first service object that matches the device type.

        Wait until a service is available or until the timeout.

        Args:
          device_type: string format of the type of device.
          timeout: in seconds

        Returns: service or None
        """
        start_time = time.time()
        timeout = start_time + timeout
        service = None
        while time.time() < timeout:
            service = self.FindElementByPropertySubstring('Service',
                                                          'Type', device_type)
            if service:
                break
            time.sleep(.5)
        return service

    def FindCellularService(self, timeout=DEFAULT_CELLULAR_TIMEOUT):
        return self._FindService(self.DEVICE_CELLULAR, timeout)

    def FindWimaxService(self, timeout=30):
        return self._FindService(self.DEVICE_WIMAX, timeout)

    def GetService(self, params):
        path = self.manager.GetService(params)
        return self.GetObjectInterface("Service", path)

    def ConnectService(self, assoc_timeout=15, config_timeout=15,
                       async=False, service=None, service_type='',
                       retry=False, retries=1, retry_sleep=15,
                       save_creds=False,
                       **kwargs):
        """Connect to a service and wait until connection is up
        Args:
          assoc_timeout, config_timeout:  Timeouts in seconds.
          async:  return immediately.  do not wait for connection.
          service: DBus service
          service_type:  If supplied, invoke type-specific code to find service.
          retry: Retry connection after Connect failure.
          retries: Number of retries to allow.
          retry_sleep: Number of seconds to wait before retrying.
          kwargs:  Additional args for type-specific code

        Returns:
          (success, dictionary), where dictionary contains stats and diagnostics.
        """
        output = {}
        connected_states = ["ready", "portal", "online"]

        # Retry connections on failure. Need to call GetService again as some
        # Connect failure states are unrecoverable.
        connect_success = False
        while not connect_success:
            if service_type == "wifi":
                try:
                    # Sanity check to make sure the caller hasn't provided
                    # both a service and a service type. At which point its
                    # unclear what they actually want to do, so err on the
                    # side of caution and except out.
                    if service:
                        raise Exception('supplied service and service type')
                    params = {
                        "Type": service_type,
                        "Mode": kwargs["mode"],
                        "SSID": kwargs["ssid"],
                        "Security": kwargs.get("security", "none"),
                        "SaveCredentials": save_creds }
                    # Supply a passphrase only if it is non-empty.
                    passphrase = kwargs.get("passphrase", "")
                    if passphrase:
                        params["Passphrase"] = passphrase
                    path = self.manager.GetService(params)
                    service = self.GetObjectInterface("Service", path)
                except Exception, e:
                    output["reason"] = "FAIL(GetService): exception %s" % e
                    return (False, output)

            output["service"] = service

            try:
                service.Connect()
                connect_success = True
            except Exception, e:
                if not retry or retries == 0:
                    output["reason"] = "FAIL(Connect): exception %s" % e
                    return (False, output)
                else:
                    logging.info("INFO(Connect): connect failed. Retrying...")
                    retries -= 1

            if not connect_success:
                # FlimFlam can be a little funny sometimes. At least for In
                # Progress errors, even though the service state may be failed,
                # it is actually still trying to connect. As such, while we're
                # waiting for retry, keep checking the service state to see if
                # it actually succeeded in connecting.
                state = FlimFlam.WaitForServiceState(
                        service=service,
                        expected_states=connected_states,
                        timeout=retry_sleep,
                        ignore_failure=True)[0]

                if state in connected_states:
                    return (True, output)

                # While service can be caller provided, it is also set by the
                # GetService call above. If service was not caller provided we
                # need to reset it to None so we don't fail the sanity check
                # above.
                if service_type != '':
                    service = None

        if async:
            return (True, output)

        logging.info("Associating...")
        (state, assoc_time) = (
            FlimFlam.WaitForServiceState(service,
                                         ["configuration"] + connected_states,
                                         assoc_timeout))
        output["state"] = state
        if state == "failure":
            output["reason"] = "FAIL(assoc)"
        if assoc_time > assoc_timeout:
            output["reason"] = "TIMEOUT(assoc)"
        output["assoc_time"] = assoc_time
        if "reason" in output:
            return (False, output)


        (state, config_time) = (
            FlimFlam.WaitForServiceState(service,
                                         connected_states, config_timeout))
        output["state"] = state
        if state == "failure":
            output["reason"] = "FAIL(config)"
        if config_time > config_timeout:
            output["reason"] = "TIMEOUT(config)"
        output["config_time"] = config_time

        if "reason" in output:
            return (False, output)

        return (True, output)

    def GetObjectInterface(self, kind, path):
        return dbus.Interface(
                self.bus.get_object(FlimFlam.SHILL_DBUS_INTERFACE, path),
                FlimFlam.SHILL_DBUS_INTERFACE + "." + kind)

    def FindElementByNameSubstring(self, kind, substring):
        properties = self.manager.GetProperties(utf8_strings = True)
        for path in properties[FlimFlam._GetContainerName(kind)]:
            if path.find(substring) >= 0:
                return self.GetObjectInterface(kind, path)
        return None

    def FindElementByPropertySubstring(self, kind, prop, substring):
        properties = self.manager.GetProperties(utf8_strings = True)
        for path in properties[FlimFlam._GetContainerName(kind)]:
            obj = self.GetObjectInterface(kind, path)
            try:
                obj_properties = obj.GetProperties(utf8_strings = True)
            except dbus.exceptions.DBusException, error:
                if (error.get_dbus_name() == self.UNKNOWN_METHOD or
                    error.get_dbus_name() == self.UNKNOWN_OBJECT):
                    # object disappeared; ignore and keep looking
                    continue
                else:
                    raise error
            if (prop in obj_properties and
                obj_properties[prop].find(substring) >= 0):
                return obj
        return None

    def GetObjectList(self, kind, properties=None):
        if properties is None:
            properties = self.manager.GetProperties(utf8_strings = True)
        return [self.GetObjectInterface(kind, path)
            for path in properties[FlimFlam._GetContainerName(kind)]]

    def GetActiveProfile(self):
        properties = self.manager.GetProperties(utf8_strings = True)
        return self.GetObjectInterface("Profile", properties["ActiveProfile"])

    def CreateProfile(self, ident):
        path = self.manager.CreateProfile(ident)
        return self.GetObjectInterface("Profile", path)

    def RemoveProfile(self, ident):
        self.manager.RemoveProfile(ident)

    def PushProfile(self, ident):
        path = self.manager.PushProfile(ident)
        return self.GetObjectInterface("Profile", path)

    def PopProfile(self, ident):
        self.manager.PopProfile(ident)

    def PopAnyProfile(self):
        self.manager.PopAnyProfile()

    def GetSystemState(self):
        properties = self.manager.GetProperties(utf8_strings = True)
        return properties["State"]

    def GetDebugTags(self):
        return self.manager.GetDebugTags()

    def ListDebugTags(self):
        return self.manager.ListDebugTags()

    def SetDebugTags(self, taglist):
        try:
            self.manager.SetDebugTags(taglist)
            self.SetDebugLevel(-4)
        except dbus.exceptions.DBusException, error:
            if error.get_dbus_name() not in [
              "org.freedesktop.DBus.Error.UnknownMethod" ]:
                raise error

    def SetDebugLevel(self, level):
        self.manager.SetDebugLevel(level)

    def GetServiceOrder(self):
        return self.manager.GetServiceOrder()

    def SetServiceOrder(self, new_order):
        old_order = self.GetServiceOrder()
        self.manager.SetServiceOrder(new_order)
        return (old_order, new_order)

    def EnableTechnology(self, tech):
        try:
            self.manager.EnableTechnology(tech)
        except dbus.exceptions.DBusException, error:
            if error.get_dbus_name() not in [
                    FlimFlam.SHILL_DBUS_INTERFACE + ".Error.AlreadyEnabled",
                    FlimFlam.SHILL_DBUS_INTERFACE + ".Error.InProgress" ]:
                raise error

    def DisableTechnology(self, tech):
        self.manager.DisableTechnology(tech, timeout=60)

    def RequestScan(self, technology):
        self.manager.RequestScan(technology)

    def GetCountry(self):
        properties = self.manager.GetProperties(utf8_strings = True)
        return properties["Country"]

    def SetCountry(self, country):
        self.manager.SetProperty("Country", country)

    def GetCheckPortalList(self):
        properties = self.manager.GetProperties(utf8_strings = True)
        return properties["CheckPortalList"]

    def SetCheckPortalList(self, tech_list):
        self.manager.SetProperty("CheckPortalList", tech_list)

    def GetPortalURL(self):
        properties = self.manager.GetProperties(utf8_strings = True)
        return properties["PortalURL"]

    def SetPortalURL(self, url):
        self.manager.SetProperty("PortalURL", url)

    def GetArpGateway(self):
        properties = self.manager.GetProperties()
        return properties["ArpGateway"]

    def SetArpGateway(self, do_arp_gateway):
        self.manager.SetProperty("ArpGateway", do_arp_gateway)


class DeviceManager(object):
    # DEPRECATED
    """Use flimflam to isolate a given interface for testing.

    DeviceManager can be used to turn off network devices that are not
    under test so that they will not interfere with testing.

    NB: Ethernet devices are special inside Flimflam.  You will need to
    take care of them via other means (like, for example, the
    backchannel ethernet code in client autotests)

    Sample usage:

      device_manager = flimflam.DeviceManager()
      try:
          device_manager.ShutdownAllExcept('cellular')
          use routing.getRouteFor()
             to verify that only the expected device is used
          do stuff to test cellular connections
      finally:
          device_manager.RestoreDevices()
    """

    @staticmethod
    def _EnableDevice(device, enable):
        """Enables/Disables a device in shill."""
        if enable:
            device.Enable()
        else:
            device.Disable()

    def __init__(self, flim=None):
        self.flim_ = flim or FlimFlam()
        self.devices_to_restore_ = []

    def ShutdownAllExcept(self, device_type):
        """Shutdown all devices except device_type ones."""
        for device in self.flim_.GetObjectList('Device'):
            device_properties = device.GetProperties(utf8_strings = True)
            if (device_properties["Type"] != device_type):
                logging.info("Powering off %s device %s",
                             device_properties["Type"],
                             device.object_path)
                self.devices_to_restore_.append(device.object_path)
                DeviceManager._EnableDevice(device, False)

    def RestoreDevices(self):
        """Restore devices powered down in ShutdownAllExcept."""
        should_raise = False
        to_raise = Exception("Nothing to raise")
        for device_path in self.devices_to_restore_:
            try:
                logging.info("Attempting to power on device %s", device_path)
                device = self.flim_.GetObjectInterface("Device", device_path)
                DeviceManager._EnableDevice(device, True)
            except Exception, e:
                # We want to keep on trying to power things on, so save an
                # exception and continue
                should_raise = True
                to_raise = e
        if should_raise:
            raise to_raise
