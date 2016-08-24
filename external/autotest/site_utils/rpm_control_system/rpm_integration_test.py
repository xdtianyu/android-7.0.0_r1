import logging
import threading

import common

from autotest_lib.site_utils.rpm_control_system import rpm_controller
from autotest_lib.site_utils.rpm_control_system import utils


# Format Appears as: [Date] [Time] - [Msg Level] - [Message]
LOGGING_FORMAT = '%(asctime)s - %(levelname)s - %(message)s'


def test_in_order_requests():
    """Simple integration testing."""
    rpm = rpm_controller.WebPoweredRPMController(
            'chromeos-rack8e-rpm1')
    info_1 = utils.PowerUnitInfo(
            device_hostname='chromeos1-rack8e-hostbs1',
            powerunit_hostname='chromeos-rack8e-rpm1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='')
    info_2 = utils.PowerUnitInfo(
            device_hostname='chromeos1-rack8e-hostbs2',
            powerunit_hostname='chromeos-rack8e-rpm1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='')
    info_3 = utils.PowerUnitInfo(
            device_hostname='chromeos1-rack8e-hostbs3',
            powerunit_hostname='chromeos-rack8e-rpm1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='')
    rpm.queue_request(info_1, 'OFF')
    rpm.queue_request(info_2, 'OFF')
    rpm.queue_request(info_3, 'CYCLE')


def test_parrallel_webrequests():
    """Simple integration testing."""
    rpm = rpm_controller.WebPoweredRPMController(
            'chromeos-rack8e-rpm1')
    info_1 = utils.PowerUnitInfo(
            device_hostname='chromeos1-rack8e-hostbs1',
            powerunit_hostname='chromeos-rack8e-rpm1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='')
    info_2 = utils.PowerUnitInfo(
            device_hostname='chromeos1-rack8e-hostbs2',
            powerunit_hostname='chromeos-rack8e-rpm1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='')
    threading.Thread(target=rpm.queue_request,
                     args=(info_1, 'OFF')).start()
    threading.Thread(target=rpm.queue_request,
                     args=(info_2, 'ON')).start()


def test_parrallel_sshrequests():
    """Simple integration testing."""
    rpm =   rpm_controller.SentryRPMController('chromeos-rack8-rpm1')
    info_1 = utils.PowerUnitInfo(
            device_hostname='chromeos-rack8-hostbs1',
            powerunit_hostname='chromeos-rack8-rpm1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='.A14')
    info_2 = utils.PowerUnitInfo(
            device_hostname='chromeos-rack8-hostbs2',
            powerunit_hostname='chromeos-rack8-rpm1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='.A11')
    threading.Thread(target=rpm.queue_request,
                     args=(info_1, 'CYCLE')).start()
    threading.Thread(target=rpm.queue_request,
                     args=(info_2, 'CYCLE')).start()

    # The following test are disabled as the
    # outlets on the rpm are in actual use.
    # rpm2 = SentryRPMController('chromeos2-row2-rack3-rpm1')
    # threading.Thread(target=rpm2.queue_request,
    #                  args=('chromeos2-row2-rack3-hostbs', 'ON')).start()
    # threading.Thread(target=rpm2.queue_request,
    #                  args=('chromeos2-row2-rack3-hostbs2', 'ON')).start()
    # threading.Thread(target=rpm2.queue_request,
    #                  args=('chromeos2-row1-rack7-hostbs1', 'ON')).start()


def test_in_order_poerequests():
    """Simple integration testing for poe controller."""
    poe_controller = rpm_controller.CiscoPOEController(
            'chromeos1-poe-switch1')
    info_1 = utils.PowerUnitInfo(
            device_hostname='chromeos1-rack4-host1bs-servo',
            powerunit_hostname='chromeos1-poe-switch1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='fa33')
    info_2 = utils.PowerUnitInfo(
            device_hostname='chromeos1-rack4-host2bs-servo',
            powerunit_hostname='chromeos1-poe-switch1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='fa34')
    poe_controller.queue_request(info_1, 'OFF')
    poe_controller.queue_request(info_1, 'ON')
    poe_controller.queue_request(info_2, 'CYCLE')


def test_parrallel_poerequests():
    """Simple integration testing for poe controller."""
    poe_controller = rpm_controller.CiscoPOEController(
            'chromeos1-poe-switch1')
    info_1 = utils.PowerUnitInfo(
            device_hostname='chromeos1-rack4-host1bs-servo',
            powerunit_hostname='chromeos1-poe-switch1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='fa33')
    info_2 = utils.PowerUnitInfo(
            device_hostname='chromeos1-rack4-host2bs-servo',
            powerunit_hostname='chromeos1-poe-switch1',
            powerunit_type=utils.PowerUnitInfo.POWERUNIT_TYPES.RPM,
            hydra_hostname=None,
            outlet='fa34')
    threading.Thread(target=poe_controller.queue_request,
                     args=(info_1, 'CYCLE')).start()
    threading.Thread(target=poe_controller.queue_request,
                     args=(info_2, 'CYCLE')).start()


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG, format=LOGGING_FORMAT)
#    The tests in this file are disabled since most of the ports are
#    in actual use now. If you are going to run them, make sure
#    to choose unused hosts/ports.
#    test_in_order_requests()
#    test_parrallel_webrequests()
#    test_parrallel_sshrequests()
#    test_in_order_poerequests()
#    test_parrallel_poerequests()
