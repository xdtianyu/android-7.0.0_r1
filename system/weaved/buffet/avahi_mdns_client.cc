/*
 * Copyright 2015 The Android Open Source Project
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

#include <vector>

#include "buffet/avahi_mdns_client.h"

#include <avahi-common/address.h>
#include <avahi-common/defs.h>
#include <avahi-common/error.h>

#include <base/guid.h>
#include <brillo/errors/error.h>

using brillo::ErrorPtr;

namespace buffet {

std::unique_ptr<MdnsClient> MdnsClient::CreateInstance() {
  return std::unique_ptr<MdnsClient>{new AvahiMdnsClient()};

}

namespace {

void HandleGroupStateChanged(AvahiEntryGroup* g,
                             AvahiEntryGroupState state,
                             AVAHI_GCC_UNUSED void* userdata) {
  if (state == AVAHI_ENTRY_GROUP_COLLISION ||
      state == AVAHI_ENTRY_GROUP_FAILURE) {
    LOG(ERROR) << "Avahi service group error: " << state;
  }
}

}  // namespace

AvahiMdnsClient::AvahiMdnsClient()
    : service_name_(base::GenerateGUID()) {
  thread_pool_.reset(avahi_threaded_poll_new());
  CHECK(thread_pool_);

  int ret = 0;

  client_.reset(avahi_client_new(
      avahi_threaded_poll_get(thread_pool_.get()), {},
      &AvahiMdnsClient::OnAvahiClientStateUpdate, this, &ret));
  CHECK(client_) << avahi_strerror(ret);

  avahi_threaded_poll_start(thread_pool_.get());

  group_.reset(avahi_entry_group_new(client_.get(), HandleGroupStateChanged,
                                     nullptr));
  CHECK(group_) << avahi_strerror(avahi_client_errno(client_.get()))
                << ". Check avahi-daemon configuration";
}

AvahiMdnsClient::~AvahiMdnsClient() {
  if (thread_pool_)
    avahi_threaded_poll_stop(thread_pool_.get());
}

void AvahiMdnsClient::PublishService(const std::string& service_type,
                                     uint16_t port,
                                     const std::vector<std::string>& txt) {
  CHECK(group_);
  CHECK_EQ("_privet._tcp", service_type);

  if (prev_port_ == port && prev_service_type_ == service_type &&
      txt_records_ == txt) {
    return;
  }

  // Create txt record.
  std::unique_ptr<AvahiStringList, decltype(&avahi_string_list_free)> txt_list{
      nullptr, &avahi_string_list_free};

  if (!txt.empty()) {
    std::vector<const char*> txt_vector_ptr;

    for (const auto& i : txt)
      txt_vector_ptr.push_back(i.c_str());

    txt_list.reset(avahi_string_list_new_from_array(txt_vector_ptr.data(),
                                                    txt_vector_ptr.size()));
    CHECK(txt_list);
  }

  int ret = 0;
  txt_records_ = txt;

  if (prev_port_ == port && prev_service_type_ == service_type) {
    ret = avahi_entry_group_update_service_txt_strlst(
        group_.get(), AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, {},
        service_name_.c_str(), service_type.c_str(), nullptr, txt_list.get());

    CHECK_GE(ret, 0) << avahi_strerror(ret);
  } else {
    prev_port_ = port;
    prev_service_type_ = service_type;

    avahi_entry_group_reset(group_.get());
    CHECK(avahi_entry_group_is_empty(group_.get()));

    ret = avahi_entry_group_add_service_strlst(
        group_.get(), AVAHI_IF_UNSPEC, AVAHI_PROTO_UNSPEC, {},
        service_name_.c_str(), service_type.c_str(), nullptr, nullptr, port,
        txt_list.get());
    CHECK_GE(ret, 0) << avahi_strerror(ret);

    ret = avahi_entry_group_commit(group_.get());
    CHECK_GE(ret, 0) << avahi_strerror(ret);
  }
}

void AvahiMdnsClient::StopPublishing(const std::string& service_type) {
  CHECK(group_);
  avahi_entry_group_reset(group_.get());
  prev_service_type_.clear();
  prev_port_ = 0;
  txt_records_.clear();
}

void AvahiMdnsClient::OnAvahiClientStateUpdate(AvahiClient* s,
                                               AvahiClientState state,
                                               void* userdata) {
  // Avahi service has been re-initialized (probably due to host name conflict),
  // so we need to republish the service if it has been previously published.
  if (state == AVAHI_CLIENT_S_RUNNING) {
    AvahiMdnsClient* self = static_cast<AvahiMdnsClient*>(userdata);
    self->RepublishService();
  }
}

void AvahiMdnsClient::RepublishService() {
  // If we don't have a service to publish, there is nothing else to do here.
  if (prev_service_type_.empty())
    return;

  LOG(INFO) << "Republishing mDNS service";
  std::string service_type = std::move(prev_service_type_);
  uint16_t port = prev_port_;
  std::vector<std::string> txt = std::move(txt_records_);
  StopPublishing(service_type);
  PublishService(service_type, port, txt);
}

}  // namespace buffet
