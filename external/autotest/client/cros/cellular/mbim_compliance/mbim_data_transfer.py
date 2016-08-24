# Copyright 2015 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
MBIM Data transfer module is responsible for generating valid MBIM NTB frames
from  IP packets and for extracting IP packets from received MBIM NTB frames.

"""
import array
import struct
from collections import namedtuple

from autotest_lib.client.cros.cellular.mbim_compliance import mbim_constants
from autotest_lib.client.cros.cellular.mbim_compliance \
        import mbim_data_channel
from autotest_lib.client.cros.cellular.mbim_compliance import mbim_errors


NTH_SIGNATURE_32 = 0x686D636E  # "ncmh"
NDP_SIGNATURE_IPS_32 = 0x00737069  # "ips0"
NDP_SIGNATURE_DSS_32 = 0x00737364  # "dss0"

NTH_SIGNATURE_16 = 0x484D434E  # "NCMH"
NDP_SIGNATURE_IPS_16 = 0x00535049  # "IPS0"
NDP_SIGNATURE_DSS_16 = 0x00535344  # "DSS0"

class MBIMDataTransfer(object):
    """
    MBIMDataTransfer class is the public interface for any data transfer
    from/to the device via the MBIM data endpoints (BULK-IN/BULK-OUT).

    The class encapsulates the MBIM NTB frame generation/parsing as well as
    sending the the NTB frames to the device and vice versa.
    Users are expected to:
    1. Initialize the channel data transfer module by providing a valid
    device context which holds all the required info regarding the devie under
    test.
    2. Use send_data_packets to send IP packets to the device.
    3. Use receive_data_packets to receive IP packets from the device.

    """
    def __init__(self, device_context):
        """
        Initialize the Data Transfer object. The data transfer object
        instantiates the data channel to prepare for any data transfer from/to
        the device using the bulk pipes.

        @params device_context: The device context which contains all the USB
                descriptors, NTB params and USB handle to the device.

        """
        self._device_context = device_context
        mbim_data_interface = (
                device_context.descriptor_cache.mbim_data_interface)
        bulk_in_endpoint = (
                device_context.descriptor_cache.bulk_in_endpoint)
        bulk_out_endpoint = (
                device_context.descriptor_cache.bulk_out_endpoint)
        self._data_channel = mbim_data_channel.MBIMDataChannel(
                device=device_context.device,
                data_interface_number=mbim_data_interface.bInterfaceNumber,
                bulk_in_endpoint_address=bulk_in_endpoint.bEndpointAddress,
                bulk_out_endpoint_address=bulk_out_endpoint.bEndpointAddress,
                max_in_buffer_size=device_context.max_in_data_transfer_size)


    def send_data_packets(self, ntb_format, data_packets):
        """
        Creates an MBIM frame for the payload provided and sends it out to the
        device using bulk out pipe.

        @param ntb_format: Whether to send an NTB16 or NTB32 frame.
        @param data_packets: Array of data packets. Each packet is a byte array
                corresponding to the IP packet or any other payload to be sent.

        """
        ntb_object = MBIMNtb(ntb_format)
        ntb_frame = ntb_object.generate_ntb(
                data_packets,
                self._device_context.max_out_data_transfer_size,
                self._device_context.out_data_transfer_divisor,
                self._device_context.out_data_transfer_payload_remainder,
                self._device_context.out_data_transfer_ndp_alignment)
        self._data_channel.send_ntb(ntb_frame)


    def receive_data_packets(self, ntb_format):
        """
        Receives an MBIM frame from the device using the bulk in pipe,
        deaggregates the payload from the frame and returns it to the caller.

        Will return an empty tuple, if no frame is received from the device.

        @param ntb_format: Whether to receive an NTB16 or NTB32 frame.
        @returns tuple of (nth, ndp, ndp_entries, payload) where,
                nth - NTH header object received.
                ndp - NDP header object received.
                ndp_entries - Array of NDP entry header objects.
                payload - Array of packets where each packet is a byte array.

        """
        ntb_frame = self._data_channel.receive_ntb()
        if not ntb_frame:
            return ()
        ntb_object = MBIMNtb(ntb_format)
        return ntb_object.parse_ntb(ntb_frame)


class MBIMNtb(object):
    """
    MBIM NTB class used for MBIM data transfer.

    This class is used to generate/parse NTB frames.

    Limitations:
    1. We currently only support a single NDP frame within an NTB.
    2. We only support IP data payload. This can be overcome by using the DSS
            (instead of IPS) prefix in NDP signature if required.

    """
    _NEXT_SEQUENCE_NUMBER = 0

    def __init__(self, ntb_format):
        """
        Initialization of the NTB object.

        We assign the appropriate header classes required based on whether
        we are going to work with NTB16 or NTB32 data frames.

        @param ntb_format: Type of NTB: 16 vs 32

        """
        self._ntb_format = ntb_format
        # Defining the tuples to be used for the headers.
        if ntb_format == mbim_constants.NTB_FORMAT_16:
            self._nth_class = Nth16
            self._ndp_class = Ndp16
            self._ndp_entry_class = NdpEntry16
            self._nth_signature = NTH_SIGNATURE_16
            self._ndp_signature = NDP_SIGNATURE_IPS_16
        else:
            self._nth_class = Nth32
            self._ndp_class = Ndp32
            self._ndp_entry_class = NdpEntry32
            self._nth_signature = NTH_SIGNATURE_32
            self._ndp_signature = NDP_SIGNATURE_IPS_32


    @classmethod
    def get_next_sequence_number(cls):
        """
        Returns incrementing sequence numbers on successive calls. We start
        the sequence numbering at 0.

        @returns The sequence number for data transfers.

        """
        # Make sure to rollover the 16 bit sequence number.
        if MBIMNtb._NEXT_SEQUENCE_NUMBER > (0xFFFF - 2):
            MBIMNtb._NEXT_SEQUENCE_NUMBER = 0x0000
        sequence_number = MBIMNtb._NEXT_SEQUENCE_NUMBER
        MBIMNtb._NEXT_SEQUENCE_NUMBER += 1
        return sequence_number


    @classmethod
    def reset_sequence_number(cls):
        """
        Resets the sequence number to be used for NTB's sent from host. This
        has to be done every time the device is reset.

        """
        cls._NEXT_SEQUENCE_NUMBER = 0x00000000


    def get_next_payload_offset(self,
                                current_offset,
                                ntb_divisor,
                                ntb_payload_remainder):
        """
        Helper function to find the offset to place the next payload

        Alignment of payloads follow this formula:
            Offset % ntb_divisor == ntb_payload_remainder.

        @params current_offset: Current index offset in the frame.
        @param ntb_divisor: Used for payload alignment within the frame.
        @param ntb_payload_remainder: Used for payload alignment within the
                frame.
        @returns offset to place the next payload at.

        """
        next_payload_offset = (
                (((current_offset + (ntb_divisor - 1)) / ntb_divisor) *
                 ntb_divisor) + ntb_payload_remainder)
        return next_payload_offset


    def generate_ntb(self,
                     payload,
                     max_ntb_size,
                     ntb_divisor,
                     ntb_payload_remainder,
                     ntb_ndp_alignment):
        """
        This function generates an NTB frame out of the payload provided.

        @param payload: Array of packets to sent to the device. Each packet
                contains the raw byte array of IP packet to be sent.
        @param max_ntb_size: Max size of NTB frame supported by the device.
        @param ntb_divisor: Used for payload alignment within the frame.
        @param ntb_payload_remainder: Used for payload alignment within the
                frame.
        @param ntb_ndp_alignment : Used for NDP header alignment within the
                frame.
        @raises MBIMComplianceNtbError if the complete |ntb| can not fit into
                |max_ntb_size|.
        @returns the raw MBIM NTB byte array.

        """
        cls = self.__class__

        # We start with the NTH header, then the payload and then finally
        # the NDP header and the associated NDP entries.
        ntb_curr_offset = self._nth_class.get_struct_len()
        num_packets = len(payload)
        nth_length = self._nth_class.get_struct_len()
        ndp_length = self._ndp_class.get_struct_len()
        # We need one extra ZLP NDP entry at the end, so account for it.
        ndp_entries_length = (
                self._ndp_entry_class.get_struct_len() * (num_packets + 1))

        # Create the NDP header and an NDP_ENTRY header for each packet.
        # We can create the NTH header only after we calculate the total length.
        self.ndp = self._ndp_class(
                signature=self._ndp_signature,
                length=ndp_length+ndp_entries_length,
                next_ndp_index=0)
        self.ndp_entries = []

        # We'll also construct the payload raw data as we loop thru the packets.
        # The padding in between the payload is added in place.
        raw_ntb_frame_payload = array.array('B', [])
        for packet in payload:
            offset = self.get_next_payload_offset(
                    ntb_curr_offset, ntb_divisor, ntb_payload_remainder)
            align_length = offset - ntb_curr_offset
            length = len(packet)
            # Add align zeroes, then payload, then pad zeroes
            raw_ntb_frame_payload += array.array('B', [0] * align_length)
            raw_ntb_frame_payload += packet
            self.ndp_entries.append(self._ndp_entry_class(
                    datagram_index=offset, datagram_length=length))
            ntb_curr_offset = offset + length

        # Add the ZLP entry
        self.ndp_entries.append(self._ndp_entry_class(
                datagram_index=0, datagram_length=0))

        # Store the NDP offset to be used in creating NTH header.
        # NDP alignment is specified by the device with a minimum of 4 and it
        # always a multiple of 2.
        ndp_align_mask = ntb_ndp_alignment - 1
        if ntb_curr_offset & ndp_align_mask:
            pad_length = ntb_ndp_alignment - (ntb_curr_offset & ndp_align_mask)
            raw_ntb_frame_payload += array.array('B', [0] * pad_length)
            ntb_curr_offset += pad_length
        ndp_offset = ntb_curr_offset
        ntb_curr_offset += ndp_length
        ntb_curr_offset += ndp_entries_length
        if ntb_curr_offset > max_ntb_size:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceNtbError,
                    'Could not fit the complete NTB of size %d into %d bytes' %
                    ntb_curr_offset, max_ntb_size)
        # Now create the NTH header
        self.nth = self._nth_class(
                signature=self._nth_signature,
                header_length=nth_length,
                sequence_number=cls.get_next_sequence_number(),
                block_length=ntb_curr_offset,
                fp_index=ndp_offset)

        # Create the raw bytes now, we create the raw bytes of the header and
        # attach it to the payload raw bytes with padding already created above.
        raw_ntb_frame = array.array('B', [])
        raw_ntb_frame += array.array('B', self.nth.pack())
        raw_ntb_frame += raw_ntb_frame_payload
        raw_ntb_frame += array.array('B', self.ndp.pack())
        for entry in self.ndp_entries:
            raw_ntb_frame += array.array('B', entry.pack())

        self.payload = payload
        self.raw_ntb_frame = raw_ntb_frame

        return raw_ntb_frame


    def parse_ntb(self, raw_ntb_frame):
        """
        This function parses an NTB frame and returns the NTH header, NDP header
        and the payload parsed which can be used to inspect the response
        from the device.

        @param raw_ntb_frame: Array of bytes of an MBIM NTB frame.
        @raises MBIMComplianceNtbError if there is an error in parsing.
        @returns tuple of (nth, ndp, ndp_entries, payload) where,
                nth - NTH header object received.
                ndp - NDP header object received.
                ndp_entries - Array of NDP entry header objects.
                payload - Array of packets where each packet is a byte array.

        """
        # Read the nth header to find the ndp header index
        self.nth = self._nth_class(raw_data=raw_ntb_frame)
        ndp_offset = self.nth.fp_index
        # Verify the total length field
        if len(raw_ntb_frame) != self.nth.block_length:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceNtbError,
                    'NTB size mismatch Total length: %x Reported: %x bytes' % (
                            len(raw_ntb_frame), self.nth.block_length))

        # Read the NDP header to find the number of packets in the entry
        self.ndp = self._ndp_class(raw_data=raw_ntb_frame[ndp_offset:])
        num_ndp_entries = (
               (self.ndp.length - self._ndp_class.get_struct_len()) /
               self._ndp_entry_class.get_struct_len())
        ndp_entries_offset = ndp_offset + self._ndp_class.get_struct_len()
        self.payload = []
        self.ndp_entries = []
        for _ in range(0, num_ndp_entries):
            ndp_entry = self._ndp_entry_class(
                   raw_data=raw_ntb_frame[ndp_entries_offset:])
            ndp_entries_offset += self._ndp_entry_class.get_struct_len()
            packet_start_offset = ndp_entry.datagram_index
            packet_end_offset = (
                   ndp_entry.datagram_index + ndp_entry.datagram_length)
            # There is one extra ZLP NDP entry at the end, so account for it.
            if ndp_entry.datagram_index and ndp_entry.datagram_length:
                packet = array.array('B', raw_ntb_frame[packet_start_offset:
                                                        packet_end_offset])
                self.payload.append(packet)
            self.ndp_entries.append(ndp_entry)

        self.raw_ntb_frame = raw_ntb_frame

        return (self.nth, self.ndp, self.ndp_entries, self.payload)


def header_class_new(cls, **kwargs):
    """
    Creates a header instance with either the given field name/value
    pairs or raw data buffer.

    @param kwargs: Dictionary of (field_name, field_value) pairs or
            raw_data=Packed binary array.
    @returns New header object created.

    """
    field_values = []
    if 'raw_data' in kwargs and kwargs['raw_data']:
        raw_data = kwargs['raw_data']
        data_format = cls.get_field_format_string()
        unpack_length = cls.get_struct_len()
        data_length = len(raw_data)
        if data_length < unpack_length:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceDataTransferError,
                    'Length of Data (%d) to be parsed less than header'
                    ' structure length (%d)' %
                    (data_length, unpack_length))
        field_values = struct.unpack_from(data_format, raw_data)
    else:
        field_names = cls.get_field_names()
        for field_name in field_names:
            if field_name not in kwargs:
                field_value = 0
                field_values.append(field_value)
            else:
                field_values.append(kwargs.pop(field_name))
        if kwargs:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceDataTransferError,
                    'Unexpected fields (%s) in %s' % (
                            kwargs.keys(), cls.__name__))
    obj = super(cls, cls).__new__(cls, *field_values)
    return obj


class MBIMNtbHeadersMeta(type):
    """
    Metaclass for all the NTB headers. This is relatively toned down metaclass
    to create namedtuples out of the header fields.

    Header definition attributes:
    _FIELDS: Used to define structure elements. Each element contains a format
            specifier and the field name.

    """
    def __new__(mcs, name, bases, attrs):
        if object in bases:
            return super(MBIMNtbHeadersMeta, mcs).__new__(
                    mcs, name, bases, attrs)
        fields = attrs['_FIELDS']
        if not fields:
            mbim_errors.log_and_raise(
                    mbim_errors.MBIMComplianceDataTransfer,
                    '%s header must have some fields defined' % name)
        _, field_names = zip(*fields)
        attrs['__new__'] = header_class_new
        header_class = namedtuple(name, field_names)
        # Prepend the class created via namedtuple to |bases| in order to
        # correctly resolve the __new__ method while preserving the class
        # hierarchy.
        cls = super(MBIMNtbHeadersMeta, mcs).__new__(
                mcs, name, (header_class,) + bases, attrs)
        return cls


class MBIMNtbHeaders(object):
    """
    Base class for all NTB headers.

    This class should not be instantiated on it's own.

    The base class overrides namedtuple's __new__ to:
    1. Create a tuple out of raw object.
    2. Put value of zero for fields which are not specified by the caller,
        For ex: reserved fields

    """
    __metaclass__ = MBIMNtbHeadersMeta

    @classmethod
    def get_fields(cls):
        """
        Helper function to find all the fields of this class.

        @returns Fields of the structure.

        """
        return cls._FIELDS


    @classmethod
    def get_field_names(cls):
        """
        Helper function to return the field names of the header.

        @returns The field names of the header structure.

        """
        _, field_names = zip(*cls.get_fields())
        return field_names


    @classmethod
    def get_field_formats(cls):
        """
        Helper function to return the field formats of the header.

        @returns The format of fields of the header structure.

        """
        field_formats, _ = zip(*cls.get_fields())
        return field_formats


    @classmethod
    def get_field_format_string(cls):
        """
        Helper function to return the field format string of the header.

        @returns The format string of the header structure.

        """
        format_string = '<' + ''.join(cls.get_field_formats())
        return format_string


    @classmethod
    def get_struct_len(cls):
        """
        Returns the length of the structure representing the header.

        @returns Length of the structure.

        """
        return struct.calcsize(cls.get_field_format_string())


    def pack(self):
        """
        Packs a header based on the field format specified.

        @returns The packet in binary array form.

        """
        cls = self.__class__
        field_names = cls.get_field_names()
        format_string = cls.get_field_format_string()
        field_values = [getattr(self, name) for name in field_names]
        return array.array('B', struct.pack(format_string, *field_values))


class Nth16(MBIMNtbHeaders):
    """ The class for MBIM NTH16 objects. """
    _FIELDS = (('I', 'signature'),
               ('H', 'header_length'),
               ('H', 'sequence_number'),
               ('H', 'block_length'),
               ('H', 'fp_index'))


class Ndp16(MBIMNtbHeaders):
    """ The class for MBIM NDP16 objects. """
    _FIELDS = (('I', 'signature'),
               ('H', 'length'),
               ('H', 'next_ndp_index'))


class NdpEntry16(MBIMNtbHeaders):
    """ The class for MBIM NDP16 objects. """
    _FIELDS = (('H', 'datagram_index'),
               ('H', 'datagram_length'))


class Nth32(MBIMNtbHeaders):
    """ The class for MBIM NTH32 objects. """
    _FIELDS = (('I', 'signature'),
               ('H', 'header_length'),
               ('H', 'sequence_number'),
               ('I', 'block_length'),
               ('I', 'fp_index'))


class Ndp32(MBIMNtbHeaders):
    """ The class for MBIM NTH32 objects. """
    _FIELDS = (('I', 'signature'),
               ('H', 'length'),
               ('H', 'reserved_6'),
               ('I', 'next_ndp_index'),
               ('I', 'reserved_12'))


class NdpEntry32(MBIMNtbHeaders):
    """ The class for MBIM NTH32 objects. """
    _FIELDS = (('I', 'datagram_index'),
               ('I', 'datagram_length'))

