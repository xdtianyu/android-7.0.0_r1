# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import dbus
import json
import logging
import os.path
from xml.dom.minidom import parse, parseString

from autotest_lib.client.bin import test, utils
from autotest_lib.client.common_lib import error
from autotest_lib.client.cros import constants, login

class security_DbusMap(test.test):
    version = 2

    def policy_sort_priority(self, policy):
        """
        Given a DOMElement representing one <policy> block from a dbus
        configuraiton file, return a number suitable for determining
        the order in which this policy would be applied by dbus-daemon.
        For example, by returning:
        0 for 'default' policies
        1 for 'group' policies
        2 for 'user' policies
        ... these numbers can be used as a sort-key for sorting
        an array of policies into the order they would be evaluated by
        dbus-daemon.
        """
        # As derived from dbus-daemon(1) manpage
        if policy.getAttribute('context') == 'default':
            return 0
        if policy.getAttribute('group') != '':
            return 1
        if policy.getAttribute('user') != '':
            return 2
        if policy.getAttribute('at_console') == 'true':
            return 3
        if policy.getAttribute('at_console') == 'false':
            return 4
        if policy.getAttribute('context') == 'mandatory':
            return 5


    def sort_policies(self, policies):
        """
        Given an array of DOMElements representing <policy> blocks,
        return a sorted copy of the array. Sorting is determined by
        the order in which dbus-daemon(1) would consider the rules.
        This is a stable sort, so in cases where dbus would employ
        "last rule wins," position in the input list will be honored.
        """
        # Use decorate-sort-undecorate to minimize calls to
        # policy_sort_priority(). See http://wiki.python.org/moin/HowTo/Sorting
        decorated = [(self.policy_sort_priority(policy), i, policy) for
                     i, policy in enumerate(policies)]
        decorated.sort()
        return [policy for _,_,policy in decorated]


    def check_policies(self, config_doms, dest, iface, member,
                       user='chronos', at_console=None):
        """
        Given 1 or more xml.dom's representing dbus configuration
        data, determine if the <destination, interface, member>
        triplet specified in the arguments would be permitted for
        the specified user.

        Returns True if permitted, False otherwise.
        See also http://dbus.freedesktop.org/doc/busconfig.dtd
        """
        # In the default case, if the caller doesn't specify
        # "at_console," employ this cros-specific heuristic:
        if user == 'chronos' and at_console == None:
            at_console = True

        # Apply the policies iteratively, in the same order
        # that dbus-daemon(1) would consider them.
        allow = False
        for dom in config_doms:
            for buscfg in dom.getElementsByTagName('busconfig'):
                policies = self.sort_policies(
                    buscfg.getElementsByTagName('policy'))
                for policy in policies:
                    ruling = self.check_one_policy(policy, dest, iface,
                                                   member, user, at_console)
                    if ruling is not None:
                        allow = ruling
        return allow


    def check_one_policy(self, policy, dest, iface, member,
                         user='chronos', at_console=True):
        """
        Given a DOMElement representing one <policy> block from a dbus
        configuration file, determine if the <destination, interface,
        member> triplet specified in the arguments would be permitted
        for the specified user.

        Returns True if permitted, False if prohibited, or
        None if the policy does not apply to the triplet.
        """
        # While D-Bus overall is a default-deny, this individual
        # rule may not match, and some previous rule may have already
        # said "allow" for this interface/method. So, we work from
        # here starting with "doesn't apply," not "deny" to avoid
        # falsely masking any previous "allow" rules.
        allow = None

        # TODO(jimhebert) group='...' is not currently used by any
        # Chrome OS dbus policies but could be in the future so
        # we should add a check for it in this if-block.
        if ((policy.getAttribute('context') != 'default') and
            (policy.getAttribute('user') != user) and
            (policy.getAttribute('at_console') != 'true')):
            # In this case, the entire <policy> block does not apply
            return None

        # Alternatively, if this IS a at_console policy, but the
        # situation being checked is not an "at_console" situation,
        # then that's another way the policy would also not apply.
        if (policy.getAttribute('at_console') == 'true' and not
            at_console):
            return None

        # If the <policy> applies, try to find <allow> or <deny>
        # child nodes that apply:
        for node in policy.childNodes:
            if (node.nodeType == node.ELEMENT_NODE and
                node.localName in ['allow','deny']):
                ruling = self.check_one_node(node, dest, iface, member)
                if ruling is not None:
                    allow = ruling
        return allow


    def check_one_node(self, node, dest, iface, member):
        """
        Given a DOMElement representing one <allow> or <deny> tag from a
        dbus configuration file, determine if the <destination, interface,
        member> triplet specified in the arguments would be permitted.

        Returns True if permitted, False if prohibited, or
        None if the policy does not apply to the triplet.
        """
        # Require send_destination to match (if we accept missing
        # send_destination we end up falsely processing tags like
        # <allow own="...">). But, do not require send_interface
        # or send_member to exist, because omitting them is used
        # as a way of wildcarding in dbus configuration.
        if ((node.getAttribute('send_destination') == dest) and
            (not node.hasAttribute('send_interface') or
             node.getAttribute('send_interface') == iface) and
            (not node.hasAttribute('send_member') or
             node.getAttribute('send_member') == member)):
            # The rule applies! Return True if it's an allow rule, else false
            logging.debug(('%s send_destination=%s send_interface=%s '
                           'send_member=%s applies to %s %s %s.') %
                          (node.localName,
                           node.getAttribute('send_destination'),
                           node.getAttribute('send_interface'),
                           node.getAttribute('send_member'),
                           dest, iface, member))
            return (node.localName == 'allow')
        else:
            return None


    def load_dbus_config_doms(self, dbusdir='/etc/dbus-1/system.d'):
        """
        Given a path to a directory containing valid dbus configuration
        files (http://dbus.freedesktop.org/doc/busconfig.dtd), return
        a series of parsed DOMs representing the configuration.
        This function implements the same requirements as dbus-daemon
        itself -- notably, that valid config files must be named
        with a ".conf" extension.
        Returns: a list of DOMs
        """
        config_doms = []
        for dirent in os.listdir(dbusdir):
            dirent = os.path.join(dbusdir, dirent)
            if os.path.isfile(dirent) and dirent.endswith('.conf'):
                config_doms.append(parse(dirent))
        return config_doms


    def mutual_compare(self, dbus_list, baseline, context='all'):
        """
        This is a front-end for compare_dbus_trees which handles
        comparison in both directions, discovering not only what is
        missing from the baseline, but what is missing from the system.

        The optional 'context' argument is (only) used to for
        providing more detailed context in the debug-logging
        that occurs.

        Returns: True if the two exactly match. False otherwise.
        """
        self.sort_dbus_tree(dbus_list)
        self.sort_dbus_tree(baseline)

        # Compare trees to find added API's.
        newapis = self.compare_dbus_trees(dbus_list, baseline)
        if (len(newapis) > 0):
            logging.error("New (accessible to %s) API's to review:" % context)
            logging.error(json.dumps(newapis, sort_keys=True, indent=2))

        # Swap arguments to find missing API's.
        missing_apis = self.compare_dbus_trees(baseline, dbus_list)
        if (len(missing_apis) > 0):
            logging.error("Missing API's (expected to be accessible to %s):" %
                          context)
            logging.error(json.dumps(missing_apis, sort_keys=True, indent=2))

        return (len(newapis) + len(missing_apis) == 0)


    def add_member(self, dbus_list, dest, iface, member):
        return self._add_surface(dbus_list, dest, iface, member, 'methods')


    def add_signal(self, dbus_list, dest, iface, signal):
        return self._add_surface(dbus_list, dest, iface, signal, 'signals')


    def add_property(self, dbus_list, dest, iface, signal):
        return self._add_surface(dbus_list, dest, iface, signal, 'properties')


    def _add_surface(self, dbus_list, dest, iface, member, slot):
        """
        This can add an entry for a member function to a given
        dbus list. It behaves somewhat like "mkdir -p" in that
        it creates any missing, necessary intermediate portions
        of the data structure. For example, if this is the first
        member being added for a given interface, the interface
        will not already be mentioned in dbus_list, and this
        function initializes the interface dictionary appropriately.
        Returns: None
        """
        # Ensure the Destination object exists in the data structure.
        dest_idx = -1
        for (i, objdict) in enumerate(dbus_list):
            if objdict['Object_name'] == dest:
                dest_idx = i
        if dest_idx == -1:
            dbus_list.append({'Object_name': dest, 'interfaces': []})

        # Ensure the Interface entry exists for that Destination object.
        iface_idx = -1
        for (i, ifacedict) in enumerate(dbus_list[dest_idx]['interfaces']):
            if ifacedict['interface'] == iface:
                iface_idx = i
        if iface_idx == -1:
            dbus_list[dest_idx]['interfaces'].append({'interface': iface,
                                                      'signals': [],
                                                      'properties': [],
                                                      'methods': []})

        # Ensure the slot exists.
        if not slot in dbus_list[dest_idx]['interfaces'][iface_idx]:
            dbus_list[dest_idx]['interfaces'][iface_idx][slot] = []

        # Add member so long as it's not a duplicate.
        if not member in (
            dbus_list[dest_idx]['interfaces'][iface_idx][slot]):
            dbus_list[dest_idx]['interfaces'][iface_idx][slot].append(
                member)


    def list_baselined_users(self):
        """
        Return a list of usernames for which we keep user-specific
        attack-surface baselines.
        """
        bdir = os.path.dirname(os.path.abspath(__file__))
        users = []
        for item in os.listdir(bdir):
            # Pick up baseline.username files but ignore emacs backups.
            if item.startswith('baseline.') and not item.endswith('~'):
                users.append(item.partition('.')[2])
        return users


    def load_baseline(self, user=''):
        """
        Return a list of interface names we expect to be owned
        by chronos.
        """
        # The overall baseline is 'baseline'. User-specific baselines are
        # stored in files named 'baseline.<username>'.
        baseline_name = 'baseline'
        if user:
            baseline_name = '%s.%s' % (baseline_name, user)

        # Figure out path to baseline file, by looking up our own path.
        bpath = os.path.abspath(__file__)
        bpath = os.path.join(os.path.dirname(bpath), baseline_name)
        return self.load_dbus_data_from_disk(bpath)


    def write_dbus_data_to_disk(self, dbus_list, file_path):
        """Writes the given dbus data to a given path to a json file.
        Args:
            dbus_list: list of dbus dictionaries to write to disk.
            file_path: the path to the file to write the data to.
        """
        file_handle = open(file_path, 'w')
        my_json = json.dumps(dbus_list, sort_keys=True, indent=2)
        # The json dumper has a trailing whitespace problem, and lacks
        # a final newline. Fix both here.
        file_handle.write(my_json.replace(', \n',',\n') + '\n')
        file_handle.close()


    def load_dbus_data_from_disk(self, file_path):
        """Loads dbus data from a given path to a json file.
        Args:
            file_path: path to the file as a string.
        Returns:
            A list of the dictionary representation of the dbus data loaded.
            The dictionary format is the same as returned by walk_object().
        """
        file_handle = open(file_path, 'r')
        dbus_data = json.loads(file_handle.read())
        file_handle.close()
        return dbus_data


    def sort_dbus_tree(self, tree):
        """Sorts a an aray of dbus dictionaries in alphabetical order.
             All levels of the tree are sorted.
        Args:
            tree: the array to sort. Modified in-place.
        """
        tree.sort(key=lambda x: x['Object_name'])
        for dbus_object in tree:
            dbus_object['interfaces'].sort(key=lambda x: x['interface'])
            for interface in dbus_object['interfaces']:
                interface['methods'].sort()
                interface['signals'].sort()
                interface['properties'].sort()


    def compare_dbus_trees(self, current, baseline):
        """Compares two dbus dictionaries and return the delta.
           The comparison only returns what is in the current (LHS) and not
           in the baseline (RHS). If you want the reverse, call again
           with the arguments reversed.
        Args:
            current: dbus tree you want to compare against the baseline.
            baseline: dbus tree baseline.
        Returns:
            A list of dictionary representations of the additional dbus
            objects, if there is a difference. Otherwise it returns an
            empty list. The format of the dictionaries is the same as the
            one returned in walk_object().
        """
        # Build the key map of what is in the baseline.
        bl_object_names = [bl_object['Object_name'] for bl_object in baseline]

        new_items = []
        for dbus_object in current:
            if dbus_object['Object_name'] in bl_object_names:
                index = bl_object_names.index(dbus_object['Object_name'])
                bl_object_interfaces = baseline[index]['interfaces']
                bl_interface_names = [name['interface'] for name in
                                      bl_object_interfaces]

                # If we have a new interface/method we need to build the shell.
                new_object = {'Object_name':dbus_object['Object_name'],
                              'interfaces':[]}

                for interface in dbus_object['interfaces']:
                    if interface['interface'] in bl_interface_names:
                        # The interface was baselined, check everything.
                        diffslots = {}
                        for slot in ['methods', 'signals', 'properties']:
                            index = bl_interface_names.index(
                                interface['interface'])
                            bl_methods = set(bl_object_interfaces[index][slot])
                            methods = set(interface[slot])
                            difference = methods.difference(bl_methods)
                            diffslots[slot] = list(difference)
                        if (diffslots['methods'] or diffslots['signals'] or
                            diffslots['properties']):
                            # This is a new thing we need to track.
                            new_methods = {'interface':interface['interface'],
                                           'methods': diffslots['methods'],
                                           'signals': diffslots['signals'],
                                           'properties': diffslots['properties']
                                           }
                            new_object['interfaces'].append(new_methods)
                            new_items.append(new_object)
                    else:
                        # This is a new interface we need to track.
                        new_object['interfaces'].append(interface)
                        new_items.append(new_object)
            else:
                # This is a new object we need to track.
                new_items.append(dbus_object)
        return new_items


    def walk_object(self, bus, object_name, start_path, dbus_objects):
        """Walks the given bus and object returns a dictionary representation.
           The formate of the dictionary is as follows:
           {
               Object_name: "string"
               interfaces:
               [
                   interface: "string"
                   methods:
                   [
                       "string1",
                       "string2"
                   ]
               ]
           }
           Note that the decision to capitalize Object_name is just
           a way to force it to appear above the interface-list it
           corresponds to, when pretty-printed by the json dumper.
           This makes it more logical for humans to read/edit.
        Args:
            bus: the bus to query, usually system.
            object_name: the name of the dbus object to walk.
            start_path: the path inside of the object in which to start walking
            dbus_objects: current list of dbus objects in the given object
        Returns:
            A dictionary representation of a dbus object
        """
        remote_object = bus.get_object(object_name,start_path)
        unknown_iface = dbus.Interface(remote_object,
                                       'org.freedesktop.DBus.Introspectable')
        # Convert the string to an xml DOM object we can walk.
        xml = parseString(unknown_iface.Introspect())
        for child in xml.childNodes:
            if ((child.nodeType == 1) and (child.localName == u'node')):
                interfaces = child.getElementsByTagName('interface')
                for interface in interfaces:
                    interface_name = interface.getAttribute('name')
                    # First get the methods.
                    methods = interface.getElementsByTagName('method')
                    method_list = []
                    for method in methods:
                        method_list.append(method.getAttribute('name'))
                    # Repeat the process for signals.
                    signals = interface.getElementsByTagName('signal')
                    signal_list = []
                    for signal in signals:
                        signal_list.append(signal.getAttribute('name'))
                    # Properties have to be discovered via API call.
                    prop_list = []
                    try:
                        prop_iface = dbus.Interface(remote_object,
                            'org.freedesktop.DBus.Properties')
                        prop_list = prop_iface.GetAll(interface_name).keys()
                    except dbus.exceptions.DBusException:
                        # Many daemons do not support this interface,
                        # which means they have no properties.
                        pass
                    # Create the dictionary with all the above.
                    dictionary = {'interface':interface_name,
                                  'methods':method_list, 'signals':signal_list,
                                  'properties':prop_list}
                    if dictionary not in dbus_objects:
                        dbus_objects.append(dictionary)
                nodes = child.getElementsByTagName('node')
                for node in nodes:
                    name = node.getAttribute('name')
                    if start_path[-1] != '/':
                            start_path = start_path + '/'
                    new_name = start_path + name
                    self.walk_object(bus, object_name, new_name, dbus_objects)
        return {'Object_name':('%s' % object_name), 'interfaces':dbus_objects}


    def mapper_main(self):
        # Currently we only dump the SystemBus. Accessing the SessionBus says:
        # "ExecFailed: /usr/bin/dbus-launch terminated abnormally with the
        # following error: Autolaunch requested, but X11 support not compiled
        # in."
        # If this changes at a later date, add dbus.SessionBus() to the dict.
        # We've left the code structured to support walking more than one bus
        # for such an eventuality.

        buses = {'System Bus': dbus.SystemBus()}

        for busname in buses.keys():
            bus = buses[busname]
            remote_dbus_object = bus.get_object('org.freedesktop.DBus',
                                                '/org/freedesktop/DBus')
            iface = dbus.Interface(remote_dbus_object, 'org.freedesktop.DBus')
            dbus_list = []
            for i in iface.ListNames():
                # There are some strange listings like ":1" which appear after
                # certain names. Ignore these since we just need the names.
                if i.startswith(':'):
                    continue
                dbus_list.append(self.walk_object(bus, i, '/', []))

        # Dump the complete observed dataset to disk. In the somewhat
        # typical case, that we will want to rev the baseline to
        # match current reality, these files are easily copied and
        # checked in as a new baseline.
        self.sort_dbus_tree(dbus_list)
        observed_data_path = os.path.join(self.outputdir, 'observed')
        self.write_dbus_data_to_disk(dbus_list, observed_data_path)

        baseline = self.load_baseline()
        test_pass = self.mutual_compare(dbus_list, baseline)

        # Figure out which of the observed API's are callable by specific users
        # whose attack surface we are particularly sensitive to:
        dbus_cfg = self.load_dbus_config_doms()
        for user in self.list_baselined_users():
            user_baseline = self.load_baseline(user)
            user_observed = []
            # user_observed will be a subset of dbus_list. Iterate and check
            # against the configured dbus policies as we go:
            for objdict in dbus_list:
                for ifacedict in objdict['interfaces']:
                    for meth in ifacedict['methods']:
                        if (self.check_policies(dbus_cfg,
                                                objdict['Object_name'],
                                                ifacedict['interface'], meth,
                                                user=user)):
                            self.add_member(user_observed,
                                            objdict['Object_name'],
                                            ifacedict['interface'], meth)
                    # We don't do permission-checking on signals because
                    # signals are allow-all by default. Just copy them over.
                    for sig in ifacedict['signals']:
                        self.add_signal(user_observed,
                                        objdict['Object_name'],
                                        ifacedict['interface'], sig)
                    # A property might be readable, or even writable, to
                    # a given user if they can reach the Get/Set interface
                    access = []
                    if (self.check_policies(dbus_cfg, objdict['Object_name'],
                                            'org.freedesktop.DBus.Properties',
                                            'Set', user=user)):
                        access.append('Set')
                    if (self.check_policies(dbus_cfg, objdict['Object_name'],
                                            'org.freedesktop.DBus.Properties',
                                            'Get', user=user) or
                        self.check_policies(dbus_cfg, objdict['Object_name'],
                                            'org.freedesktop.DBus.Properties',
                                            'GetAll', user=user)):
                        access.append('Get')
                    if access:
                        access = ','.join(access)
                        for prop in ifacedict['properties']:
                            self.add_property(user_observed,
                                              objdict['Object_name'],
                                              ifacedict['interface'],
                                              '%s (%s)' % (prop, access))

            self.write_dbus_data_to_disk(user_observed,
                                         '%s.%s' % (observed_data_path, user))
            test_pass = test_pass and self.mutual_compare(user_observed,
                                                          user_baseline, user)
        if not test_pass:
            raise error.TestFail('Baseline mismatch(es)')


    def run_once(self):
        """
        Enumerates all discoverable interfaces, methods, and signals
        in dbus-land. Verifies that it matches an expected set.
        """
        login.wait_for_browser()
        self.mapper_main()
