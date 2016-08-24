import argparse
import ast
import logging
import os
import shlex
import sys


class autoserv_parser(object):
    """Custom command-line options parser for autoserv.

    We can't use the general getopt methods here, as there will be unknown
    extra arguments that we pass down into the control file instead.
    Thus we process the arguments by hand, for which we are duly repentant.
    Making a single function here just makes it harder to read. Suck it up.
    """
    def __init__(self):
        self.args = sys.argv[1:]
        self.parser = argparse.ArgumentParser(
                usage='%(prog)s [options] [control-file]')
        self.setup_options()

        # parse an empty list of arguments in order to set self.options
        # to default values so that codepaths that assume they are always
        # reached from an autoserv process (when they actually are not)
        # will still work
        self.options = self.parser.parse_args(args=[])


    def setup_options(self):
        """Setup options to call autoserv command.
        """
        self.parser.add_argument('-m', action='store', type=str,
                                 dest='machines',
                                 help='list of machines')
        self.parser.add_argument('-M', action='store', type=str,
                                 dest='machines_file',
                                 help='list of machines from file')
        self.parser.add_argument('-c', action='store_true',
                                 dest='client', default=False,
                                 help='control file is client side')
        self.parser.add_argument('-s', action='store_true',
                                 dest='server', default=False,
                                 help='control file is server side')
        self.parser.add_argument('-r', action='store', type=str,
                                 dest='results', default=None,
                                 help='specify results directory')
        self.parser.add_argument('-l', action='store', type=str,
                                 dest='label', default='',
                                 help='label for the job')
        self.parser.add_argument('-G', action='store', type=str,
                                 dest='group_name', default='',
                                 help='The host_group_name to store in keyvals')
        self.parser.add_argument('-u', action='store', type=str,
                                 dest='user',
                                 default=os.environ.get('USER'),
                                 help='username for the job')
        self.parser.add_argument('-P', action='store', type=str,
                                 dest='parse_job',
                                 default='',
                                 help=('Parse the results of the job using this'
                                       ' execution tag. Accessible in control '
                                       'files as job.tag.'))
        self.parser.add_argument('--execution-tag', action='store',
                                 type=str, dest='execution_tag',
                                 default='',
                                 help=('Accessible in control files as job.tag;'
                                       ' Defaults to the value passed to -P.'))
        self.parser.add_argument('-i', action='store_true',
                                 dest='install_before', default=False,
                                 help=('reinstall machines before running the '
                                       'job'))
        self.parser.add_argument('-I', action='store_true',
                                 dest='install_after', default=False,
                                 help=('reinstall machines after running the '
                                       'job'))
        self.parser.add_argument('-v', action='store_true',
                                 dest='verify', default=False,
                                 help='verify the machines only')
        self.parser.add_argument('-R', action='store_true',
                                 dest='repair', default=False,
                                 help='repair the machines')
        self.parser.add_argument('-C', '--cleanup', action='store_true',
                                 default=False,
                                 help='cleanup all machines after the job')
        self.parser.add_argument('--provision', action='store_true',
                                 default=False,
                                 help='Provision the machine.')
        self.parser.add_argument('--job-labels', action='store',
                                 help='Comma seperated job labels.')
        self.parser.add_argument('-T', '--reset', action='store_true',
                                 default=False,
                                 help=('Reset (cleanup and verify) all machines'
                                       ' after the job'))
        self.parser.add_argument('-n', action='store_true',
                                 dest='no_tee', default=False,
                                 help='no teeing the status to stdout/err')
        self.parser.add_argument('-N', action='store_true',
                                 dest='no_logging', default=False,
                                 help='no logging')
        self.parser.add_argument('--verbose', action='store_true',
                                 help=('Include DEBUG messages in console '
                                       'output'))
        self.parser.add_argument('--no_console_prefix', action='store_true',
                                 help=('Disable the logging prefix on console '
                                       'output'))
        self.parser.add_argument('-p', '--write-pidfile', action='store_true',
                                 dest='write_pidfile', default=False,
                                 help=('write pidfile (pidfile name is '
                                       'determined by --pidfile-label'))
        self.parser.add_argument('--pidfile-label', action='store',
                                 default='autoserv',
                                 help=('Determines filename to use as pidfile '
                                       '(if -p is specified). Pidfile will be '
                                       '.<label>_execute. Default to '
                                       'autoserv.'))
        self.parser.add_argument('--use-existing-results', action='store_true',
                                 help=('Indicates that autoserv is working with'
                                       ' an existing results directory'))
        self.parser.add_argument('-a', '--args', dest='args',
                                 help='additional args to pass to control file')
        self.parser.add_argument('--ssh-user', action='store',
                                 type=str, dest='ssh_user', default='root',
                                 help='specify the user for ssh connections')
        self.parser.add_argument('--ssh-port', action='store',
                                 type=int, dest='ssh_port', default=22,
                                 help=('specify the port to use for ssh '
                                       'connections'))
        self.parser.add_argument('--ssh-pass', action='store',
                                 type=str, dest='ssh_pass',
                                 default='',
                                 help=('specify the password to use for ssh '
                                       'connections'))
        self.parser.add_argument('--install-in-tmpdir', action='store_true',
                                 dest='install_in_tmpdir', default=False,
                                 help=('by default install autotest clients in '
                                       'a temporary directory'))
        self.parser.add_argument('--collect-crashinfo', action='store_true',
                                 dest='collect_crashinfo', default=False,
                                 help='just run crashinfo collection')
        self.parser.add_argument('--control-filename', action='store',
                                 type=str, default=None,
                                 help=('filename to use for the server control '
                                       'file in the results directory'))
        self.parser.add_argument('--test-retry', action='store',
                                 type=int, default=0,
                                 help=('Num of times to retry a test that '
                                       'failed [default: %(default)d]'))
        self.parser.add_argument('--verify_job_repo_url', action='store_true',
                                 dest='verify_job_repo_url', default=False,
                                 help=('Verify that the job_repo_url of the '
                                       'host has staged packages for the job.'))
        self.parser.add_argument('--no_collect_crashinfo', action='store_true',
                                 dest='skip_crash_collection', default=False,
                                 help=('Turns off crash collection to shave '
                                       'time off test runs.'))
        self.parser.add_argument('--disable_sysinfo', action='store_true',
                                 dest='disable_sysinfo', default=False,
                                 help=('Turns off sysinfo collection to shave '
                                       'time off test runs.'))
        self.parser.add_argument('--ssh_verbosity', action='store',
                                 dest='ssh_verbosity', default=0,
                                 type=str, choices=['0', '1', '2', '3'],
                                 help=('Verbosity level for ssh, between 0 '
                                       'and 3 inclusive. '
                                       '[default: %(default)s]'))
        self.parser.add_argument('--ssh_options', action='store',
                                 dest='ssh_options', default='',
                                 help=('A string giving command line flags '
                                       'that will be included in ssh commands'))
        self.parser.add_argument('--require-ssp', action='store_true',
                                 dest='require_ssp', default=False,
                                 help=('Force the autoserv process to run with '
                                       'server-side packaging'))
        self.parser.add_argument('--warn-no-ssp', action='store_true',
                                 dest='warn_no_ssp', default=False,
                                 help=('Post a warning in autoserv log that '
                                       'the process runs in a drone without '
                                       'server-side packaging support, even '
                                       'though the job requires server-side '
                                       'packaging'))
        self.parser.add_argument('--no_use_packaging', action='store_true',
                                 dest='no_use_packaging', default=False,
                                 help=('Disable install modes that use the '
                                       'packaging system.'))
        self.parser.add_argument('--test_source_build', action='store',
                                 type=str, default='',
                                 dest='test_source_build',
                                 help=('Name of the build that contains the '
                                       'test code. Default is empty, that is, '
                                       'use the build specified in --image to '
                                       'retrieve tests.'))
        self.parser.add_argument('--parent_job_id', action='store',
                                 type=str, default=None,
                                 dest='parent_job_id',
                                 help=('ID of the parent job. Default to None '
                                       'if the job does not have a parent job'))
        self.parser.add_argument('--image', action='store', type=str,
                               default='', dest='image',
                               help=('Full path of an OS image to install, e.g.'
                                     ' http://devserver/update/alex-release/'
                                     'R27-3837.0.0 or a build name: '
                                     'x86-alex-release/R27-3837.0.0 to '
                                     'utilize lab devservers automatically.'))
        self.parser.add_argument('--host_attributes', action='store',
                                 dest='host_attributes', default='{}',
                                 help=('Host attribute to be applied to all '
                                       'machines/hosts for this autoserv run. '
                                       'Must be a string-encoded dict. '
                                       'Example: {"key1":"value1", "key2":'
                                       '"value2"}'))
        self.parser.add_argument('--lab', action='store', type=str,
                                 dest='lab', default='',
                                 help=argparse.SUPPRESS)
        #
        # Warning! Please read before adding any new arguments!
        #
        # New arguments will be ignored if a test runs with server-side
        # packaging and if the test source build does not have the new
        # arguments.
        #
        # New argument should NOT set action to `store_true`. A workaround is to
        # use string value of `True` or `False`, then convert them to boolean in
        # code.
        # The reason is that parse_args will always ignore the argument name and
        # value. An unknown argument without a value will lead to positional
        # argument being removed unexpectedly.
        #


    def parse_args(self):
        """Parse and process command line arguments.
        """
        # Positional arguments from the end of the command line will be included
        # in the list of unknown_args.
        self.options, unknown_args = self.parser.parse_known_args()
        # Filter out none-positional arguments
        removed_args = []
        while unknown_args and unknown_args[0][0] == '-':
            removed_args.append(unknown_args.pop(0))
            # Always assume the argument has a value.
            if unknown_args:
                removed_args.append(unknown_args.pop(0))
        if removed_args:
            logging.warn('Unknown arguments are removed from the options: %s',
                         removed_args)

        self.args = unknown_args + shlex.split(self.options.args or '')

        if self.options.image:
            self.options.install_before = True
            self.options.image =  self.options.image.strip()
        self.options.host_attributes = ast.literal_eval(
                self.options.host_attributes)


# create the one and only one instance of autoserv_parser
autoserv_parser = autoserv_parser()
