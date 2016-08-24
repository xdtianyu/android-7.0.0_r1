# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus, gobject, sys

import common
from autotest_lib.client.common_lib import error
from autotest_lib.client.common_lib.cros import session_manager
from autotest_lib.client.cros import ownership


"""Utility class for tests that generate, push and fetch policies.

As the python bindings for the protobufs used in policies are built as a part
of tests that use them, callers must pass in their location at call time."""


def compare_policy_response(proto_binding_location, policy_response,
                            owner=None, guests=None, new_users=None,
                            roaming=None, whitelist=None, proxies=None):
    """Check the contents of |policy_response| against given args.

    Deserializes |policy_response| into a PolicyFetchResponse protobuf,
    with an embedded (serialized) PolicyData protobuf that embeds a
    (serialized) ChromeDeviceSettingsProto, and checks to see if this
    protobuf turducken contains the information passed in.

    @param proto_binding_location: the location of generated python bindings
                                   for policy protobufs.
    @param policy_response: string serialization of a PolicyData protobuf.
    @param owner: string representing the owner's name/account.
    @param guests: boolean indicating whether guests should be allowed.
    @param new_users: boolean indicating if user pods are on login screen.
    @param roaming: boolean indicating whether data roaming is enabled.
    @param whitelist: list of accounts that are allowed to log in.
    @param proxies: dictionary - { 'proxy_mode': <string> }

    @return True if |policy_response| has all the provided data, else False.
    """
    # Pull in protobuf bindings.
    sys.path.append(proto_binding_location)
    from device_management_backend_pb2 import PolicyFetchResponse
    from device_management_backend_pb2 import PolicyData
    from chrome_device_policy_pb2 import ChromeDeviceSettingsProto
    from chrome_device_policy_pb2 import AllowNewUsersProto
    from chrome_device_policy_pb2 import GuestModeEnabledProto
    from chrome_device_policy_pb2 import ShowUserNamesOnSigninProto
    from chrome_device_policy_pb2 import DataRoamingEnabledProto
    from chrome_device_policy_pb2 import DeviceProxySettingsProto

    response_proto = PolicyFetchResponse()
    response_proto.ParseFromString(policy_response)
    ownership.assert_has_policy_data(response_proto)

    data_proto = PolicyData()
    data_proto.ParseFromString(response_proto.policy_data)
    ownership.assert_has_device_settings(data_proto)
    if owner: ownership.assert_username(data_proto, owner)

    settings = ChromeDeviceSettingsProto()
    settings.ParseFromString(data_proto.policy_value)
    if guests: ownership.assert_guest_setting(settings, guests)
    if new_users: ownership.assert_show_users(settings, new_users)
    if roaming: ownership.assert_roaming(settings, roaming)
    if whitelist:
        ownership.assert_new_users(settings, False)
        ownership.assert_users_on_whitelist(settings, whitelist)
        if proxies: ownership.assert_proxy_settings(settings, proxies)


def build_policy_data(proto_binding_location, owner=None, guests=None,
                      new_users=None, roaming=None, whitelist=None,
                      proxies=None):
    """Generate and serialize a populated device policy protobuffer.

    Creates a PolicyData protobuf, with an embedded
    ChromeDeviceSettingsProto, containing the information passed in.

    @param proto_binding_location: the location of generated python bindings
                                   for policy protobufs.
    @param owner: string representing the owner's name/account.
    @param guests: boolean indicating whether guests should be allowed.
    @param new_users: boolean indicating if user pods are on login screen.
    @param roaming: boolean indicating whether data roaming is enabled.
    @param whitelist: list of accounts that are allowed to log in.
    @param proxies: dictionary - { 'proxy_mode': <string> }

    @return serialization of the PolicyData proto that we build.
    """
    # Pull in protobuf bindings.
    sys.path.append(proto_binding_location)
    from device_management_backend_pb2 import PolicyData
    from chrome_device_policy_pb2 import ChromeDeviceSettingsProto
    from chrome_device_policy_pb2 import AllowNewUsersProto
    from chrome_device_policy_pb2 import GuestModeEnabledProto
    from chrome_device_policy_pb2 import ShowUserNamesOnSigninProto
    from chrome_device_policy_pb2 import DataRoamingEnabledProto
    from chrome_device_policy_pb2 import DeviceProxySettingsProto

    data_proto = PolicyData()
    data_proto.policy_type = ownership.POLICY_TYPE
    if owner: data_proto.username = owner

    settings = ChromeDeviceSettingsProto()
    if guests:
        settings.guest_mode_enabled.guest_mode_enabled = guests
    if new_users:
        settings.show_user_names.show_user_names = new_users
    if roaming:
        settings.data_roaming_enabled.data_roaming_enabled = roaming
    if whitelist:
        settings.allow_new_users.allow_new_users = False
        for user in whitelist:
            settings.user_whitelist.user_whitelist.append(user)
    if proxies:
        settings.device_proxy_settings.proxy_mode = proxies['proxy_mode']

    data_proto.policy_value = settings.SerializeToString()
    return data_proto.SerializeToString()


def generate_policy(proto_binding_location, key, pubkey, policy, old_key=None):
    """Generate and serialize a populated, signed device policy protobuffer.

    Creates a protobuf containing the device policy |policy|, signed with
    |key|.  Also includes the public key |pubkey|, signed with |old_key|
    if provided.  If not, |pubkey| is signed with |key|.  The protobuf
    is serialized to a string and returned.

    @param proto_binding_location: the location of generated python bindings
                                   for policy protobufs.
    @param key: new policy signing key.
    @param pubkey: new public key to be signed and embedded in generated
                   PolicyFetchResponse.
    @param policy: policy data to be embedded in generated PolicyFetchResponse.
    @param old_key: if provided, this implies the generated PolicyFetchRespone
                    is intended to represent a key rotation.  pubkey will be
                    signed with this key before embedding.

    @return serialization of the PolicyFetchResponse proto that we build.
    """
    # Pull in protobuf bindings.
    sys.path.append(proto_binding_location)
    from device_management_backend_pb2 import PolicyFetchResponse

    if old_key == None:
        old_key = key
    policy_proto = PolicyFetchResponse()
    policy_proto.policy_data = policy
    policy_proto.policy_data_signature = ownership.sign(key, policy)
    policy_proto.new_public_key = pubkey
    policy_proto.new_public_key_signature = ownership.sign(old_key, pubkey)
    return policy_proto.SerializeToString()


def push_policy_and_verify(policy_string, sm):
    """Push a device policy to the session manager over DBus.

    The serialized device policy |policy_string| is sent to the session
    manager with the StorePolicy DBus call.  Success of the store is
    validated by fetching the policy again and comparing.

    @param policy_string: serialized policy to push to the session manager.
    @param sm: a connected SessionManagerInterface.

    @raises error.TestFail if policy push failed.
    """
    listener = session_manager.OwnershipSignalListener(gobject.MainLoop())
    listener.listen_for_new_policy()
    sm.StorePolicy(dbus.ByteArray(policy_string), byte_arrays=True)
    listener.wait_for_signals(desc='Policy push.')

    retrieved_policy = sm.RetrievePolicy(byte_arrays=True)
    if retrieved_policy != policy_string:
        raise error.TestFail('Policy should not be %s' % retrieved_policy)


def get_policy(sm):
    """Get a device policy from the session manager over DBus.

    Provided mainly for symmetry with push_policy_and_verify().

    @param sm: a connected SessionManagerInterface.

    @return Serialized PolicyFetchResponse.
    """
    return sm.RetrievePolicy(byte_arrays=True)
