/*
 * Copyright (C) 2008 The Android Open Source Project
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

#ifndef _TETHER_CONTROLLER_H
#define _TETHER_CONTROLLER_H

#include <netinet/in.h>

#include <list>
#include <set>
#include <string>

typedef std::list<char *> InterfaceCollection;
typedef std::list<std::string> NetAddressCollection;

class TetherController {
    InterfaceCollection  *mInterfaces;
    // NetId to use for forwarded DNS queries. This may not be the default
    // network, e.g., in the case where we are tethering to a DUN APN.
    unsigned              mDnsNetId;
    NetAddressCollection *mDnsForwarders;
    pid_t                 mDaemonPid;
    int                   mDaemonFd;
    std::set<std::string> mForwardingRequests;

public:
    TetherController();
    virtual ~TetherController();

    bool enableForwarding(const char* requester);
    bool disableForwarding(const char* requester);
    size_t forwardingRequestCount();

    int startTethering(int num_addrs, char **dhcp_ranges);
    int stopTethering();
    bool isTetheringStarted();

    unsigned getDnsNetId();
    int setDnsForwarders(unsigned netId, char **servers, int numServers);
    NetAddressCollection *getDnsForwarders();

    int tetherInterface(const char *interface);
    int untetherInterface(const char *interface);
    InterfaceCollection *getTetheredInterfaceList();

private:
    int applyDnsInterfaces();
    bool setIpFwdEnabled();
};

#endif
