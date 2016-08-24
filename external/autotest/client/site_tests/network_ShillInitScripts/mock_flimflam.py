import collections
import dbus
import dbus.service
import dbus.mainloop.glib
import gobject
import logging
import threading

""" MockFlimflam provides a select few methods from the flimflam
    DBus API so that we can track "dbus-send" invocations sent
    by the shill init scripts.  It could be used as a kernel for
    a test of other facilities that use the shill/flimflam DBus
    API and at that point it should be moved out of this specific
    test. """

MethodCall = collections.namedtuple("MethodCall", ["method", "argument"])

class FlimflamManager(dbus.service.Object):
    """ The flimflam DBus Manager object instance.  Methods in this
        object are called whenever a DBus RPC method is invoked. """
    def __init__(self, bus, object_path):
        dbus.service.Object.__init__(self, bus, object_path)
        self.method_calls = []


    @dbus.service.method('org.chromium.flimflam.Manager',
                         in_signature='s', out_signature='o')
    def CreateProfile(self, profile):
        """ Creates a profile.

        @param profile string name of profile to create.

        """
        self.add_method_call('CreateProfile', profile)
        return '/'


    @dbus.service.method('org.chromium.flimflam.Manager',
                         in_signature='s', out_signature='')
    def RemoveProfile(self, profile):
        """ Removes a profile.

        @param profile string name of profile to remove.

        """
        self.add_method_call('RemoveProfile', profile)


    @dbus.service.method('org.chromium.flimflam.Manager',
                         in_signature='s', out_signature='o')
    def PushProfile(self, profile):
        """ Pushes a profile.

        @param profile string name of profile to push.

        """
        self.add_method_call('PushProfile', profile)
        return '/'


    @dbus.service.method('org.chromium.flimflam.Manager',
                         in_signature='ss', out_signature='o')
    def InsertUserProfile(self, profile, user_hash):
        """ Inserts a profile.

        @param profile string name of profile to insert.
        @param user_hash string user hash associated with this profile.

        """
        self.add_method_call('InsertUserProfile', (profile, user_hash))
        return '/'


    @dbus.service.method('org.chromium.flimflam.Manager',
                         in_signature='s', out_signature='')
    def PopProfile(self, profile):
        """ Pops a profile.

        @param profile string name of profile to pop.

        """
        self.add_method_call('PopProfile', profile)


    @dbus.service.method('org.chromium.flimflam.Manager',
                         in_signature='', out_signature='')
    def PopAllUserProfiles(self):
        """ Pops all user profiles from the profile stack.. """
        self.add_method_call('PopAllUserProfiles', '')


    def add_method_call(self, method, arg):
        """ Note that a method call was made.

        @param method string the method that was called.
        @param arg tuple list of arguments that were called on |method|.

        """
        print "Called method %s" % method
        logging.info("Mock Flimflam method %s called with argument %s",
                     method, arg)
        self.method_calls.append(MethodCall(method, arg))


    def get_method_calls(self):
        """ Provide the method call list, clears this list internally.

        @return list of MethodCall objects

        """
        method_calls = self.method_calls
        self.method_calls = []
        return method_calls


class MockFlimflam(threading.Thread):
    """ This thread object instantiates a mock flimflam manager and
        runs a mainloop that receives DBus API messages. """
    FLIMFLAM = "org.chromium.flimflam"
    def __init__(self):
        threading.Thread.__init__(self)
        gobject.threads_init()


    def run(self):
        """ Runs the main loop. """
        dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
        self.bus = dbus.SystemBus()
        name = dbus.service.BusName(self.FLIMFLAM, self.bus)
        self.manager = FlimflamManager(self.bus, '/')
        self.mainloop = gobject.MainLoop()
        self.mainloop.run()


    def quit(self):
        """ Quits the main loop. """
        self.mainloop.quit()


    def get_method_calls(self):
        """ Returns the method calls that were called on the mock object.

        @return list of MethodCall objects representing the methods called.

         """
        return self.manager.get_method_calls()


if __name__ == '__main__':
    MockFlimflam().run()
