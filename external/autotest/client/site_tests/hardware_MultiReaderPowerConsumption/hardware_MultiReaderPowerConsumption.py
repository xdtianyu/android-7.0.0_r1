import os, logging

from autotest_lib.client.bin import utils
from autotest_lib.client.common_lib import error, autotemp
from autotest_lib.client.cros import power_status
from autotest_lib.client.cros import storage as storage_mod


class hardware_MultiReaderPowerConsumption(storage_mod.StorageTester):
    version = 1
    _files_to_delete = []
    _ramdisk_path = None
    _storage = None


    def initialize(self):
        super(hardware_MultiReaderPowerConsumption, self).initialize()

        # Make sure we're not on AC power
        self.status = power_status.get_status()
        if self.status.on_ac():
            raise error.TestNAError(
                  'This test needs to be run with the AC power offline')


    def cleanup(self):
        # Remove intermediate files
        for path in self._files_to_delete:
            utils.system('rm -f %s' % path)

        if self._storage and os.path.ismount(self._storage['mountpoint']):
            self.scanner.umount_volume(storage_dict=self._storage)

        if self._ramdisk_path and os.path.ismount(self._ramdisk_path.name):
            umount_ramdisk(self._ramdisk_path.name)
            self._ramdisk_path.clean()

        super(hardware_MultiReaderPowerConsumption, self).cleanup()


    def readwrite_test(self, path, size, delete_file=False):
        """Heavy-duty random read/write test. Run `dd` & `tail -f` in parallel

        The random write is done by writing a file from /dev/urandom into the
        given location, while the random read is done by concurrently reading
        that file.

        @param path: The directory that will create the test file.
        @param size: Size of the test file, in MiB.
        @param delete_file: Flag the file to be deleted on test exit.
               Otherwise file deletion won't be performed.
        """
        # Calculate the parameters for dd
        size = 1024*1024*size
        blocksize = 8192

        # Calculate the filename and full path, flag to delete if needed
        filename = 'tempfile.%d.delete-me' % size
        pathfile = os.path.join(path, filename)
        if delete_file:
            self._files_to_delete.append(pathfile)

        pid = os.fork() # We need to run two processes in parallel
        if pid:
            # parent
            utils.BgJob('tail -f %s --pid=%s > /dev/null'
                        % (pathfile, pid))
            # Reap the dd child so that tail does not wait for the zombie
            os.waitpid(pid, 0)
        else:
            # child
            utils.system('dd if=/dev/urandom of=%s bs=%d count=%s'
                         % (pathfile, blocksize, (size//blocksize)))
            # A forked child is exiting here, so we really do want os._exit:
            os._exit(0)


    def run_once(self, ramdisk_size=513, file_size=512, drain_limit=1.05,
                 volume_filter={'bus': 'usb'}):
        """Test card reader CPU power consumption to be within acceptable
        range while performing random r/w

        The random r/w is performed in the readwrite_test() method, by
        concurrently running `dd if=/dev/urandom` and `tail -f`. It is run once
        on a ramdisk with the SD card mounted, then on the SD card with the
        ramdisk unmounted, and then on the SD card with the ramdisk unmounted.
        The measured values are then reported.

        @param ramdisk_size: Size of ramdisk (in MiB).
        @param file_size: Size of test file (in MiB).
        @param volume_filter: Where to find the card reader.
        @param drain_limit: maximum ratio between the card reader
                            energy consumption and each of the two
                            ramdisk read/write test energy consumption
                            values. 1.00 means the card reader test may
                            not consume more energy than either ramdisk
                            test, 0.9 means it may consume no more than
                            90% of the ramdisk value, and so forth.
        """
        # Switch to VT2 so the screen turns itself off automatically instead of
        # dimming, in order to reduce the battery consuption caused by other
        # variables.
        utils.system('chvt 2')

        logging.debug('STEP 1: ensure SD card is inserted and mounted')
        self._storage = self.wait_for_device(volume_filter, cycles=1,
                                             mount_volume=True)[0]

        logging.debug('STEP 2: mount the ramdisk')
        self._ramdisk_path = autotemp.tempdir(unique_id='ramdisk',
                                              dir=self.tmpdir)
        mount_ramdisk(self._ramdisk_path.name, ramdisk_size)

        # Read current charge, as well as maximum charge.
        self.status.refresh()
        max_charge = self.status.battery[0].charge_full_design
        initial_charge = self.status.battery[0].charge_now

        logging.debug('STEP 3: perform heavy-duty read-write test on ramdisk')
        self.readwrite_test(self._ramdisk_path.name, file_size)
        # Read current charge (reading A)
        self.status.refresh()
        charge_A = self.status.battery[0].charge_now

        logging.debug('STEP 4: unmount ramdisk')
        umount_ramdisk(self._ramdisk_path.name)

        logging.debug('STEP 5: perform identical read write test on SD card')
        self.readwrite_test(self._storage['mountpoint'], file_size,
                            delete_file=True)
        # Read current charge (reading B)
        self.status.refresh()
        charge_B = self.status.battery[0].charge_now

        logging.debug('STEP 6: unmount card')
        self.scanner.umount_volume(storage_dict=self._storage, args='-f -l')

        logging.debug('STEP 7: perform ramdisk test again')
        mount_ramdisk(self._ramdisk_path.name, ramdisk_size)
        self.readwrite_test(self._ramdisk_path.name, file_size)
        # Read current charge (reading C)
        self.status.refresh()
        charge_C = self.status.battery[0].charge_now

        # Compute the results
        ramdisk_plus = initial_charge - charge_A
        sd_card_solo = charge_A - charge_B
        ramdisk_solo = charge_B - charge_C

        sd_card_drain_ratio_a = (sd_card_solo / ramdisk_plus)
        sd_card_drain_ratio_b = (sd_card_solo / ramdisk_solo)

        msg = None
        if sd_card_drain_ratio_a > drain_limit:
            msg = ('Card reader drain exceeds mounted baseline by > %f (%f)'
                   % (drain_limit, sd_card_drain_ratio_a))
        elif sd_card_drain_ratio_b > drain_limit:
            msg = ('Card reader drain exceeds unmounted baseline by > %f (%f)'
                   % (drain_limit, sd_card_drain_ratio_b))

        if msg:
            raise error.TestError(msg)
        else:
            fmt = 'Card reader drain ratio Ok: mounted %f; unmounted %f'
            logging.info(fmt % (sd_card_drain_ratio_a, sd_card_drain_ratio_b))


def mount_ramdisk(path, size):
    utils.system('mount -t tmpfs none %s -o size=%sm' % (path, size))


def umount_ramdisk(path):
    """Umount ramdisk mounted at |path|

    @param path: the mountpoint for the mountd RAM disk
    """
    utils.system('rm -rf %s/*' % path)
    utils.system('umount -f -l %s' % path)
