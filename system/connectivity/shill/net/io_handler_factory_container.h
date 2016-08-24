//
// Copyright (C) 2014 The Android Open Source Project
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

#ifndef SHILL_NET_IO_HANDLER_FACTORY_CONTAINER_H_
#define SHILL_NET_IO_HANDLER_FACTORY_CONTAINER_H_

#include <memory>

#include <base/lazy_instance.h>

#include "shill/net/io_handler_factory.h"

namespace shill {

// By default, this container will use the IOHandlerFactory that uses
// libbase's FileDescriptorWatcher. The caller can implement their own
// IOHandlerFactory and overwrite the default using SetIOHandlerFactory.
class SHILL_EXPORT IOHandlerFactoryContainer {
 public:
  virtual ~IOHandlerFactoryContainer();

  // This is a singleton. Use IOHandlerFactoryContainer::GetInstance()->Foo().
  static IOHandlerFactoryContainer* GetInstance();

  // Update the default IOHandlerFactory for creating IOHandlers. This
  // container will assume the ownership of the passed in |factory|.
  void SetIOHandlerFactory(IOHandlerFactory* factory);

  IOHandlerFactory* GetIOHandlerFactory();

 protected:
  IOHandlerFactoryContainer();

 private:
  friend struct base::DefaultLazyInstanceTraits<IOHandlerFactoryContainer>;
  std::unique_ptr<IOHandlerFactory> factory_;

  DISALLOW_COPY_AND_ASSIGN(IOHandlerFactoryContainer);
};

}  // namespace shill

#endif  // SHILL_NET_IO_HANDLER_FACTORY_CONTAINER_H_
