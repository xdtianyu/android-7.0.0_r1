#
# Copyright 2008 Google Inc. All Rights Reserved.
#
"""
This module contains the generic CLI object

High Level Design:

The atest class contains attributes & method generic to all the CLI
operations.

The class inheritance is shown here using the command
'atest host create ...' as an example:

atest <-- host <-- host_create <-- site_host_create

Note: The site_<topic>.py and its classes are only needed if you need
to override the common <topic>.py methods with your site specific ones.


High Level Algorithm:

1. atest figures out the topic and action from the 2 first arguments
   on the command line and imports the <topic> (or site_<topic>)
   module.

1. Init
   The main atest module creates a <topic>_<action> object.  The
   __init__() function is used to setup the parser options, if this
   <action> has some specific options to add to its <topic>.

   If it exists, the child __init__() method must call its parent
   class __init__() before adding its own parser arguments.

2. Parsing
   If the child wants to validate the parsing (e.g. make sure that
   there are hosts in the arguments), or if it wants to check the
   options it added in its __init__(), it should implement a parse()
   method.

   The child parser must call its parent parser and gets back the
   options dictionary and the rest of the command line arguments
   (leftover). Each level gets to see all the options, but the
   leftovers can be deleted as they can be consumed by only one
   object.

3. Execution
   This execute() method is specific to the child and should use the
   self.execute_rpc() to send commands to the Autotest Front-End.  It
   should return results.

4. Output
   The child output() method is called with the execute() resutls as a
   parameter.  This is child-specific, but should leverage the
   atest.print_*() methods.
"""

import optparse
import os
import re
import sys
import textwrap
import traceback
import urllib2

from autotest_lib.cli import rpc
from autotest_lib.client.common_lib.test_utils import mock


# Maps the AFE keys to printable names.
KEYS_TO_NAMES_EN = {'hostname': 'Host',
                    'platform': 'Platform',
                    'status': 'Status',
                    'locked': 'Locked',
                    'locked_by': 'Locked by',
                    'lock_time': 'Locked time',
                    'lock_reason': 'Lock Reason',
                    'labels': 'Labels',
                    'description': 'Description',
                    'hosts': 'Hosts',
                    'users': 'Users',
                    'id': 'Id',
                    'name': 'Name',
                    'invalid': 'Valid',
                    'login': 'Login',
                    'access_level': 'Access Level',
                    'job_id': 'Job Id',
                    'job_owner': 'Job Owner',
                    'job_name': 'Job Name',
                    'test_type': 'Test Type',
                    'test_class': 'Test Class',
                    'path': 'Path',
                    'owner': 'Owner',
                    'status_counts': 'Status Counts',
                    'hosts_status': 'Host Status',
                    'hosts_selected_status': 'Hosts filtered by Status',
                    'priority': 'Priority',
                    'control_type': 'Control Type',
                    'created_on': 'Created On',
                    'synch_type': 'Synch Type',
                    'control_file': 'Control File',
                    'only_if_needed': 'Use only if needed',
                    'protection': 'Protection',
                    'run_verify': 'Run verify',
                    'reboot_before': 'Pre-job reboot',
                    'reboot_after': 'Post-job reboot',
                    'experimental': 'Experimental',
                    'synch_count': 'Sync Count',
                    'max_number_of_machines': 'Max. hosts to use',
                    'parse_failed_repair': 'Include failed repair results',
                    'atomic_group.name': 'Atomic Group Name',
                    'shard': 'Shard',
                    }

# In the failure, tag that will replace the item.
FAIL_TAG = '<XYZ>'

# Global socket timeout: uploading kernels can take much,
# much longer than the default
UPLOAD_SOCKET_TIMEOUT = 60*30


# Convertion functions to be called for printing,
# e.g. to print True/False for booleans.
def __convert_platform(field):
    if field is None:
        return ""
    elif isinstance(field, int):
        # Can be 0/1 for False/True
        return str(bool(field))
    else:
        # Can be a platform name
        return field


def _int_2_bool_string(value):
    return str(bool(value))

KEYS_CONVERT = {'locked': _int_2_bool_string,
                'invalid': lambda flag: str(bool(not flag)),
                'only_if_needed': _int_2_bool_string,
                'platform': __convert_platform,
                'labels': lambda labels: ', '.join(labels),
                'shards': lambda shard: shard.hostname if shard else ''}


def _get_item_key(item, key):
    """Allow for lookups in nested dictionaries using '.'s within a key."""
    if key in item:
        return item[key]
    nested_item = item
    for subkey in key.split('.'):
        if not subkey:
            raise ValueError('empty subkey in %r' % key)
        try:
            nested_item = nested_item[subkey]
        except KeyError, e:
            raise KeyError('%r - looking up key %r in %r' %
                           (e, key, nested_item))
    else:
        return nested_item


class CliError(Exception):
    """Error raised by cli calls.
    """
    pass


class item_parse_info(object):
    """Object keeping track of the parsing options.
    """

    def __init__(self, attribute_name, inline_option='',
                 filename_option='', use_leftover=False):
        """Object keeping track of the parsing options that will
        make up the content of the atest attribute:
        attribute_name: the atest attribute name to populate    (label)
        inline_option: the option containing the items           (--label)
        filename_option: the option containing the filename      (--blist)
        use_leftover: whether to add the leftover arguments or not."""
        self.attribute_name = attribute_name
        self.filename_option = filename_option
        self.inline_option = inline_option
        self.use_leftover = use_leftover


    def get_values(self, options, leftover=[]):
        """Returns the value for that attribute by accumualting all
        the values found through the inline option, the parsing of the
        file and the leftover"""

        def __get_items(input, split_spaces=True):
            """Splits a string of comma separated items. Escaped commas will not
            be split. I.e. Splitting 'a, b\,c, d' will yield ['a', 'b,c', 'd'].
            If split_spaces is set to False spaces will not be split. I.e.
            Splitting 'a b, c\,d, e' will yield ['a b', 'c,d', 'e']"""

            # Replace escaped slashes with null characters so we don't misparse
            # proceeding commas.
            input = input.replace(r'\\', '\0')

            # Split on commas which are not preceded by a slash.
            if not split_spaces:
                split = re.split(r'(?<!\\),', input)
            else:
                split = re.split(r'(?<!\\),|\s', input)

            # Convert null characters to single slashes and escaped commas to
            # just plain commas.
            return (item.strip().replace('\0', '\\').replace(r'\,', ',') for
                    item in split if item.strip())

        if self.use_leftover:
            add_on = leftover
            leftover = []
        else:
            add_on = []

        # Start with the add_on
        result = set()
        for items in add_on:
            # Don't split on space here because the add-on
            # may have some spaces (like the job name)
            result.update(__get_items(items, split_spaces=False))

        # Process the inline_option, if any
        try:
            items = getattr(options, self.inline_option)
            result.update(__get_items(items))
        except (AttributeError, TypeError):
            pass

        # Process the file list, if any and not empty
        # The file can contain space and/or comma separated items
        try:
            flist = getattr(options, self.filename_option)
            file_content = []
            for line in open(flist).readlines():
                file_content += __get_items(line)
            if len(file_content) == 0:
                raise CliError("Empty file %s" % flist)
            result.update(file_content)
        except (AttributeError, TypeError):
            pass
        except IOError:
            raise CliError("Could not open file %s" % flist)

        return list(result), leftover


class atest(object):
    """Common class for generic processing
    Should only be instantiated by itself for usage
    references, otherwise, the <topic> objects should
    be used."""
    msg_topic = ('[acl|host|job|label|shard|atomicgroup|test|user|server|'
                 'stable_version]')
    usage_action = '[action]'
    msg_items = ''

    def invalid_arg(self, header, follow_up=''):
        """Fail the command with error that command line has invalid argument.

        @param header: Header of the error message.
        @param follow_up: Extra error message, default to empty string.
        """
        twrap = textwrap.TextWrapper(initial_indent='        ',
                                     subsequent_indent='       ')
        rest = twrap.fill(follow_up)

        if self.kill_on_failure:
            self.invalid_syntax(header + rest)
        else:
            print >> sys.stderr, header + rest


    def invalid_syntax(self, msg):
        """Fail the command with error that the command line syntax is wrong.

        @param msg: Error message.
        """
        print
        print >> sys.stderr, msg
        print
        print "usage:",
        print self._get_usage()
        print
        sys.exit(1)


    def generic_error(self, msg):
        """Fail the command with a generic error.

        @param msg: Error message.
        """
        if self.debug:
            traceback.print_exc()
        print >> sys.stderr, msg
        sys.exit(1)


    def parse_json_exception(self, full_error):
        """Parses the JSON exception to extract the bad
        items and returns them
        This is very kludgy for the moment, but we would need
        to refactor the exceptions sent from the front end
        to make this better.

        @param full_error: The complete error message.
        """
        errmsg = str(full_error).split('Traceback')[0].rstrip('\n')
        parts = errmsg.split(':')
        # Kludge: If there are 2 colons the last parts contains
        # the items that failed.
        if len(parts) != 3:
            return []
        return [item.strip() for item in parts[2].split(',') if item.strip()]


    def failure(self, full_error, item=None, what_failed='', fatal=False):
        """If kill_on_failure, print this error and die,
        otherwise, queue the error and accumulate all the items
        that triggered the same error.

        @param full_error: The complete error message.
        @param item: Name of the actionable item, e.g., hostname.
        @param what_failed: Name of the failed item.
        @param fatal: True to exit the program with failure.
        """

        if self.debug:
            errmsg = str(full_error)
        else:
            errmsg = str(full_error).split('Traceback')[0].rstrip('\n')

        if self.kill_on_failure or fatal:
            print >> sys.stderr, "%s\n    %s" % (what_failed, errmsg)
            sys.exit(1)

        # Build a dictionary with the 'what_failed' as keys.  The
        # values are dictionaries with the errmsg as keys and a set
        # of items as values.
        # self.failed =
        # {'Operation delete_host_failed': {'AclAccessViolation:
        #                                        set('host0', 'host1')}}
        # Try to gather all the same error messages together,
        # even if they contain the 'item'
        if item and item in errmsg:
            errmsg = errmsg.replace(item, FAIL_TAG)
        if self.failed.has_key(what_failed):
            self.failed[what_failed].setdefault(errmsg, set()).add(item)
        else:
            self.failed[what_failed] = {errmsg: set([item])}


    def show_all_failures(self):
        """Print all failure information.
        """
        if not self.failed:
            return 0
        for what_failed in self.failed.keys():
            print >> sys.stderr, what_failed + ':'
            for (errmsg, items) in self.failed[what_failed].iteritems():
                if len(items) == 0:
                    print >> sys.stderr, errmsg
                elif items == set(['']):
                    print >> sys.stderr, '    ' + errmsg
                elif len(items) == 1:
                    # Restore the only item
                    if FAIL_TAG in errmsg:
                        errmsg = errmsg.replace(FAIL_TAG, items.pop())
                    else:
                        errmsg = '%s (%s)' % (errmsg, items.pop())
                    print >> sys.stderr, '    ' + errmsg
                else:
                    print >> sys.stderr, '    ' + errmsg + ' with <XYZ> in:'
                    twrap = textwrap.TextWrapper(initial_indent='        ',
                                                 subsequent_indent='        ')
                    items = list(items)
                    items.sort()
                    print >> sys.stderr, twrap.fill(', '.join(items))
        return 1


    def __init__(self):
        """Setup the parser common options"""
        # Initialized for unit tests.
        self.afe = None
        self.failed = {}
        self.data = {}
        self.debug = False
        self.parse_delim = '|'
        self.kill_on_failure = False
        self.web_server = ''
        self.verbose = False
        self.no_confirmation = False
        self.topic_parse_info = item_parse_info(attribute_name='not_used')

        self.parser = optparse.OptionParser(self._get_usage())
        self.parser.add_option('-g', '--debug',
                               help='Print debugging information',
                               action='store_true', default=False)
        self.parser.add_option('--kill-on-failure',
                               help='Stop at the first failure',
                               action='store_true', default=False)
        self.parser.add_option('--parse',
                               help='Print the output using | '
                               'separated key=value fields',
                               action='store_true', default=False)
        self.parser.add_option('--parse-delim',
                               help='Delimiter to use to separate the '
                               'key=value fields', default='|')
        self.parser.add_option('--no-confirmation',
                               help=('Skip all confirmation in when function '
                                     'require_confirmation is called.'),
                               action='store_true', default=False)
        self.parser.add_option('-v', '--verbose',
                               action='store_true', default=False)
        self.parser.add_option('-w', '--web',
                               help='Specify the autotest server '
                               'to talk to',
                               action='store', type='string',
                               dest='web_server', default=None)


    def _get_usage(self):
        return "atest %s %s [options] %s" % (self.msg_topic.lower(),
                                             self.usage_action,
                                             self.msg_items)


    def backward_compatibility(self, action, argv):
        """To be overidden by subclass if their syntax changed.

        @param action: Name of the action.
        @param argv: A list of arguments.
        """
        return action


    def parse(self, parse_info=[], req_items=None):
        """Parse command arguments.

        parse_info is a list of item_parse_info objects.
        There should only be one use_leftover set to True in the list.

        Also check that the req_items is not empty after parsing.

        @param parse_info: A list of item_parse_info objects.
        @param req_items: A list of required items.
        """
        (options, leftover) = self.parse_global()

        all_parse_info = parse_info[:]
        all_parse_info.append(self.topic_parse_info)

        try:
            for item_parse_info in all_parse_info:
                values, leftover = item_parse_info.get_values(options,
                                                              leftover)
                setattr(self, item_parse_info.attribute_name, values)
        except CliError, s:
            self.invalid_syntax(s)

        if (req_items and not getattr(self, req_items, None)):
            self.invalid_syntax('%s %s requires at least one %s' %
                                (self.msg_topic,
                                 self.usage_action,
                                 self.msg_topic))

        return (options, leftover)


    def parse_global(self):
        """Parse the global arguments.

        It consumes what the common object needs to know, and
        let the children look at all the options.  We could
        remove the options that we have used, but there is no
        harm in leaving them, and the children may need them
        in the future.

        Must be called from its children parse()"""
        (options, leftover) = self.parser.parse_args()
        # Handle our own options setup in __init__()
        self.debug = options.debug
        self.kill_on_failure = options.kill_on_failure

        if options.parse:
            suffix = '_parse'
        else:
            suffix = '_std'
        for func in ['print_fields', 'print_table',
                     'print_by_ids', 'print_list']:
            setattr(self, func, getattr(self, func + suffix))

        self.parse_delim = options.parse_delim

        self.verbose = options.verbose
        self.no_confirmation = options.no_confirmation
        self.web_server = options.web_server
        try:
            self.afe = rpc.afe_comm(self.web_server)
        except rpc.AuthError, s:
            self.failure(str(s), fatal=True)

        return (options, leftover)


    def check_and_create_items(self, op_get, op_create,
                                items, **data_create):
        """Create the items if they don't exist already.

        @param op_get: Name of `get` RPC.
        @param op_create: Name of `create` RPC.
        @param items: Actionable items specified in CLI command, e.g., hostname,
                      to be passed to each RPC.
        @param data_create: Data to be passed to `create` RPC.
        """
        for item in items:
            ret = self.execute_rpc(op_get, name=item)

            if len(ret) == 0:
                try:
                    data_create['name'] = item
                    self.execute_rpc(op_create, **data_create)
                except CliError:
                    continue


    def execute_rpc(self, op, item='', **data):
        """Execute RPC.

        @param op: Name of the RPC.
        @param item: Actionable item specified in CLI command.
        @param data: Data to be passed to RPC.
        """
        retry = 2
        while retry:
            try:
                return self.afe.run(op, **data)
            except urllib2.URLError, err:
                if hasattr(err, 'reason'):
                    if 'timed out' not in err.reason:
                        self.invalid_syntax('Invalid server name %s: %s' %
                                            (self.afe.web_server, err))
                if hasattr(err, 'code'):
                    error_parts = [str(err)]
                    if self.debug:
                        error_parts.append(err.read()) # read the response body
                    self.failure('\n\n'.join(error_parts), item=item,
                                 what_failed=("Error received from web server"))
                    raise CliError("Error from web server")
                if self.debug:
                    print 'retrying: %r %d' % (data, retry)
                retry -= 1
                if retry == 0:
                    if item:
                        myerr = '%s timed out for %s' % (op, item)
                    else:
                        myerr = '%s timed out' % op
                    self.failure(myerr, item=item,
                                 what_failed=("Timed-out contacting "
                                              "the Autotest server"))
                    raise CliError("Timed-out contacting the Autotest server")
            except mock.CheckPlaybackError:
                raise
            except Exception, full_error:
                # There are various exceptions throwns by JSON,
                # urllib & httplib, so catch them all.
                self.failure(full_error, item=item,
                             what_failed='Operation %s failed' % op)
                raise CliError(str(full_error))


    # There is no output() method in the atest object (yet?)
    # but here are some helper functions to be used by its
    # children
    def print_wrapped(self, msg, values):
        """Print given message and values in wrapped lines unless
        AUTOTEST_CLI_NO_WRAP is specified in environment variables.

        @param msg: Message to print.
        @param values: A list of values to print.
        """
        if len(values) == 0:
            return
        elif len(values) == 1:
            print msg + ': '
        elif len(values) > 1:
            if msg.endswith('s'):
                print msg + ': '
            else:
                print msg + 's: '

        values.sort()

        if 'AUTOTEST_CLI_NO_WRAP' in os.environ:
            print '\n'.join(values)
            return

        twrap = textwrap.TextWrapper(initial_indent='\t',
                                     subsequent_indent='\t')
        print twrap.fill(', '.join(values))


    def __conv_value(self, type, value):
        return KEYS_CONVERT.get(type, str)(value)


    def print_fields_std(self, items, keys, title=None):
        """Print the keys in each item, one on each line.

        @param items: Items to print.
        @param keys: Name of the keys to look up each item in items.
        @param title: Title of the output, default to None.
        """
        if not items:
            return
        if title:
            print title
        for item in items:
            for key in keys:
                print '%s: %s' % (KEYS_TO_NAMES_EN[key],
                                  self.__conv_value(key,
                                                    _get_item_key(item, key)))


    def print_fields_parse(self, items, keys, title=None):
        """Print the keys in each item as comma separated name=value

        @param items: Items to print.
        @param keys: Name of the keys to look up each item in items.
        @param title: Title of the output, default to None.
        """
        for item in items:
            values = ['%s=%s' % (KEYS_TO_NAMES_EN[key],
                                  self.__conv_value(key,
                                                    _get_item_key(item, key)))
                      for key in keys
                      if self.__conv_value(key,
                                           _get_item_key(item, key)) != '']
            print self.parse_delim.join(values)


    def __find_justified_fmt(self, items, keys):
        """Find the max length for each field.

        @param items: Items to lookup for.
        @param keys: Name of the keys to look up each item in items.
        """
        lens = {}
        # Don't justify the last field, otherwise we have blank
        # lines when the max is overlaps but the current values
        # are smaller
        if not items:
            print "No results"
            return
        for key in keys[:-1]:
            lens[key] = max(len(self.__conv_value(key,
                                                  _get_item_key(item, key)))
                            for item in items)
            lens[key] = max(lens[key], len(KEYS_TO_NAMES_EN[key]))
        lens[keys[-1]] = 0

        return '  '.join(["%%-%ds" % lens[key] for key in keys])


    def print_dict(self, items, title=None, line_before=False):
        """Print a dictionary.

        @param items: Dictionary to print.
        @param title: Title of the output, default to None.
        @param line_before: True to print an empty line before the output,
                            default to False.
        """
        if not items:
            return
        if line_before:
            print
        print title
        for key, value in items.items():
            print '%s : %s' % (key, value)


    def print_table_std(self, items, keys_header, sublist_keys=()):
        """Print a mix of header and lists in a user readable format.

        The headers are justified, the sublist_keys are wrapped.

        @param items: Items to print.
        @param keys_header: Header of the keys, use to look up in items.
        @param sublist_keys: Keys for sublist in each item.
        """
        if not items:
            return
        fmt = self.__find_justified_fmt(items, keys_header)
        header = tuple(KEYS_TO_NAMES_EN[key] for key in keys_header)
        print fmt % header
        for item in items:
            values = tuple(self.__conv_value(key,
                                             _get_item_key(item, key))
                           for key in keys_header)
            print fmt % values
            if sublist_keys:
                for key in sublist_keys:
                    self.print_wrapped(KEYS_TO_NAMES_EN[key],
                                       _get_item_key(item, key))
                print '\n'


    def print_table_parse(self, items, keys_header, sublist_keys=()):
        """Print a mix of header and lists in a user readable format.

        @param items: Items to print.
        @param keys_header: Header of the keys, use to look up in items.
        @param sublist_keys: Keys for sublist in each item.
        """
        for item in items:
            values = ['%s=%s' % (KEYS_TO_NAMES_EN[key],
                                 self.__conv_value(key, _get_item_key(item, key)))
                      for key in keys_header
                      if self.__conv_value(key,
                                           _get_item_key(item, key)) != '']

            if sublist_keys:
                [values.append('%s=%s'% (KEYS_TO_NAMES_EN[key],
                                         ','.join(_get_item_key(item, key))))
                 for key in sublist_keys
                 if len(_get_item_key(item, key))]

            print self.parse_delim.join(values)


    def print_by_ids_std(self, items, title=None, line_before=False):
        """Prints ID & names of items in a user readable form.

        @param items: Items to print.
        @param title: Title of the output, default to None.
        @param line_before: True to print an empty line before the output,
                            default to False.
        """
        if not items:
            return
        if line_before:
            print
        if title:
            print title + ':'
        self.print_table_std(items, keys_header=['id', 'name'])


    def print_by_ids_parse(self, items, title=None, line_before=False):
        """Prints ID & names of items in a parseable format.

        @param items: Items to print.
        @param title: Title of the output, default to None.
        @param line_before: True to print an empty line before the output,
                            default to False.
        """
        if not items:
            return
        if line_before:
            print
        if title:
            print title + '=',
        values = []
        for item in items:
            values += ['%s=%s' % (KEYS_TO_NAMES_EN[key],
                                  self.__conv_value(key,
                                                    _get_item_key(item, key)))
                       for key in ['id', 'name']
                       if self.__conv_value(key,
                                            _get_item_key(item, key)) != '']
        print self.parse_delim.join(values)


    def print_list_std(self, items, key):
        """Print a wrapped list of results

        @param items: Items to to lookup for given key, could be a nested
                      dictionary.
        @param key: Name of the key to look up for value.
        """
        if not items:
            return
        print ' '.join(_get_item_key(item, key) for item in items)


    def print_list_parse(self, items, key):
        """Print a wrapped list of results.

        @param items: Items to to lookup for given key, could be a nested
                      dictionary.
        @param key: Name of the key to look up for value.
        """
        if not items:
            return
        print '%s=%s' % (KEYS_TO_NAMES_EN[key],
                         ','.join(_get_item_key(item, key) for item in items))


    @staticmethod
    def prompt_confirmation(message=None):
        """Prompt a question for user to confirm the action before proceeding.

        @param message: A detailed message to explain possible impact of the
                        action.

        @return: True to proceed or False to abort.
        """
        if message:
            print message
        sys.stdout.write('Continue? [y/N] ')
        read = raw_input().lower()
        if read == 'y':
            return True
        else:
            print 'User did not confirm. Aborting...'
            return False


    @staticmethod
    def require_confirmation(message=None):
        """Decorator to prompt a question for user to confirm action before
        proceeding.

        If user chooses not to proceed, do not call the function.

        @param message: A detailed message to explain possible impact of the
                        action.

        @return: A decorator wrapper for calling the actual function.
        """
        def deco_require_confirmation(func):
            """Wrapper for the decorator.

            @param func: Function to be called.

            @return: the actual decorator to call the function.
            """
            def func_require_confirmation(*args, **kwargs):
                """Decorator to prompt a question for user to confirm.

                @param message: A detailed message to explain possible impact of
                                the action.
                """
                if (args[0].no_confirmation or
                    atest.prompt_confirmation(message)):
                    func(*args, **kwargs)

            return func_require_confirmation
        return deco_require_confirmation
