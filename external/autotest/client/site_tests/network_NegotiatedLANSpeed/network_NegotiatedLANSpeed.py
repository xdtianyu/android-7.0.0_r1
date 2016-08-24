import logging, re, utils
from autotest_lib.client.bin import test
from autotest_lib.client.common_lib import error

class network_NegotiatedLANSpeed(test.test):
    version = 1


    def run_once(self, iface_name = 'eth0'):
        # bring up the interface if its not already up
        if not self.iface_up(iface_name):
            utils.system('ifconfig %s up' % iface_name)
            if not self.iface_up(iface_name):
                raise error.TestFail('interface failed to come up')
        # confirm negotiated bandwidth is acceptable
        if not int(self.get_speed(iface_name)) >= 1000:
            raise error.TestFail('interface failed to negotiate at 1000Mbps')


    def iface_up(self, name):
        try:
            out = utils.system_output('ifconfig %s' % name)
        except error.CmdError, e:
            logging.info(e)
            raise error.TestFail('test interface not found')
        match = re.search('UP', out, re.S)
        return match


    def get_speed(self, name):
        try:
            out = utils.system_output('ethtool %s | grep Speed | \
                sed s/^.*:.// | sed s/M.*$//' % name)
        except error.CmdError, e:
            logging.info(e)
            raise error.TestFail('unable to determine negotiated link speed')
        return out
