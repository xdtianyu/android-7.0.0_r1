# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
All of the MBIM messages are created using the MBIMControlMessageMeta metaclass.
The metaclass supports a hierarchy of message definitions so that each message
definition extends the structure of the base class it inherits.

(mbim_message.py)
MBIMControlMessage|         (mbim_message_request.py)
                  |>MBIMControlMessageRequest |
                  |                           |>MBIMOpen
                  |                           |>MBIMClose
                  |                           |>MBIMCommand    |
                  |                           |                |>MBIMSetConnect
                  |                           |                |>...
                  |                           |
                  |                           |>MBIMHostError
                  |
                  |         (mbim_message_response.py)
                  |>MBIMControlMessageResponse|
                                              |>MBIMOpenDone
                                              |>MBIMCloseDone
                                              |>MBIMCommandDone|
                                              |                |>MBIMConnectInfo
                                              |                |>...
                                              |
                                              |>MBIMHostError
"""
import array
import logging
import struct
import sys
from collections import namedtuple

from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors


# Type of message classes. The values of each field in the message is stored
# as an attribute of the object created.
# Request message classes accepts values for the attributes of the object.
MESSAGE_TYPE_REQUEST = 1
# Response message classes accepts raw_data which is parsed into attributes of
# the object.
MESSAGE_TYPE_RESPONSE = 2

# Message field types.
# Just a normal field type. No special properties.
FIELD_TYPE_NORMAL = 1
# Identify the payload ID for a message. This is used in  parsing of
# response messages to help in identifying the child message class.
FIELD_TYPE_PAYLOAD_ID = 2
# Total length of the message including any payload_buffer it may contain.
FIELD_TYPE_TOTAL_LEN = 3
# Length of the payload contained in the payload_buffer.
FIELD_TYPE_PAYLOAD_LEN = 4
# Number of fragments of this message.
FIELD_TYPE_NUM_FRAGMENTS = 5
# Transaction ID of this message
FIELD_TYPE_TRANSACTION_ID = 6


def message_class_new(cls, **kwargs):
    """
    Creates a message instance with either the given field name/value
    pairs or raw data buffer.

    The total_length and transaction_id fields are automatically calculated
    if not explicitly provided in the message args.

    @param kwargs: Dictionary of (field_name, field_value) pairs or
                    raw_data=Packed binary array.
    @returns New message object created.

    """
    if 'raw_data' in kwargs and kwargs['raw_data']:
        # We unpack the raw data received into the appropriate fields
        # for this class. If there is some additional data present in
        # |raw_data| that does not fit the format of the structure,
        # they're stored in the variable sized |payload_buffer| field.
        raw_data = kwargs['raw_data']
        data_format = cls.get_field_format_string(get_all=True)
        unpack_length = cls.get_struct_len(get_all=True)
        data_length = len(raw_data)
        if data_length < unpack_length:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceControlMessageError,
                    'Length of Data (%d) to be parsed less than message'
                    ' structure length (%d)' %
                    (data_length, unpack_length))
        obj = super(cls, cls).__new__(cls, *struct.unpack_from(data_format,
                                                               raw_data))
        if data_length > unpack_length:
            setattr(obj, 'payload_buffer', raw_data[unpack_length:])
        else:
            setattr(obj, 'payload_buffer', None)
        return obj
    else:
        # Check if all the fields have been populated for this message
        # except for transaction ID and message length since these
        # are generated during init.
        field_values = []
        fields = cls.get_fields(get_all=True)
        defaults = cls.get_defaults(get_all=True)
        for _, field_name, field_type in fields:
            if field_name not in kwargs:
                if field_type == FIELD_TYPE_TOTAL_LEN:
                    field_value = cls.get_struct_len(get_all=True)
                    if 'payload_buffer' in kwargs:
                        field_value += len(kwargs.get('payload_buffer'))
                elif field_type == FIELD_TYPE_TRANSACTION_ID:
                    field_value = cls.get_next_transaction_id()
                else:
                    field_value = defaults.get(field_name, None)
                if field_value is None:
                    mbim_errors.log_and_raise(
                            mbim_errors.MBIMComplianceControlMessageError,
                            'Missing field value (%s) in %s' % (
                                    field_name, cls.__name__))
                field_values.append(field_value)
            else:
                field_values.append(kwargs.pop(field_name))
        obj = super(cls, cls).__new__(cls, *field_values)
        # We need to account for optional variable sized payload_buffer
        # in some messages which are not explicitly mentioned in the
        # |cls._FIELDS| attribute.
        if 'payload_buffer' in kwargs:
            setattr(obj, 'payload_buffer', kwargs.pop('payload_buffer'))
        else:
            setattr(obj, 'payload_buffer', None)
        if kwargs:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceControlMessageError,
                    'Unexpected fields (%s) in %s' % (
                            kwargs.keys(), cls.__name__))
        return obj


class MBIMControlMessageMeta(type):
    """
    Metaclass for all the control message parsing/generation.

    The metaclass creates each class by concatenating all the message fields
    from it's base classes to create a hierarchy of messages.
    Thus the payload class of each message class becomes the subclass of that
    message.

    Message definition attributes->
    _FIELDS(optional): Used to define structure elements. The fields of a
                       message is the concatenation of the _FIELDS attribute
                       along with all the _FIELDS attribute from it's parent
                       classes.
    _DEFAULTS(optional): Field name/value pairs to be assigned to some
                         of the fields if they are fixed for a message type.
                         These are generally used to assign values to fields in
                         the parent class.
    _IDENTIFIERS(optional): Field name/value pairs to be used to idenitfy this
                            message during parsing from raw_data.
    _SECONDARY_FRAGMENTS(optional): Used to identify if this class can be
                                    fragmented and name of secondary class
                                    definition.
    MESSAGE_TYPE: Used to identify request/repsonse classes.

    Message internal attributes->
    _CONSOLIDATED_FIELDS: Consolidated list of all the fields defining this
                          message.
    _CONSOLIDATED_DEFAULTS: Consolidated list of all the default field
                            name/value pairs for this  message.

    """
    def __new__(mcs, name, bases, attrs):
        # The MBIMControlMessage base class, which inherits from 'object',
        # is merely used to establish the class hierarchy and is never
        # constructed on it's own.
        if object in bases:
            return super(MBIMControlMessageMeta, mcs).__new__(
                    mcs, name, bases, attrs)

        # Append the current class fields, defaults to any base parent class
        # fields.
        fields = []
        defaults = {}
        for base_class in bases:
            if hasattr(base_class, '_CONSOLIDATED_FIELDS'):
                fields = getattr(base_class, '_CONSOLIDATED_FIELDS')
            if hasattr(base_class, '_CONSOLIDATED_DEFAULTS'):
                defaults = getattr(base_class, '_CONSOLIDATED_DEFAULTS').copy()
        if '_FIELDS' in attrs:
            fields = fields + map(list, attrs['_FIELDS'])
        if '_DEFAULTS' in attrs:
            defaults.update(attrs['_DEFAULTS'])
        attrs['_CONSOLIDATED_FIELDS'] = fields
        attrs['_CONSOLIDATED_DEFAULTS'] = defaults

        if not fields:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceControlMessageError,
                    '%s message must have some fields defined' % name)

        attrs['__new__'] = message_class_new
        _, field_names, _ = zip(*fields)
        message_class = namedtuple(name, field_names)
        # Prepend the class created via namedtuple to |bases| in order to
        # correctly resolve the __new__ method while preserving the class
        # hierarchy.
        cls = super(MBIMControlMessageMeta, mcs).__new__(
                mcs, name, (message_class,) + bases, attrs)
        return cls


class MBIMControlMessage(object):
    """
    MBIMControlMessage base class.

    This class should not be instantiated or used directly.

    """
    __metaclass__ = MBIMControlMessageMeta

    _NEXT_TRANSACTION_ID = 0X00000000


    @classmethod
    def _find_subclasses(cls):
        """
        Helper function to find all the derived payload classes of this
        class.

        """
        return [c for c in cls.__subclasses__()]


    @classmethod
    def get_fields(cls, get_all=False):
        """
        Helper function to find all the fields of this class.

        Returns either the total message fields or only the current
        substructure fields in the nested message.

        @param get_all: Whether to return the total struct fields or sub struct
                         fields.
        @returns Fields of the structure.

        """
        if get_all:
            return cls._CONSOLIDATED_FIELDS
        else:
            return cls._FIELDS


    @classmethod
    def get_defaults(cls, get_all=False):
        """
        Helper function to find all the default field values of this class.

        Returns either the total message default field name/value pairs or only
        the current substructure defaults in the nested message.

        @param get_all: Whether to return the total struct defaults or sub
                         struct defaults.
        @returns Defaults of the structure.

        """
        if get_all:
            return cls._CONSOLIDATED_DEFAULTS
        else:
            return cls._DEFAULTS


    @classmethod
    def _get_identifiers(cls):
        """
        Helper function to find all the identifier field name/value pairs of
        this class.

        @returns All the idenitifiers of this class.

        """
        return getattr(cls, '_IDENTIFIERS', None)


    @classmethod
    def _find_field_names_of_type(cls, find_type, get_all=False):
        """
        Helper function to find all the field names which matches the field_type
        specified.

        params find_type: One of the FIELD_TYPE_* enum values specified above.
        @returns Corresponding field names if found, else None.
        """
        fields = cls.get_fields(get_all=get_all)
        field_names = []
        for _, field_name, field_type in fields:
            if field_type == find_type:
                field_names.append(field_name)
        return field_names


    @classmethod
    def get_secondary_fragment(cls):
        """
        Helper function to retrieve the associated secondary fragment class.

        @returns |_SECONDARY_FRAGMENT| attribute of the class

        """
        return getattr(cls, '_SECONDARY_FRAGMENT', None)


    @classmethod
    def get_field_names(cls, get_all=True):
        """
        Helper function to return the field names of the message.

        @returns The field names of the message structure.

        """
        _, field_names, _ = zip(*cls.get_fields(get_all=get_all))
        return field_names


    @classmethod
    def get_field_formats(cls, get_all=True):
        """
        Helper function to return the field formats of the message.

        @returns The format of fields of the message structure.

        """
        field_formats, _, _ = zip(*cls.get_fields(get_all=get_all))
        return field_formats


    @classmethod
    def get_field_format_string(cls, get_all=True):
        """
        Helper function to return the field format string of the message.

        @returns The format string of the message structure.

        """
        format_string = '<' + ''.join(cls.get_field_formats(get_all=get_all))
        return format_string


    @classmethod
    def get_struct_len(cls, get_all=False):
        """
        Returns the length of the structure representing the message.

        Returns the length of either the total message or only the current
        substructure in the nested message.

        @param get_all: Whether to return the total struct length or sub struct
                length.
        @returns Length of the structure.

        """
        return struct.calcsize(cls.get_field_format_string(get_all=get_all))


    @classmethod
    def find_primary_parent_fragment(cls):
        """
        Traverses up the message tree to find the primary fragment class
        at the same tree level as the secondary frag class associated with this
        message class. This should only be called on primary fragment derived
        classes!

        @returns Primary frag class associated with the message.

        """
        secondary_frag_cls = cls.get_secondary_fragment()
        secondary_frag_parent_cls = secondary_frag_cls.__bases__[1]
        message_cls = cls
        message_parent_cls = message_cls.__bases__[1]
        while message_parent_cls != secondary_frag_parent_cls:
            message_cls = message_parent_cls
            message_parent_cls = message_cls.__bases__[1]
        return message_cls


    @classmethod
    def get_next_transaction_id(cls):
        """
        Returns incrementing transaction ids on successive calls.

        @returns The tracsaction id for control message delivery.

        """
        if MBIMControlMessage._NEXT_TRANSACTION_ID > (sys.maxint - 2):
            MBIMControlMessage._NEXT_TRANSACTION_ID = 0x00000000
        MBIMControlMessage._NEXT_TRANSACTION_ID += 1
        return MBIMControlMessage._NEXT_TRANSACTION_ID


    def _get_fields_of_type(self, field_type, get_all=False):
        """
        Helper function to find all the field name/value of the specified type
        in the given object.

        @returns Corresponding map of field name/value pairs extracted from the
                object.

        """
        field_names = self.__class__._find_field_names_of_type(field_type,
                                                               get_all=get_all)
        return {f: getattr(self, f) for f in field_names}


    def _get_payload_id_fields(self):
        """
        Helper function to find all the payload id field name/value in the given
        object.

        @returns Corresponding field name/value pairs extracted from the object.

        """
        return self._get_fields_of_type(FIELD_TYPE_PAYLOAD_ID)


    def get_payload_len(self):
        """
        Helper function to find the payload len field value in the given
        object.

        @returns Corresponding field value extracted from the object.

        """
        payload_len_fields = self._get_fields_of_type(FIELD_TYPE_PAYLOAD_LEN)
        if ((not payload_len_fields) or (len(payload_len_fields) > 1)):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceControlMessageError,
                    "Erorr in finding payload len field in message: %s" %
                    self.__class__.__name__)
        return payload_len_fields.values()[0]


    def get_total_len(self):
        """
        Helper function to find the total len field value in the given
        object.

        @returns Corresponding field value extracted from the object.

        """
        total_len_fields = self._get_fields_of_type(FIELD_TYPE_TOTAL_LEN,
                                                    get_all=True)
        if ((not total_len_fields) or (len(total_len_fields) > 1)):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceControlMessageError,
                    "Erorr in finding total len field in message: %s" %
                    self.__class__.__name__)
        return total_len_fields.values()[0]


    def get_num_fragments(self):
        """
        Helper function to find the fragment num field value in the given
        object.

        @returns Corresponding field value extracted from the object.

        """
        num_fragment_fields = self._get_fields_of_type(FIELD_TYPE_NUM_FRAGMENTS)
        if ((not num_fragment_fields) or (len(num_fragment_fields) > 1)):
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceControlMessageError,
                    "Erorr in finding num fragments field in message: %s" %
                    self.__class__.__name__)
        return num_fragment_fields.values()[0]


    def find_payload_class(self):
        """
        Helper function to find the derived class which has the default
        |payload_id| fields matching the current message contents.

        @returns Corresponding class if found, else None.

        """
        cls = self.__class__
        for payload_cls in cls._find_subclasses():
            message_ids = self._get_payload_id_fields()
            subclass_ids = payload_cls._get_identifiers()
            if message_ids == subclass_ids:
                return payload_cls
        return None


    def calculate_total_len(self):
        """
        Helper function to calculate the total len of a given message
        object.

        @returns Total length of the message.

        """
        message_class = self.__class__
        total_len = message_class.get_struct_len(get_all=True)
        if self.payload_buffer:
            total_len += len(self.payload_buffer)
        return total_len


    def pack(self, format_string, field_names):
        """
        Packs a list of fields based on their formats.

        @param format_string: The concatenated formats for the fields given in
                |field_names|.
        @param field_names: The name of the fields to be packed.
        @returns The packet in binary array form.

        """
        field_values = [getattr(self, name) for name in field_names]
        return array.array('B', struct.pack(format_string, *field_values))


    def print_all_fields(self):
        """Prints all the field name, value pair of this message."""
        logging.debug('Class Name: %s', self.__class__.__name__)
        for field_name in self.__class__.get_field_names(get_all=True):
            logging.debug('Field Name: %s, Field Value: %s',
                           field_name, str(getattr(self, field_name)))
        if self.payload_buffer:
            logging.debug('Payload: %s', str(getattr(self, 'payload_buffer')))


    def create_raw_data(self):
        """
        Creates the raw binary data corresponding to the message struct.

        @param payload_buffer: Variable sized paylaod buffer to attach at the
                end of the msg.
        @returns Packed byte array of the message.

        """
        message = self
        message_class = message.__class__
        format_string = message_class.get_field_format_string()
        field_names = message_class.get_field_names()
        packet = message.pack(format_string, field_names)
        if self.payload_buffer:
            packet.extend(self.payload_buffer)
        return packet


    def copy(self, **fields_to_alter):
        """
        Replaces the message tuple with updated field values.

        @param fields_to_alter: Field name/value pairs to be changed.
        @returns Updated message with the field values updated.

        """
        message = self._replace(**fields_to_alter)
        # Copy the associated payload_buffer field to the new tuple.
        message.payload_buffer = self.payload_buffer
        return message
