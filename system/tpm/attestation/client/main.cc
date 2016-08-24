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

#include <stdio.h>
#include <sysexits.h>

#include <memory>
#include <string>

#include <base/command_line.h>
#include <base/files/file_util.h>
#include <base/message_loop/message_loop.h>
#include <brillo/bind_lambda.h>
#include <brillo/daemons/daemon.h>
#include <brillo/syslog_logging.h>

#include "attestation/client/dbus_proxy.h"
#include "attestation/common/attestation_ca.pb.h"
#include "attestation/common/crypto_utility_impl.h"
#include "attestation/common/interface.pb.h"
#include "attestation/common/print_interface_proto.h"

namespace attestation {

const char kCreateAndCertifyCommand[] = "create_and_certify";
const char kCreateCommand[] = "create";
const char kInfoCommand[] = "info";
const char kEndorsementCommand[] = "endorsement";
const char kAttestationKeyCommand[] = "attestation_key";
const char kActivateCommand[] = "activate";
const char kEncryptForActivateCommand[] = "encrypt_for_activate";
const char kEncryptCommand[] = "encrypt";
const char kDecryptCommand[] = "decrypt";
const char kSignCommand[] = "sign";
const char kVerifyCommand[] = "verify";
const char kRegisterCommand[] = "register";
const char kUsage[] = R"(
Usage: attestation_client <command> [<args>]
Commands:
  create_and_certify [--user=<email>] [--label=<keylabel>]
      Creates a key and requests certification by the Google Attestation CA.
      This is the default command.
  create [--user=<email>] [--label=<keylabel] [--usage=sign|decrypt]
      Creates a certifiable key.

  info [--user=<email>] [--label=<keylabel>]
      Prints info about a key.
  endorsement
      Prints info about the TPM endorsement.
  attestation_key
      Prints info about the TPM attestation key.

  activate --input=<input_file>
      Activates an attestation key using the encrypted credential in
      |input_file|.
  encrypt_for_activate --input=<input_file> --output=<output_file>
      Encrypts the content of |input_file| as required by the TPM for activating
      an attestation key. The result is written to |output_file|.

  encrypt [--user=<email>] [--label=<keylabel>] --input=<input_file>
          --output=<output_file>
      Encrypts the contents of |input_file| as required by the TPM for a decrypt
      operation. The result is written to |output_file|.
  decrypt [--user=<email>] [--label=<keylabel>] --input=<input_file>
      Decrypts the contents of |input_file|.

  sign [--user=<email>] [--label=<keylabel>] --input=<input_file>
          [--output=<output_file>]
      Signs the contents of |input_file|.
  verify [--user=<email>] [--label=<keylabel] --input=<signed_data_file>
          --signature=<signature_file>
      Verifies the signature in |signature_file| against the contents of
      |input_file|.

  register [--user=<email>] [--label=<keylabel]
      Registers a key with a PKCS #11 token.
)";

// The Daemon class works well as a client loop as well.
using ClientLoopBase = brillo::Daemon;

class ClientLoop : public ClientLoopBase {
 public:
  ClientLoop() = default;
  ~ClientLoop() override = default;

 protected:
  int OnInit() override {
    int exit_code = ClientLoopBase::OnInit();
    if (exit_code != EX_OK) {
      return exit_code;
    }
    attestation_.reset(new attestation::DBusProxy());
    if (!attestation_->Initialize()) {
      return EX_UNAVAILABLE;
    }
    exit_code = ScheduleCommand();
    if (exit_code == EX_USAGE) {
      printf("%s", kUsage);
    }
    return exit_code;
  }

  void OnShutdown(int* exit_code) override {
    attestation_.reset();
    ClientLoopBase::OnShutdown(exit_code);
  }

 private:
  // Posts tasks according to the command line options.
  int ScheduleCommand() {
    base::Closure task;
    base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
    const auto& args = command_line->GetArgs();
    if (command_line->HasSwitch("help") || command_line->HasSwitch("h") ||
        (!args.empty() && args.front() == "help")) {
      return EX_USAGE;
    }
    if (args.empty() || args.front() == kCreateAndCertifyCommand) {
      task = base::Bind(&ClientLoop::CallCreateGoogleAttestedKey,
                        weak_factory_.GetWeakPtr(),
                        command_line->GetSwitchValueASCII("label"),
                        command_line->GetSwitchValueASCII("user"));
    } else if (args.front() == kCreateCommand) {
      std::string usage_str = command_line->GetSwitchValueASCII("usage");
      KeyUsage usage;
      if (usage_str.empty() || usage_str == "sign") {
        usage = KEY_USAGE_SIGN;
      } else if (usage_str == "decrypt") {
        usage = KEY_USAGE_DECRYPT;
      } else {
        return EX_USAGE;
      }
      task = base::Bind(&ClientLoop::CallCreateCertifiableKey,
                        weak_factory_.GetWeakPtr(),
                        command_line->GetSwitchValueASCII("label"),
                        command_line->GetSwitchValueASCII("user"),
                        usage);
    } else if (args.front() == kInfoCommand) {
      task = base::Bind(&ClientLoop::CallGetKeyInfo,
                        weak_factory_.GetWeakPtr(),
                        command_line->GetSwitchValueASCII("label"),
                        command_line->GetSwitchValueASCII("user"));
    } else if (args.front() == kEndorsementCommand) {
      task = base::Bind(&ClientLoop::CallGetEndorsementInfo,
                        weak_factory_.GetWeakPtr());
    } else if (args.front() == kAttestationKeyCommand) {
      task = base::Bind(&ClientLoop::CallGetAttestationKeyInfo,
                        weak_factory_.GetWeakPtr());
    } else if (args.front() == kActivateCommand) {
      if (!command_line->HasSwitch("input")) {
        return EX_USAGE;
      }
      std::string input;
      base::FilePath filename(command_line->GetSwitchValueASCII("input"));
      if (!base::ReadFileToString(filename, &input)) {
        LOG(ERROR) << "Failed to read file: " << filename.value();
        return EX_NOINPUT;
      }
      task = base::Bind(&ClientLoop::CallActivateAttestationKey,
                        weak_factory_.GetWeakPtr(),
                        input);
    } else if (args.front() == kEncryptForActivateCommand) {
      if (!command_line->HasSwitch("input") ||
          !command_line->HasSwitch("output")) {
        return EX_USAGE;
      }
      std::string input;
      base::FilePath filename(command_line->GetSwitchValueASCII("input"));
      if (!base::ReadFileToString(filename, &input)) {
        LOG(ERROR) << "Failed to read file: " << filename.value();
        return EX_NOINPUT;
      }
      task = base::Bind(&ClientLoop::EncryptForActivate,
                        weak_factory_.GetWeakPtr(),
                        input);
    } else if (args.front() == kEncryptCommand) {
      if (!command_line->HasSwitch("input") ||
          !command_line->HasSwitch("output")) {
        return EX_USAGE;
      }
      std::string input;
      base::FilePath filename(command_line->GetSwitchValueASCII("input"));
      if (!base::ReadFileToString(filename, &input)) {
        LOG(ERROR) << "Failed to read file: " << filename.value();
        return EX_NOINPUT;
      }
      task = base::Bind(&ClientLoop::Encrypt,
                        weak_factory_.GetWeakPtr(),
                        command_line->GetSwitchValueASCII("label"),
                        command_line->GetSwitchValueASCII("user"),
                        input);
    } else if (args.front() == kDecryptCommand) {
      if (!command_line->HasSwitch("input")) {
        return EX_USAGE;
      }
      std::string input;
      base::FilePath filename(command_line->GetSwitchValueASCII("input"));
      if (!base::ReadFileToString(filename, &input)) {
        LOG(ERROR) << "Failed to read file: " << filename.value();
        return EX_NOINPUT;
      }
      task = base::Bind(&ClientLoop::CallDecrypt,
                        weak_factory_.GetWeakPtr(),
                        command_line->GetSwitchValueASCII("label"),
                        command_line->GetSwitchValueASCII("user"),
                        input);
    } else if (args.front() == kSignCommand) {
      if (!command_line->HasSwitch("input")) {
        return EX_USAGE;
      }
      std::string input;
      base::FilePath filename(command_line->GetSwitchValueASCII("input"));
      if (!base::ReadFileToString(filename, &input)) {
        LOG(ERROR) << "Failed to read file: " << filename.value();
        return EX_NOINPUT;
      }
      task = base::Bind(&ClientLoop::CallSign,
                        weak_factory_.GetWeakPtr(),
                        command_line->GetSwitchValueASCII("label"),
                        command_line->GetSwitchValueASCII("user"),
                        input);
    } else if (args.front() == kVerifyCommand) {
      if (!command_line->HasSwitch("input") ||
          !command_line->HasSwitch("signature")) {
        return EX_USAGE;
      }
      std::string input;
      base::FilePath filename(command_line->GetSwitchValueASCII("input"));
      if (!base::ReadFileToString(filename, &input)) {
        LOG(ERROR) << "Failed to read file: " << filename.value();
        return EX_NOINPUT;
      }
      std::string signature;
      base::FilePath filename2(command_line->GetSwitchValueASCII("signature"));
      if (!base::ReadFileToString(filename2, &signature)) {
        LOG(ERROR) << "Failed to read file: " << filename2.value();
        return EX_NOINPUT;
      }
      task = base::Bind(&ClientLoop::VerifySignature,
                        weak_factory_.GetWeakPtr(),
                        command_line->GetSwitchValueASCII("label"),
                        command_line->GetSwitchValueASCII("user"),
                        input,
                        signature);
    } else if (args.front() == kRegisterCommand) {
      task = base::Bind(&ClientLoop::CallRegister,
                        weak_factory_.GetWeakPtr(),
                        command_line->GetSwitchValueASCII("label"),
                        command_line->GetSwitchValueASCII("user"));
    } else {
      return EX_USAGE;
    }
    base::MessageLoop::current()->PostTask(FROM_HERE, task);
    return EX_OK;
  }

  template <typename ProtobufType>
  void PrintReplyAndQuit(const ProtobufType& reply) {
    printf("%s\n", GetProtoDebugString(reply).c_str());
    Quit();
  }

  void WriteOutput(const std::string& output) {
    base::FilePath filename(base::CommandLine::ForCurrentProcess()->
        GetSwitchValueASCII("output"));
    if (base::WriteFile(filename, output.data(), output.size()) !=
        static_cast<int>(output.size())) {
      LOG(ERROR) << "Failed to write file: " << filename.value();
      QuitWithExitCode(EX_IOERR);
    }
  }

  void CallCreateGoogleAttestedKey(const std::string& label,
                                   const std::string& username) {
    CreateGoogleAttestedKeyRequest request;
    request.set_key_label(label);
    request.set_key_type(KEY_TYPE_RSA);
    request.set_key_usage(KEY_USAGE_SIGN);
    request.set_certificate_profile(ENTERPRISE_MACHINE_CERTIFICATE);
    request.set_username(username);
    attestation_->CreateGoogleAttestedKey(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<CreateGoogleAttestedKeyReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void CallGetKeyInfo(const std::string& label, const std::string& username) {
    GetKeyInfoRequest request;
    request.set_key_label(label);
    request.set_username(username);
    attestation_->GetKeyInfo(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<GetKeyInfoReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void CallGetEndorsementInfo() {
    GetEndorsementInfoRequest request;
    request.set_key_type(KEY_TYPE_RSA);
    attestation_->GetEndorsementInfo(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<GetEndorsementInfoReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void CallGetAttestationKeyInfo() {
    GetAttestationKeyInfoRequest request;
    request.set_key_type(KEY_TYPE_RSA);
    attestation_->GetAttestationKeyInfo(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<GetAttestationKeyInfoReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void CallActivateAttestationKey(const std::string& input) {
    ActivateAttestationKeyRequest request;
    request.set_key_type(KEY_TYPE_RSA);
    request.mutable_encrypted_certificate()->ParseFromString(input);
    request.set_save_certificate(true);
    attestation_->ActivateAttestationKey(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<ActivateAttestationKeyReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void EncryptForActivate(const std::string& input) {
    GetEndorsementInfoRequest request;
    request.set_key_type(KEY_TYPE_RSA);
    attestation_->GetEndorsementInfo(
        request,
        base::Bind(&ClientLoop::EncryptForActivate2,
                   weak_factory_.GetWeakPtr(),
                   input));
  }

  void EncryptForActivate2(const std::string& input,
                           const GetEndorsementInfoReply& endorsement_info) {
    if (endorsement_info.status() != STATUS_SUCCESS) {
      PrintReplyAndQuit(endorsement_info);
    }
    GetAttestationKeyInfoRequest request;
    request.set_key_type(KEY_TYPE_RSA);
    attestation_->GetAttestationKeyInfo(
        request,
        base::Bind(&ClientLoop::EncryptForActivate3,
                   weak_factory_.GetWeakPtr(),
                   input,
                   endorsement_info));
  }

  void EncryptForActivate3(
      const std::string& input,
      const GetEndorsementInfoReply& endorsement_info,
      const GetAttestationKeyInfoReply& attestation_key_info) {
    if (attestation_key_info.status() != STATUS_SUCCESS) {
      PrintReplyAndQuit(attestation_key_info);
    }
    CryptoUtilityImpl crypto(nullptr);
    EncryptedIdentityCredential encrypted;
    if (!crypto.EncryptIdentityCredential(
        input,
        endorsement_info.ek_public_key(),
        attestation_key_info.public_key_tpm_format(),
        &encrypted)) {
      QuitWithExitCode(EX_SOFTWARE);
    }
    std::string output;
    encrypted.SerializeToString(&output);
    WriteOutput(output);
    Quit();
  }

  void CallCreateCertifiableKey(const std::string& label,
                                const std::string& username,
                                KeyUsage usage) {
    CreateCertifiableKeyRequest request;
    request.set_key_label(label);
    request.set_username(username);
    request.set_key_type(KEY_TYPE_RSA);
    request.set_key_usage(usage);
    attestation_->CreateCertifiableKey(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<CreateCertifiableKeyReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void Encrypt(const std::string& label,
               const std::string& username,
               const std::string& input) {
    GetKeyInfoRequest request;
    request.set_key_label(label);
    request.set_username(username);
    attestation_->GetKeyInfo(request, base::Bind(&ClientLoop::Encrypt2,
                                                 weak_factory_.GetWeakPtr(),
                                                 input));
  }

  void Encrypt2(const std::string& input,
                const GetKeyInfoReply& key_info) {
    CryptoUtilityImpl crypto(nullptr);
    std::string output;
    if (!crypto.EncryptForUnbind(key_info.public_key(), input, &output)) {
      QuitWithExitCode(EX_SOFTWARE);
    }
    WriteOutput(output);
    Quit();
  }

  void CallDecrypt(const std::string& label,
                   const std::string& username,
                   const std::string& input) {
    DecryptRequest request;
    request.set_key_label(label);
    request.set_username(username);
    request.set_encrypted_data(input);
    attestation_->Decrypt(
        request,
        base::Bind(&ClientLoop::PrintReplyAndQuit<DecryptReply>,
                   weak_factory_.GetWeakPtr()));
  }

  void CallSign(const std::string& label,
                const std::string& username,
                const std::string& input) {
    SignRequest request;
    request.set_key_label(label);
    request.set_username(username);
    request.set_data_to_sign(input);
    attestation_->Sign(request, base::Bind(&ClientLoop::OnSignComplete,
                                           weak_factory_.GetWeakPtr()));
  }

  void OnSignComplete(const SignReply& reply) {
    if (reply.status() == STATUS_SUCCESS &&
        base::CommandLine::ForCurrentProcess()->HasSwitch("output")) {
      WriteOutput(reply.signature());
    }
    PrintReplyAndQuit<SignReply>(reply);
  }

  void VerifySignature(const std::string& label,
                       const std::string& username,
                       const std::string& input,
                       const std::string& signature) {
    GetKeyInfoRequest request;
    request.set_key_label(label);
    request.set_username(username);
    attestation_->GetKeyInfo(request, base::Bind(&ClientLoop::VerifySignature2,
                                                 weak_factory_.GetWeakPtr(),
                                                 input, signature));
  }

  void VerifySignature2(const std::string& input,
                        const std::string& signature,
                        const GetKeyInfoReply& key_info) {
    CryptoUtilityImpl crypto(nullptr);
    if (crypto.VerifySignature(key_info.public_key(), input, signature)) {
      printf("Signature is OK!\n");
    } else {
      printf("Signature is BAD!\n");
    }
    Quit();
  }

  void CallRegister(const std::string& label, const std::string& username) {
    RegisterKeyWithChapsTokenRequest request;
    request.set_key_label(label);
    request.set_username(username);
    attestation_->RegisterKeyWithChapsToken(request, base::Bind(
        &ClientLoop::PrintReplyAndQuit<RegisterKeyWithChapsTokenReply>,
        weak_factory_.GetWeakPtr()));
  }

  std::unique_ptr<attestation::AttestationInterface> attestation_;

  // Declare this last so weak pointers will be destroyed first.
  base::WeakPtrFactory<ClientLoop> weak_factory_{this};

  DISALLOW_COPY_AND_ASSIGN(ClientLoop);
};

}  // namespace attestation

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  brillo::InitLog(brillo::kLogToStderr);
  attestation::ClientLoop loop;
  return loop.Run();
}
