// Copyright 2015 The Weave Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef LIBWEAVE_EXAMPLES_PROVIDER_FILE_CONFIG_STORE_H_
#define LIBWEAVE_EXAMPLES_PROVIDER_FILE_CONFIG_STORE_H_

#include <map>
#include <string>
#include <vector>

#include <weave/provider/config_store.h>
#include <weave/provider/task_runner.h>

namespace weave {
namespace examples {

class FileConfigStore : public provider::ConfigStore {
 public:
  FileConfigStore(const std::string& model_id,
                  provider::TaskRunner* task_runner);

  bool LoadDefaults(Settings* settings) override;
  std::string LoadSettings(const std::string& name) override;
  void SaveSettings(const std::string& name,
                    const std::string& settings,
                    const DoneCallback& callback) override;

  std::string LoadSettings() override;

 private:
  std::string GetPath(const std::string& name) const;
  const std::string model_id_;
  provider::TaskRunner* task_runner_{nullptr};
};

}  // namespace examples
}  // namespace weave

#endif  // LIBWEAVE_EXAMPLES_PROVIDER_FILE_CONFIG_STORE_H_
