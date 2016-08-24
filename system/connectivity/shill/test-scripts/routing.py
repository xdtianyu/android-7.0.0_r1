#!/usr/bin/env python
#
# Copyright (C) 2009 The Android Open Source Project
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

"""Return information about routing table entries

Read and parse the system routing table. There are
two classes defined here: NetworkRoutes, which contains
information about all routes, and Route, which describes
a single routing table entry.
"""

ROUTES_FILE = "/proc/net/route"
# The following constants are from <net/route.h>
RTF_UP      = 0x0001
RTF_GATEWAY = 0x0002
RTF_HOST    = 0x0004

import socket
import struct

def intToDottedQuad(addr):
    return socket.inet_ntoa(struct.pack('@I', addr))

def convertIpToInt(i):
    """Convert the supplied argument to an int representing an IP address."""
    if isinstance(i, int):
        return i
    return struct.unpack('I', socket.inet_aton(i))[0]

class Route(object):
    def __init__(self, iface, dest, gway, flags, mask):
        self.interface = iface
        self.destination = int(dest, 16)
        self.gateway = int(gway, 16)
        self.flagbits = int(flags, 16)
        self.netmask = int(mask, 16)

    def __str__(self):
        flags = ""
        if self.flagbits & RTF_UP:
            flags += "U"
        if self.flagbits & RTF_GATEWAY:
            flags += "G"
        if self.flagbits & RTF_HOST:
            flags += "H"
        return "<%s dest: %s gway: %s mask: %s flags: %s>" % (
                self.interface,
                intToDottedQuad(self.destination),
                intToDottedQuad(self.gateway),
                intToDottedQuad(self.netmask),
                flags)

    def isUsable(self):
        return self.flagbits & RTF_UP

    def isHostRoute(self):
        return self.flagbits & RTF_HOST

    def isGatewayRoute(self):
        return self.flagbits & RTF_GATEWAY

    def isInterfaceRoute(self):
        return (self.flagbits & RTF_GATEWAY) == 0

    def isDefaultRoute(self):
        return (self.flagbits & RTF_GATEWAY) and self.destination == 0

    def matches(self, ip):
        return (ip & self.netmask) == self.destination

class NetworkRoutes(object):
    def __init__(self, routelist=None):
        if not routelist:
            routef = open(ROUTES_FILE)
            routelist = routef.readlines()
            routef.close()

        # The first line is headers that will allow us
        # to correctly interpret the values in the following
        # lines
        colMap = {}
        headers = routelist[0].split()
        for (pos, token) in enumerate(headers):
            colMap[token] = pos

        self.routes = []
        for routeline in routelist[1:]:
            route = routeline.split()
            interface = route[colMap["Iface"]]
            destination = route[colMap["Destination"]]
            gateway = route[colMap["Gateway"]]
            flags = route[colMap["Flags"]]
            mask = route[colMap["Mask"]]
            self.routes.append(
                    Route(interface, destination, gateway, flags, mask))

    def hasDefaultRoute(self, interface):
        for rr in self.routes:
            if (rr.isUsable() and
                    rr.interface == interface and
                    rr.isDefaultRoute()):
                return True
        return False

    def getDefaultRoutes(self):
        defroutes = []
        for rr in self.routes:
            if rr.isUsable() and rr.isDefaultRoute():
                defroutes.append(rr)
        return defroutes

    def hasInterfaceRoute(self, interface):
        for rr in self.routes:
            if (rr.isUsable() and
                    rr.interface == interface and
                    rr.isInterfaceRoute()):
                return True
            return False

    def getRouteFor(self, ip_as_int_or_string):
        ip = convertIpToInt(ip_as_int_or_string)
        for rr in self.routes:
            if rr.isUsable() and rr.matches(ip):
                return rr
        return None


if __name__ == "__main__":
    routes = NetworkRoutes()
    if routes == None:
        print "Failed to read routing table"
    else:
        for each_route in routes.routes:
            print each_route

        print "hasDefaultRoute(\"eth0\"):", routes.hasDefaultRoute("eth0")

        dflts = routes.getDefaultRoutes()
        if dflts == None:
            print "There are no default routes"
        else:
            print "There are %d default routes" % (len(dflts))


        print "hasInterfaceRoute(\"eth0\"):", routes.hasInterfaceRoute("eth0")

    routes = NetworkRoutes([
        "Iface Destination Gateway  Flags RefCnt "
        "Use Metric Mask MTU Window IRTT",
        "ones 00010203 FE010203 0007 0 0 0 00FFFFFF 0 0 0\n",
        "default 00000000 09080706 0007 0 0 0 00000000 0 0 0\n",
        ])

    print routes.getRouteFor(0x01010203)
    print routes.getRouteFor("3.2.1.1")
    print routes.getRouteFor(0x08010209)
