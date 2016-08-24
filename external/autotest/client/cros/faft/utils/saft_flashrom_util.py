# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" This module provides convenience routines to access Flash ROM (EEPROM)

saft_flashrom_util is based on utility 'flashrom'.

Original tool syntax:
    (read ) flashrom -r <file>
    (write) flashrom -l <layout_fn> [-i <image_name> ...] -w <file>

The layout_fn is in format of
    address_begin:address_end image_name
    which defines a region between (address_begin, address_end) and can
    be accessed by the name image_name.

Currently the tool supports multiple partial write but not partial read.

In the saft_flashrom_util, we provide read and partial write abilities.
For more information, see help(saft_flashrom_util.flashrom_util).
"""

class TestError(Exception):
    pass


class LayoutScraper(object):
    """Object of this class is used to retrieve layout from a BIOS file."""

    # The default conversion table for mosys.
    DEFAULT_CHROMEOS_FMAP_CONVERSION = {
        "Boot Stub": "FV_BSTUB",
        "GBB Area": "FV_GBB",
        "Recovery Firmware": "FVDEV",
        "RO VPD": "RO_VPD",
        "Firmware A Key": "VBOOTA",
        "Firmware A Data": "FVMAIN",
        "Firmware B Key": "VBOOTB",
        "Firmware B Data": "FVMAINB",
        "Log Volume": "FV_LOG",
        # New layout in Chrome OS Main Processor Firmware Specification,
        # used by all newer (>2011) platforms except Mario.
        "BOOT_STUB": "FV_BSTUB",
        "GBB": "FV_GBB",
        "RECOVERY": "FVDEV",
        "VBLOCK_A": "VBOOTA",
        "VBLOCK_B": "VBOOTB",
        "FW_MAIN_A": "FVMAIN",
        "FW_MAIN_B": "FVMAINB",
        # New sections in Depthcharge.
        "EC_MAIN_A": "ECMAINA",
        "EC_MAIN_B": "ECMAINB",
        # EC firmware layout
        "EC_RW": "EC_RW",
        }

    def __init__(self, os_if):
        self.image = None
        self.os_if = os_if

    def _get_text_layout(self, file_name):
        """Retrieve text layout from a firmware image file.

        This function uses the 'mosys' utility to scan the firmware image and
        retrieve the section layout information.

        The layout is reported as a set of lines with multiple
        "<name>"="value" pairs, all this output is passed to the caller.
        """

        mosys_cmd = 'mosys -k eeprom map %s' % file_name
        return self.os_if.run_shell_command_get_output(mosys_cmd)

    def _line_to_dictionary(self, line):
        """Convert a text layout line into a dictionary.

        Get a string consisting of single space separated "<name>"="value>"
        pairs and convert it into a dictionary where keys are the <name>
        fields, and values are the corresponding <value> fields.

        Return the dictionary to the caller.
        """

        rv = {}

        items = line.replace('" ', '"^').split('^')
        for item in items:
            pieces = item.split('=')
            if len(pieces) != 2:
                continue
            rv[pieces[0]] = pieces[1].strip('"')
        return rv

    def check_layout(self, layout, file_size):
        """Verify the layout to be consistent.

        The layout is consistent if there is no overlapping sections and the
        section boundaries do not exceed the file size.

        Inputs:
          layout: a dictionary keyed by a string (the section name) with
                  values being two integers tuples, the first and the last
                  bites' offset in the file.
          file_size: and integer, the size of the file the layout describes
                     the sections in.

        Raises:
          TestError in case the layout is not consistent.
        """

        # Generate a list of section range tuples.
        ost = sorted([layout[section] for section in layout])
        base = -1
        for section_base, section_end in ost:
            if section_base <= base or section_end + 1 < section_base:
                raise TestError('bad section at 0x%x..0x%x' % (
                        section_base, section_end))
            base = section_end
        if base > file_size:
            raise TestError('Section end 0x%x exceeds file size %x' % (
                    base, file_size))

    def get_layout(self, file_name):
        """Generate layout for a firmware file.

        First retrieve the text layout as reported by 'mosys' and then convert
        it into a dictionary, replacing section names reported by mosys into
        matching names from DEFAULT_CHROMEOS_FMAP_CONVERSION dictionary above,
        using the names as keys in the layout dictionary. The elements of the
        layout dictionary are the offsets of the first ans last bytes of the
        section in the firmware file.

        Then verify the generated layout's consistency and return it to the
        caller.
        """

        layout_data = {} # keyed by the section name, elements - tuples of
                         # (<section start addr>, <section end addr>)

        for line in self._get_text_layout(file_name):
            d = self._line_to_dictionary(line)
            try:
                name = self.DEFAULT_CHROMEOS_FMAP_CONVERSION[d['area_name']]
            except KeyError:
                continue  # This line does not contain an area of interest.

            if name in layout_data:
                raise TestError('%s duplicated in the layout' % name)

            offset = int(d['area_offset'], 0)
            size = int(d['area_size'], 0)
            layout_data[name] = (offset, offset + size - 1)

        self.check_layout(layout_data, self.os_if.get_file_size(file_name))
        return layout_data

# flashrom utility wrapper
class flashrom_util(object):
    """ a wrapper for "flashrom" utility.

    You can read, write, or query flash ROM size with this utility.
    Although you can do "partial-write", the tools always takes a
    full ROM image as input parameter.

    NOTE before accessing flash ROM, you may need to first "select"
    your target - usually BIOS or EC. That part is not handled by
    this utility. Please find other external script to do it.

    To perform a read, you need to:
     1. Prepare a flashrom_util object
        ex: flashrom = flashrom_util.flashrom_util()
     2. Perform read operation
        ex: image = flashrom.read_whole()

        When the contents of the flashrom is read off the target, it's map
        gets created automatically (read from the flashrom image using
        'mosys'). If the user wants this object to operate on some other file,
        he could either have the map for the file created explicitly by
        invoking flashrom.set_firmware_layout(filename), or supply his own map
        (which is a dictionary where keys are section names, and values are
        tuples of integers, base address of the section and the last address
        of the section).

    By default this object operates on the map retrieved from the image and
    stored locally, this map can be overwritten by an explicitly passed user
    map.

   To perform a (partial) write:

     1. Prepare a buffer storing an image to be written into the flashrom.
     2. Have the map generated automatically or prepare your own, for instance:
        ex: layout_map_all = { 'all': (0, rom_size - 1) }
        ex: layout_map = { 'ro': (0, 0xFFF), 'rw': (0x1000, rom_size-1) }
     4. Perform write operation

        ex using default map:
          flashrom.write_partial(new_image, (<section_name>, ...))
        ex using explicitly provided map:
          flashrom.write_partial(new_image, layout_map_all, ('all',))

    Attributes:
        keep_temp_files: boolean flag to control cleaning of temporary files
    """

    def __init__(self, os_if, keep_temp_files=False,
                 target_is_ec=False):
        """ constructor of flashrom_util. help(flashrom_util) for more info """
        self.os_if = os_if
        self.keep_temp_files = keep_temp_files
        self.firmware_layout = {}
        self._target_command = ''
        if target_is_ec:
            self._enable_ec_access()
        else:
            self._enable_bios_access()

    def _enable_bios_access(self):
        if not self.os_if.target_hosted():
            return
        self._target_command = '-p host'

    def _enable_ec_access(self):
        if not self.os_if.target_hosted():
            return
        self._target_command = '-p ec'

    def _get_temp_filename(self, prefix):
        """Returns name of a temporary file in /tmp."""
        return self.os_if.create_temp_file(prefix)

    def _remove_temp_file(self, filename):
        """Removes a temp file if self.keep_temp_files is false."""
        if self.keep_temp_files:
            return
        if self.os_if.path_exists(filename):
            self.os_if.remove_file(filename)

    def _create_layout_file(self, layout_map):
        """Creates a layout file based on layout_map.

        Returns the file name containing layout information.
        """
        layout_text = ['0x%08lX:0x%08lX %s' % (v[0], v[1], k)
            for k, v in layout_map.items()]
        layout_text.sort()  # XXX unstable if range exceeds 2^32
        tmpfn = self._get_temp_filename('lay_')
        self.os_if.write_file(tmpfn, '\n'.join(layout_text) + '\n')
        return tmpfn

    def get_section(self, base_image, section_name):
        """
        Retrieves a section of data based on section_name in layout_map.
        Raises error if unknown section or invalid layout_map.
        """
        if section_name not in self.firmware_layout:
            return []
        pos = self.firmware_layout[section_name]
        if pos[0] >= pos[1] or pos[1] >= len(base_image):
            raise TestError('INTERNAL ERROR: invalid layout map: %s.' %
                            section_name)
        blob = base_image[pos[0] : pos[1] + 1]
        # Trim down the main firmware body to its actual size since the
        # signing utility uses the size of the input file as the size of
        # the data to sign. Make it the same way as firmware creation.
        if section_name in ('FVMAIN', 'FVMAINB', 'ECMAINA', 'ECMAINB'):
            align = 4
            pad = blob[-1]
            blob = blob.rstrip(pad)
            blob = blob + ((align - 1) - (len(blob) - 1) % align) * pad
        return blob

    def put_section(self, base_image, section_name, data):
        """
        Updates a section of data based on section_name in firmware_layout.
        Raises error if unknown section.
        Returns the full updated image data.
        """
        pos = self.firmware_layout[section_name]
        if pos[0] >= pos[1] or pos[1] >= len(base_image):
            raise TestError('INTERNAL ERROR: invalid layout map.')
        if len(data) != pos[1] - pos[0] + 1:
            # Pad the main firmware body since we trimed it down before.
            if (len(data) < pos[1] - pos[0] + 1 and section_name in
                    ('FVMAIN', 'FVMAINB', 'ECMAINA', 'ECMAINB')):
                pad = base_image[pos[1]]
                data = data + pad * (pos[1] - pos[0] + 1 - len(data))
            else:
                raise TestError('INTERNAL ERROR: unmatched data size.')
        return base_image[0 : pos[0]] + data + base_image[pos[1] + 1 :]

    def get_size(self):
        """ Gets size of current flash ROM """
        # TODO(hungte) Newer version of tool (flashrom) may support --get-size
        # command which is faster in future. Right now we use back-compatible
        # method: read whole and then get length.
        image = self.read_whole()
        return len(image)

    def set_firmware_layout(self, file_name):
        """get layout read from the BIOS """

        scraper = LayoutScraper(self.os_if)
        self.firmware_layout = scraper.get_layout(file_name)

    def enable_write_protect(self):
        """Enable the write pretection of the flash chip."""
        cmd = 'flashrom %s --wp-enable' % self._target_command
        self.os_if.run_shell_command(cmd)

    def disable_write_protect(self):
        """Disable the write pretection of the flash chip."""
        cmd = 'flashrom %s --wp-disable' % self._target_command
        self.os_if.run_shell_command(cmd)

    def read_whole(self):
        """
        Reads whole flash ROM data.
        Returns the data read from flash ROM, or empty string for other error.
        """
        tmpfn = self._get_temp_filename('rd_')
        cmd = 'flashrom %s -r "%s"' % (self._target_command, tmpfn)
        self.os_if.log('flashrom_util.read_whole(): %s' % cmd)
        self.os_if.run_shell_command(cmd)
        result = self.os_if.read_file(tmpfn)
        self.set_firmware_layout(tmpfn)

        # clean temporary resources
        self._remove_temp_file(tmpfn)
        return result

    def write_partial(self, base_image, write_list, write_layout_map=None):
        """
        Writes data in sections of write_list to flash ROM.
        An exception is raised if write operation fails.
        """

        if write_layout_map:
            layout_map = write_layout_map
        else:
            layout_map = self.firmware_layout

        tmpfn = self._get_temp_filename('wr_')
        self.os_if.write_file(tmpfn, base_image)
        layout_fn = self._create_layout_file(layout_map)

        cmd = 'flashrom %s -l "%s" -i %s -w "%s"' % (
                self._target_command, layout_fn, ' -i '.join(write_list), tmpfn)
        self.os_if.log('flashrom.write_partial(): %s' % cmd)
        self.os_if.run_shell_command(cmd)

        # flashrom write will reboot the ec after corruption
        # For Android, need to make sure ec is back online
        # before continuing, or adb command will cause test failure
        if self.os_if.is_android:
            self.os_if.wait_for_device(60)

        # clean temporary resources
        self._remove_temp_file(tmpfn)
        self._remove_temp_file(layout_fn)

    def write_whole(self, base_image):
        """Write the whole base image. """
        layout_map = { 'all': (0, len(base_image) - 1) }
        self.write_partial(base_image, ('all',), layout_map)
