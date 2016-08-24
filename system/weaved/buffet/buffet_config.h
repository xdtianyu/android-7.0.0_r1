// Copyright 2015 The Android Open Source Project
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

#ifndef BUFFET_BUFFET_CONFIG_H_
#define BUFFET_BUFFET_CONFIG_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include <base/callback.h>
#include <base/files/file_path.h>
#include <brillo/errors/error.h>
#include <brillo/key_value_store.h>
#include <weave/provider/config_store.h>

#include "buffet/encryptor.h"

namespace buffet {

class StorageInterface;

// Handles reading buffet config and state files.
class BuffetConfig final : public weave::provider::ConfigStore {
 public:
  struct Options {
    std::string client_id;
    std::string client_secret;
    std::string api_key;
    std::string oauth_url;
    std::string service_url;

    base::FilePath defaults;
    base::FilePath settings;

    base::FilePath definitions;
    base::FilePath test_definitions;

    std::string test_privet_ssid;
  };

  // An IO abstraction to enable testing without using real files.
  class FileIO {
   public:
    virtual bool ReadFile(const base::FilePath& path, std::string* content) = 0;
    virtual bool WriteFile(const base::FilePath& path,
                           const std::string& content) = 0;
  };

  ~BuffetConfig() override = default;

  explicit BuffetConfig(const Options& options);

  // Config overrides.
  bool LoadDefaults(weave::Settings* settings) override;
  std::string LoadSettings(const std::string& name) override;
  std::string LoadSettings() override;
  void SaveSettings(const std::string& name,
                    const std::string& settings,
                    const weave::DoneCallback& callback) override;

  bool LoadDefaults(const brillo::KeyValueStore& store,
                    weave::Settings* settings);

  // Allows injection of a non-default |encryptor| for testing. The caller
  // retains ownership of the pointer.
  void SetEncryptor(Encryptor* encryptor) {
    encryptor_ = encryptor;
  }

  // Allows injection of non-default |file_io| for testing. The caller retains
  // ownership of the pointer.
  void SetFileIO(FileIO* file_io) {
    file_io_ = file_io;
  }

 private:
  base::FilePath CreatePath(const std::string& name) const;
  bool LoadFile(const base::FilePath& file_path,
                std::string* data,
                brillo::ErrorPtr* error);

  Options options_;
  std::unique_ptr<Encryptor> default_encryptor_;
  Encryptor* encryptor_{nullptr};
  std::unique_ptr<FileIO> default_file_io_;
  FileIO* file_io_{nullptr};

  DISALLOW_COPY_AND_ASSIGN(BuffetConfig);
};

}  // namespace buffet

#endif  // BUFFET_BUFFET_CONFIG_H_
