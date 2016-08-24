import logging, re, time

from autotest_lib.server import autotest, test, hosts
from autotest_lib.server.cros import stress
from autotest_lib.client.common_lib import error

# Timeout duration to wait for a DHCP response.
# It usually doesn't take this long, but just in case.
TIMEOUT = 60

class network_StressServoEthernetPlug(test.test):

    ETH_MAC = 'mac'
    ETH_IP = 'ipaddress'

    version = 1


    def initialize(self, host):
        self.host = host
        self.host_iface = None
        self.servo_iface = None
        self.servo_eth_up()

        end_time = time.time() + TIMEOUT
        while time.time() < end_time:
            self.eth_interfaces = self.get_eth_interfaces()
            if len(self.eth_interfaces) >= 2:
                break
            time.sleep(1)

        # Assuming 2 ethernet interfaces, the interface not for host
        # is that associated with servo.
        for iface, eth_dict in self.eth_interfaces.iteritems():
            if eth_dict[self.ETH_IP] == self.host.hostname:
                self.host_iface = iface
            else:
                self.servo_iface = iface

        if not self.servo_iface:
            raise error.TestError('Cannot find servo ethernet interface')

        logging.info('Servo eth: %s', self.servo_iface)
        logging.info('Host eth: %s', self.host_iface)


    def servo_eth_up(self):
        logging.info('Bringing up ethernet')
        self.host.servo.set('dut_hub_on', 'yes')


    def servo_eth_down(self):
        logging.info('Bringing down ethernet')
        self.host.servo.set('dut_hub_on', 'no')


    def get_eth_interfaces(self):
        """ Gets the ethernet device object.

        Returns:
            A dictionary of ethernet devices.
            {
                'eth<x>':
                {
                    'mac': <mac address>,
                    'ipaddress': <ipaddress>,
                }
            }
        """
        results = self.host.run('ifconfig').stdout.split('\n')
        eth_dict = {}

        iterator = results.__iter__()
        for line in iterator:
            # Search for the beginning of an interface section.
            iface_start = re.search('^(eth\S+)\s+Link encap:Ethernet\s+HWaddr'
                                    '\s+(\S+)', line)
            if iface_start:
                (iface, hwaddr) = iface_start.groups()
                line = iterator.next()
                result = re.search('^\s+inet addr:(\S+)\s+', line)
                ipaddress = None
                if result:
                    ipaddress = result.groups()[0]
                eth_dict[iface] = {self.ETH_MAC: hwaddr, self.ETH_IP: ipaddress}
        return eth_dict


    def verify_eth_status(self, up_list, timeout=TIMEOUT):
        """ Verify the up_list ifaces are up (and its contrapositive). """
        end_time = time.time() + timeout
        interfaces = {}
        while time.time() < end_time:
            interfaces = self.get_eth_interfaces()
            error_message = ('Expected eth status %s but instead got %s' %
                             (up_list, interfaces.keys()))
            if set(interfaces.keys()) == set(up_list):
                # Check to make sure all the interfaces are up.
                for iface, eth_dict in interfaces.iteritems():
                    if not eth_dict[self.ETH_IP]:
                        error_message = ('Ethernet interface %s did not '
                                         'receive address.' % iface)
                        break
                else:
                    # All desired interfaces are up, and they have ip addresses.
                    break
            time.sleep(1)
        else:
            # If the while loop terminates without interruption, we've timed out
            # waiting for the interface.
            raise error.TestFail(error_message)


    def run_once(self, num_iterations=1):
        for iteration in range(num_iterations):
            logging.info('Executing iteration %d', iteration)
            self.servo_eth_down()
            self.verify_eth_status([self.host_iface])
            self.servo_eth_up()
            self.verify_eth_status([self.host_iface, self.servo_iface])
