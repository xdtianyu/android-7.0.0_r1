//
// Copyright (C) 2012 The Android Open Source Project
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

#ifndef SHILL_NET_RTNL_HANDLER_H_
#define SHILL_NET_RTNL_HANDLER_H_

#include <memory>
#include <set>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/lazy_instance.h>
#include <base/memory/ref_counted.h>
#include <gtest/gtest_prod.h>  // for FRIEND_TEST

#include "shill/net/io_handler_factory_container.h"
#include "shill/net/rtnl_listener.h"
#include "shill/net/rtnl_message.h"
#include "shill/net/shill_export.h"

namespace shill {

class Sockets;

// This singleton class is responsible for interacting with the RTNL subsystem.
// RTNL provides (among other things) access to interface discovery (add/remove
// events), interface state monitoring and the ability to change interace flags.
// Similar functionality also exists for IP address configuration for interfaces
// and IP routing tables.
//
// RTNLHandler provides access to these events through a callback system and
// provides utility functions to make changes to interface, address and routing
// state.
class SHILL_EXPORT RTNLHandler {
 public:
  // Request mask.
  static const int kRequestLink = 1;
  static const int kRequestAddr = 2;
  static const int kRequestRoute = 4;
  static const int kRequestRdnss = 8;
  static const int kRequestNeighbor = 16;
  static const int kRequestBridgeNeighbor = 32;

  typedef std::set<int> ErrorMask;

  virtual ~RTNLHandler();

  // Since this is a singleton, use RTNHandler::GetInstance()->Foo().
  static RTNLHandler* GetInstance();

  // This starts the event-monitoring function of the RTNL handler. This
  // function will create an IOHandler and add it to the current message
  // loop.
  virtual void Start(uint32_t netlink_groups_mask);

  // Add an RTNL event listener to the list of entities that will
  // be notified of RTNL events.
  virtual void AddListener(RTNLListener* to_add);

  // Remove a previously added RTNL event listener
  virtual void RemoveListener(RTNLListener* to_remove);

  // Set flags on a network interface that has a kernel index of
  // 'interface_index'.  Only the flags bits set in 'change' will
  // be set, and they will be set to the corresponding bit in 'flags'.
  virtual void SetInterfaceFlags(int interface_index,
                                 unsigned int flags,
                                 unsigned int change);

  // Set the maximum transmission unit (MTU) for the network interface that
  // has a kernel index of |interface_index|.
  virtual void SetInterfaceMTU(int interface_index, unsigned int mtu);

  // Set address of a network interface that has a kernel index of
  // 'interface_index'.
  virtual bool AddInterfaceAddress(int interface_index,
                                   const IPAddress& local,
                                   const IPAddress& gateway,
                                   const IPAddress& peer);

  // Remove address from a network interface that has a kernel index of
  // 'interface_index'.
  virtual bool RemoveInterfaceAddress(int interface_index,
                                      const IPAddress& local);

  // Remove a network interface from the kernel.
  virtual bool RemoveInterface(int interface_index);

  // Request that various tables (link, address, routing) tables be
  // exhaustively dumped via RTNL.  As results arrive from the kernel
  // they will be broadcast to all listeners.  The possible values
  // (multiple can be ORred together) are below.
  virtual void RequestDump(int request_flags);

  // Returns the index of interface |interface_name|, or -1 if unable to
  // determine the index.
  virtual int GetInterfaceIndex(const std::string& interface_name);

  // Send a formatted RTNL message.  Associates an error mask -- a list
  // of errors that are expected and should not trigger log messages by
  // default -- with the outgoing message.  If the message is sent
  // successfully, the sequence number in |message| is set, and the
  // function returns true.  Otherwise this function returns false.
  virtual bool SendMessageWithErrorMask(RTNLMessage* message,
                                        const ErrorMask& error_mask);

  // Sends a formatted RTNL message using SendMessageWithErrorMask
  // using an error mask inferred from the mode and type of |message|.
  virtual bool SendMessage(RTNLMessage* message);

 protected:
  RTNLHandler();

 private:
  friend struct base::DefaultLazyInstanceTraits<RTNLHandler>;
  friend class CellularTest;
  friend class DeviceInfoTest;
  friend class ModemTest;
  friend class RTNLHandlerTest;
  friend class RTNLListenerTest;
  friend class RoutingTableTest;

  FRIEND_TEST(RTNLListenerTest, NoRun);
  FRIEND_TEST(RTNLListenerTest, Run);
  FRIEND_TEST(RoutingTableTest, RouteDeleteForeign);

  static const int kReceiveBufferSize;
  static const int kInvalidSocket;

  // Size of the window for receiving error sequences out-of-order.
  static const int kErrorWindowSize;

  // This stops the event-monitoring function of the RTNL handler -- it is
  // private since it will never happen in normal running, but is useful for
  // tests.
  void Stop();

  // Dispatches an rtnl message to all listeners
  void DispatchEvent(int type, const RTNLMessage& msg);
  // Send the next table-dump request to the kernel
  void NextRequest(uint32_t seq);
  // Parse an incoming rtnl message from the kernel
  void ParseRTNL(InputData* data);

  bool AddressRequest(int interface_index,
                      RTNLMessage::Mode mode,
                      int flags,
                      const IPAddress& local,
                      const IPAddress& gateway,
                      const IPAddress& peer);

  // Called by the RTNL read handler on exceptional events.
  void OnReadError(const std::string& error_msg);

  // Returns whether |sequence| lies within the current error mask window.
  bool IsSequenceInErrorMaskWindow(uint32_t sequence);

  // Saves an error mask to be associated with this sequence number.
  void SetErrorMask(uint32_t sequence, const ErrorMask& error_mask);

  // Destructively retrieves the error mask associated with this sequeunce
  // number.  If this sequence number now lies outside the receive window
  // or no error mask was assigned, an empty ErrorMask is returned.
  ErrorMask GetAndClearErrorMask(uint32_t sequence);

  std::unique_ptr<Sockets> sockets_;
  bool in_request_;

  int rtnl_socket_;
  uint32_t request_flags_;
  uint32_t request_sequence_;
  uint32_t last_dump_sequence_;

  std::vector<RTNLListener*> listeners_;
  base::Callback<void(InputData*)> rtnl_callback_;
  std::unique_ptr<IOHandler> rtnl_handler_;
  IOHandlerFactory* io_handler_factory_;
  std::vector<ErrorMask> error_mask_window_;

  DISALLOW_COPY_AND_ASSIGN(RTNLHandler);
};

}  // namespace shill

#endif  // SHILL_NET_RTNL_HANDLER_H_
