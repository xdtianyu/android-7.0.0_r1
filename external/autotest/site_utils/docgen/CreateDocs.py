#!/usr/bin/python
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

""" Parse suite control files and make HTML documentation from included tests.

This program will create a list of test cases found in suite files by parsing
through each suite control file and making a list of all of the jobs called from
it. Once it has a list of tests, it will parse the AutoTest control file for
each test and grab the doc strings. These doc strings, along with any
constraints in the suite control file, will be added to the original test
script. These new scripts will be placed in a stand alone directory. Doxygen
will then use these files for the sole purpose of producing HTML documentation
for all of the tests. Once HTML docs are created some post processing will be
done against the docs to change a few strings.

If this script is executed without a --src argument, it will assume it is being
executed from <ChromeOS>/src/third_party/autotest/files/utils/docgen/ directory.

Classes:

  DocCreator
    This class is responsible for all processing. It requires the following:
      - Absolute path of suite control files.
      - Absolute path of where to place temporary files it constructs from the
        control files and test scripts.
    This class makes the following assumptions:
      - Each master suite has a README.txt file with general instructions on
        test preparation and usage.
      - The control file for each test has doc strings with labels of:
        - PURPOSE: one line description of why this test exists.
        - CRITERIA: Pass/Failure conditions.
        - DOC: additional test details.
  ReadNode
    This class parses a node from a control file into a key/value pair. In this
    context, a node represents a syntactic construct of an abstract syntax tree.
    The root of the tree is the module object (in this case a control file). If
    suite=True, it will assume the node is from a suite control file.

Doxygen should already be configured with a configuration file called:
doxygen.conf. This file should live in the same directory with this program.
If you haven't installed doxygen, you'll need to install this program before
this script is executed. This program will automatically update the doxygen.conf
file to match self.src_tests and self.html.

TODO: (kdlucas@google.com) Update ReadNode class to use the replacement module
for the compiler module, as that has been deprecated.
"""

__author__ = 'kdlucas@google.com (Kelly Lucas)'
__version__ = '0.9.1'

import compiler
import fileinput
import glob
import logging
import optparse
import os
import shutil
import subprocess
import sys

import fs_find_tests


class DocCreator(object):
    """Process suite control files to combine docstrings and create HTML docs.

    The DocCreator class is designed to parse AutoTest suite control files to
    find all of the tests referenced, and build HTML documentation based on the
    docstrings in those files. It will cross reference the test control file
    and any parameters passed through the suite file, with the original test
    case. DocCreator relies on doxygen to actually generate the HTML documents.

    The workflow is as follows:
        - Parse the suite file(s) and generate a test list.
        - Locate the test source, and grab the docstrings from the associated
          AutoTest control file.
        - Combine the docstring from the control file with any parameters passed
          in from the suite control file, with the original test case.
        - Write a new test file with the combined docstrings to src_tests.
        - Create HTML documentation by running doxygen against the tests stored
          in self.src_tests.

    Implements the following methods:
        - GetTests() - Parse suite control files, create a dictionary of tests.
        - ParseControlFiles() - Runs through all tests and parses control files
        - _CleanDir() - Remove any files in a direcory and create an empty one.
        - _GetDoctString() - Parses docstrings and joins it with constraints.
        - _CreateTest() - Add docstrings and constraints to existing test script
          to form a new test script.
        - CreateMainPage() - Create a mainpage.txt file based on contents of the
          suite README file.
        - _ConfigDoxygen - Updates doxygen.conf to match some attributes this
          script was run with.
        - RunDoxygen() - Executes the doxygen program.
        - CleanDocs() - Changes some text in the HTML files to conform to our
          naming conventions and style.

    Depends upon class ReadNode.
    """
    def __init__(self, options, args, logger):
        """Parse command line arguments and set some initial variables."""

        self.options = options
        self.args = args
        self.logger = logger

        # Make parameters a little shorter by making the following assignments.
        if options.all_tests:
            self.suite = 'suite_All'
        else:
            self.suite = self.options.suite

        self.autotest_root = self.options.autotest_dir
        self.debug = self.options.debug
        self.docversion = self.options.docversion
        self.doxyconf = self.options.doxyconf
        self.html = '%s_%s' % (self.suite, self.options.html)
        self.latex = self.options.latex
        self.layout = self.options.layout
        self.logfile = self.options.logfile
        self.readme = self.options.readme
        self.src_tests = '%s_%s' % (self.suite, self.options.src_tests)

        self.testcase = {}
        self.testcase_src = {}

        self.site_dir = os.path.join(self.autotest_root, 'client', 'site_tests')
        self.test_dir = os.path.join(self.autotest_root, 'client', 'tests')
        self.suite_dir = os.path.join(self.site_dir, self.suite)

        self.logger.debug('Executing with debug level: %s', self.debug)
        self.logger.debug('Writing to logfile: %s', self.logfile)
        self.logger.debug('New test directory: %s', self.src_tests)
        self.logger.debug('Test suite: %s', self.suite)

        self.suitename = {
                          'suite_All': 'All Existing Autotest Tests',
                          'suite_Factory': 'Factory Testing',
                          'suite_HWConfig': 'Hardware Configuration',
                          'suite_HWQual': 'Hardware Qualification',
                         }

    def GetAllTests(self):
        """Create list of all discovered tests."""
        for path in [ 'server/tests', 'server/site_tests', 'client/tests',
                      'client/site_tests']:
            test_path = os.path.join(self.autotest_root, path)
            if not os.path.exists(test_path):
                continue
            self.logger.info("Scanning %s", test_path)
            tests, tests_src = fs_find_tests.GetTestsFromFS(test_path,
                                                            self.logger)
            test_intersection = set(self.testcase) & set(tests)
            if test_intersection:
                self.logger.warning("Duplicates found: %s", test_intersection)
            self.testcase.update(tests)
            self.testcase_src.update(tests_src)

    def GetTestsFromSuite(self):
        """Create list of tests invoked by a suite."""

        suite_search = os.path.join(self.suite_dir, 'control.*')
        for suitefile in glob.glob(suite_search):
            self.logger.debug('Scanning %s for tests', suitefile)
            if os.path.isfile(suitefile):
                try:
                    suite = compiler.parseFile(suitefile)
                except SyntaxError, e:
                    self.logger.error('Error parsing (gettests): %s\n%s',
                                      suitefile, e)
                    raise SystemExit

            # Walk through each node found in the control file, which in our
            # case will be a call to a test. compiler.walk() will walk through
            # each component node, and call the appropriate function in class
            # ReadNode. The returned key should be a string, and the name of a
            # test. visitor.value should be any extra arguments found in the
            # suite file that are used with that test case.
            for n in suite.node.nodes:
                visitor = ReadNode(suite=True)
                compiler.walk(n, visitor)
                if len(visitor.key) > 1:
                    filtered_input = ''
                    # Lines in value should start with '  -' for bullet item.
                    if visitor.value:
                        lines = visitor.value.split('\n')
                        for line in lines:
                            if line.startswith('  -'):
                                filtered_input += line + '\n'
                    # A test could be called multiple times, so see if the key
                    # already exists, and if so append the new value.
                    if visitor.key in self.testcase:
                        s = self.testcase[visitor.key] + filtered_input
                        self.testcase[visitor.key] = s
                    else:
                        self.testcase[visitor.key] = filtered_input

    def GetTests(self):
        """Create dictionary of tests based on suite control file contents."""
        if self.options.all_tests:
            self.GetAllTests()
        else:
            self.GetTestsFromSuite()

    def _CleanDir(self, directory):
        """Ensure the directory is available and empty.

        Args:
            directory: string, path of directory
        """

        if os.path.isdir(directory):
            try:
                shutil.rmtree(directory)
            except IOError, err:
                self.logger.error('Error cleaning %s\n%s', directory, err)
        try:
            os.makedirs(directory)
        except IOError, err:
            self.logger.error('Error creating %s\n%s', directory, err)
            self.logger.error('Check your permissions of %s', directory)
            raise SystemExit

    def LocateTest(self, test_name):
        """Determine the full path location of the test."""
        if test_name in self.testcase_src:
            return os.path.join(self.testcase_src[test_name], test_name)

        test_dir = os.path.join(self.site_dir, test_name)
        if not os.path.isdir(test_dir):
            test_dir = os.path.join(self.test_dir, test_name)
        if os.path.isdir(test_dir):
            return test_dir

        self.logger.warning('Cannot find test: %s', test)
        return None


    def ParseControlFiles(self):
        """Get docstrings from control files and add them to new test scripts.

        This method will cycle through all of the tests and attempt to find
        their control file. If found, it will parse the docstring from the
        control file, add this to any parameters found in the suite file, and
        add this combined docstring to the original test. These new tests will
        be written in the self.src_tests directory.
        """
        # Clean some target directories.
        for d in [self.src_tests, self.html]:
            self._CleanDir(d)

        for test in self.testcase:
            test_dir = self.LocateTest(test)
            if test_dir:
                control_file = os.path.join(test_dir, 'control')
                test_file = os.path.join(test_dir, test + '.py')
                docstring = self._GetDocString(control_file, test)
                self._CreateTest(test_file, docstring, test)

    def _GetDocString(self, control_file, test):
        """Get the docstrings from control file and join to suite file params.

        Args:
            control_file: string, absolute path to test control file.
            test: string, name of test.
        Returns:
            string: combined docstring with needed markup language for doxygen.
        """

        # Doxygen needs the @package marker.
        package_doc = '## @package '
        # To allow doxygen to use special commands, we must use # for comments.
        comment = '# '
        endlist = '  .\n'
        control_dict = {}
        output = []
        temp = []
        tempstring = ''
        docstring = ''
        keys = ['\\brief\n', '<H3>Pass/Fail Criteria:</H3>\n',
                '<H3>Author</H3>\n', '<H3>Test Duration</H3>\n',
                '<H3>Category</H3>\n', '<H3>Test Type</H3>\n',
                '<H3>Test Class</H3>\n', '<H3>Notest</H3>\n',
               ]

        if not os.path.isfile(control_file):
            self.logger.error('Cannot find: %s', control_file)
            return None
        try:
            control = compiler.parseFile(control_file)
        except SyntaxError, e:
            self.logger.error('Error parsing (docstring): %s\n%s',
                              control_file, e)
            return None

        for n in control.node.nodes:
            visitor = ReadNode()
            compiler.walk(n, visitor)
            control_dict[visitor.key] = visitor.value

        for k in keys:
            if k in control_dict:
                if len(control_dict[k]) > 1:
                    if k != test:
                        temp.append(k)
                    temp.append(control_dict[k])
                    if control_dict[k]:
                        temp.append(endlist)
                    # Add constraints and extra args after the Criteria section.
                    if 'Criteria:' in k:
                        if self.testcase[test]:
                            temp.append('<H3>Arguments:</H3>\n')
                            temp.append(self.testcase[test])
                            # '.' character at the same level as the '-' tells
                            # doxygen this is the end of the list.
                            temp.append(endlist)

        output.append(package_doc + test + '\n')
        tempstring = "".join(temp)
        lines = tempstring.split('\n')
        for line in lines:
            # Doxygen requires a '#' character to add special doxygen commands.
            comment_line = comment + line + '\n'
            output.append(comment_line)

        docstring = "".join(output)

        return docstring


    def _CreateTest(self, test_file, docstring, test):
        """Create a new test with the combined docstrings from multiple sources.

        Args:
            test_file: string, file name of new test to write.
            docstring: string, the docstring to add to the existing test.
            test: string, name of the test.

        This method is used to create a temporary copy of a new test, that will
        be a combination of the original test plus the docstrings from the
        control file, and any constraints from the suite control file.
        """

        class_def = 'class ' + test
        pathname = os.path.join(self.src_tests, test + '.py')

        # Open the test and write out new test with added docstrings
        try:
            f = open(test_file, 'r')
        except IOError, err:
            self.logger.error('Error while reading %s\n%s', test_file, err)
            return
        lines = f.readlines()
        f.close()

        try:
            f = open(pathname, 'w')
        except IOError, err:
            self.logger.error('Error creating %s\n%s', pathname, err)
            return

        for line in lines:
            if class_def in line and docstring:
                f.write(docstring)
                f.write('\n')
            f.write(line)
        f.close()

    def CreateMainPage(self, current_dir):
        """Create a main page to provide content for index.html.

        This method assumes a file named README.txt is located in your suite
        directory with general instructions on setting up and using the suite.
        If your README file is in another file, ensure you pass a --readme
        option with the correct filename. To produce a better looking
        landing page, use the '-' character for list items. This method assumes
        os commands start with '$'.
        """

        # Define some strings that Doxygen uses for specific formatting.
        cstart = '/**'
        cend = '**/'
        mp = '@mainpage'
        section_begin = '@section '
        vstart = '@verbatim '
        vend = ' @endverbatim\n'

        # Define some characters we expect to delineate sections in the README.
        sec_char = '=========='
        command_prompt = '$ '
        crosh_prompt = 'crosh>'
        command_cont = '\\'

        command = False
        comment = False
        section = False
        sec_ctr = 0

        if self.options.all_tests:
            readme_file = os.path.join(current_dir, self.readme)
        else:
            readme_file = os.path.join(self.suite_dir, self.readme)
        mainpage_file = os.path.join(self.src_tests, 'mainpage.txt')

        try:
            f = open(readme_file, 'r')
        except IOError, err:
            self.logger.error('Error opening %s\n%s', readme_file, err)
            return
        try:
            fw = open(mainpage_file, 'w')
        except IOError, err:
            self.logger.error('Error opening %s\n%s', mainpage_file, err)
            return

        lines = f.readlines()
        f.close()

        fw.write(cstart)
        fw.write('\n')
        fw.write(mp)
        fw.write('\n')

        for line in lines:
            if sec_char in line:
                comment = True
                section = not section
            elif section:
                sec_ctr += 1
                section_name = ' section%d ' % sec_ctr
                fw.write(section_begin + section_name + line)
            else:
                # comment is used to denote when we should start recording text
                # from the README file. Some of the initial text is not needed.
                if comment:
                    if command_prompt in line or crosh_prompt in line:
                        line = line.rstrip()
                        if line[-1] == command_cont:
                            fw.write(vstart + line[:-1])
                            command = True
                        else:
                            fw.write(vstart + line + vend)
                    elif command:
                        line = line.strip()
                        if line[-1] == command_cont:
                          fw.write(line)
                        else:
                          fw.write(line + vend)
                          command = False
                    else:
                        fw.write(line)

        fw.write('\n')
        fw.write(cend)
        fw.close()

    def _ConfigDoxygen(self):
        """Set Doxygen configuration to match our options."""

        doxy_config = {
                       'ALPHABETICAL_INDEX': 'YES',
                       'EXTRACT_ALL': 'YES',
                       'EXTRACT_LOCAL_METHODS': 'YES',
                       'EXTRACT_PRIVATE': 'YES',
                       'EXTRACT_STATIC': 'YES',
                       'FILE_PATTERNS': '*.py *.txt',
                       'FULL_PATH_NAMES ': 'YES',
                       'GENERATE_TREEVIEW': 'YES',
                       'HTML_DYNAMIC_SECTIONS': 'YES',
                       'HTML_FOOTER': 'footer.html',
                       'HTML_HEADER': 'header.html',
                       'HTML_OUTPUT ': self.html,
                       'INLINE_SOURCES': 'YES',
                       'INPUT ': self.src_tests,
                       'JAVADOC_AUTOBRIEF': 'YES',
                       'LATEX_OUTPUT ': self.latex,
                       'LAYOUT_FILE ': self.layout,
                       'OPTIMIZE_OUTPUT_JAVA': 'YES',
                       'PROJECT_NAME ': self.suitename[self.suite],
                       'PROJECT_NUMBER': self.docversion,
                       'SOURCE_BROWSER': 'YES',
                       'STRIP_CODE_COMMENTS': 'NO',
                       'TAB_SIZE': '4',
                       'USE_INLINE_TREES': 'YES',
                      }

        doxy_layout = {
                       'tab type="mainpage"': 'title="%s"' %
                         self.suitename[self.suite],
                       'tab type="namespaces"': 'title="Tests"',
                       'tab type="namespacemembers"': 'title="Test Functions"',
                      }

        for line in fileinput.input(self.doxyconf, inplace=1):
            for k in doxy_config:
                if line.startswith(k):
                    line = '%s = %s\n' % (k, doxy_config[k])
            sys.stdout.write(line)

        for line in fileinput.input('header.html', inplace=1):
            if line.startswith('<H2>'):
                line = '<H2>%s</H2>\n' % self.suitename[self.suite]
            sys.stdout.write(line)

        for line in fileinput.input(self.layout, inplace=1):
            for k in doxy_layout:
                if line.find(k) != -1:
                    line = line.replace('title=""', doxy_layout[k])
            sys.stdout.write(line.rstrip() + '\n')

    def RunDoxygen(self, doxyargs):
        """Execute Doxygen on the files in the self.src_tests directory.

        Args:
          doxyargs: string, any command line args to be passed to doxygen.
        """

        doxycmd = 'doxygen %s' % doxyargs

        p = subprocess.Popen(doxycmd, shell=True, stdout=subprocess.PIPE,
                             stderr=subprocess.PIPE)
        stdout, stderr = p.communicate()
        if p.returncode:
            self.logger.error('Error while running %s', doxycmd)
            self.logger.error(stdout)
            self.logger.error(stderr)
        else:
            self.logger.info('%s successfully ran', doxycmd)

    def CreateDocs(self):
        """Configure and execute Doxygen to create HTML docuements."""

        # First run doxygen with args to create default configuration files.
        # Create layout xml file.
        doxyargs = '-l %s' % self.layout
        self.RunDoxygen(doxyargs)

        # Create doxygen configuration file.
        doxyargs = '-g %s' % self.doxyconf
        self.RunDoxygen(doxyargs)

        # Edit the configuration files to match our options.
        self._ConfigDoxygen()

        # Run doxygen with configuration file as argument.
        self.RunDoxygen(self.doxyconf)

    def PostProcessDocs(self, current_dir):
        """Run some post processing on the newly created docs."""

        # Key = original string, value = replacement string.
        replace = {
                   '>Package': '>Test',
                  }

        docpages = os.path.join(self.html, '*.html')
        files = glob.glob(docpages)
        for file in files:
            for line in fileinput.input(file, inplace=1):
                for k in replace:
                    if line.find(k) != -1:
                        line = line.replace(k, replace[k])
                print line,

        logo_image = 'customLogo.gif'
        html_root = os.path.join(current_dir, self.html)
        shutil.copy(os.path.join(current_dir, logo_image), html_root)

        # Copy under dashboard.
        if self.options.dashboard:
            dashboard_root = os.path.join(self.autotest_root, 'results',
                                          'dashboard', 'testdocs')
            if not os.path.isdir(dashboard_root):
                try:
                    os.makedirs(dashboard_root)
                except e:
                    self.logger.error('Error creating %s:%s', dashboard_root, e)
                    return
            os.system('cp -r %s/* %s' % (html_root, dashboard_root))
            os.system('find %s -type d -exec chmod 755 {} \;' % dashboard_root)
            os.system('find %s -type f -exec chmod 644 {} \;' % dashboard_root)

        self.logger.info('Sanitized documentation completed.')


class ReadNode(object):
    """Parse a compiler node object from a control file.

    Args:
        suite: boolean, set to True if parsing nodes from a suite control file.
    """

    def __init__(self, suite=False):
        self.key = ''
        self.value = ''
        self.testdef = False
        self.suite = suite
        self.bullet = '  - '

    def visitName(self, n):
        if n.name == 'job':
            self.testdef = True

    def visitConst(self, n):
        if self.testdef:
            self.key = str(n.value)
            self.testdef = False
        else:
            self.value += str(n.value) + '\n'

    def visitKeyword(self, n):
        if n.name != 'constraints':
            self.value += self.bullet + n.name + ': '
        for item in n.expr:
            if isinstance(item, compiler.ast.Const):
                for i in item:
                    self.value += self.bullet + str(i) + '\n'
                self.value += '  .\n'
            else:
                self.value += str(item) + '\n'


    def visitAssName(self, n):
        # To remove section from appearing in the documentation, set value = ''.
        sections = {
                    'AUTHOR': '',
                    'CRITERIA': '<H3>Pass/Fail Criteria:</H3>\n',
                    'DOC': '<H3>Notes</H3>\n',
                    'NAME': '',
                    'PURPOSE': '\\brief\n',
                    'TIME': '<H3>Test Duration</H3>\n',
                    'TEST_CATEGORY': '<H3>Category</H3>\n',
                    'TEST_CLASS': '<H3>Test Class</H3>\n',
                    'TEST_TYPE': '<H3>Test Type</H3>\n',
                   }

        if not self.suite:
            self.key = sections.get(n.name, n.name)


def ParseOptions(current_dir):
    """Common processing of command line options."""

    desc="""%prog will scan AutoTest suite control files to build a list of
    test cases called in the suite, and build HTML documentation based on
    the docstrings it finds in the tests, control files, and suite control
    files.
    """
    parser = optparse.OptionParser(description=desc,
                                   prog='CreateDocs',
                                   version=__version__,
                                   usage='%prog')
    parser.add_option('--alltests',
                      help='Scan for all tests',
                      action='store_true',
                      default=False,
                      dest='all_tests')
    parser.add_option('--autotest_dir',
                      help='path to autotest root directory'
                           ' [default: %default]',
                      default=None,
                      dest='autotest_dir')
    parser.add_option('--dashboard',
                      help='Copy output under dashboard',
                      action='store_true',
                      default=False,
                      dest='dashboard')
    parser.add_option('--debug',
                      help='Debug level [default: %default]',
                      default='debug',
                      dest='debug')
    parser.add_option('--docversion',
                      help='Specify a version for the documentation'
                           '[default: %default]',
                      default=None,
                      dest='docversion')
    parser.add_option('--doxy',
                      help='doxygen configuration file [default: %default]',
                      default=os.path.join(current_dir, 'doxygen.conf'),
                      dest='doxyconf')
    parser.add_option('--html',
                      help='path to store html docs [default: %default]',
                      default='html',
                      dest='html')
    parser.add_option('--latex',
                      help='path to store latex docs [default: %default]',
                      default='latex',
                      dest='latex')
    parser.add_option('--layout',
                      help='doxygen layout file [default: %default]',
                      default=os.path.join(current_dir, 'doxygenLayout.xml'),
                      dest='layout')
    parser.add_option('--log',
                      help='Logfile for program output [default: %default]',
                      default=os.path.join(current_dir, 'docCreator.log'),
                      dest='logfile')
    parser.add_option('--readme',
                      help='filename of suite documentation'
                           '[default: %default]',
                      default='README.txt',
                      dest='readme')
    parser.add_option('--suite',
                      help='Directory name of suite [default: %default]',
                      type='choice',
                      default='suite_HWQual',
                      choices = [
                                 'suite_Factory',
                                 'suite_HWConfig',
                                 'suite_HWQual',
                                ],
                      dest='suite')
    parser.add_option('--tests',
                      help='Absolute path of temporary test files'
                           ' [default: %default]',
                      default='testsource',
                      dest='src_tests')
    return parser.parse_args()


def CheckOptions(options, logger):
    """Verify required command line options."""

    if not options.autotest_dir:
        logger.error('You must supply --autotest_dir')
        raise SystemExit

    if not os.path.isfile(options.doxyconf):
        logger.error('Unable to locate --doxy: %s', options.doxyconf)
        raise SystemExit

    if not os.path.isfile(options.layout):
        logger.error('Unable to locate --layout: %s', options.layout)
        raise SystemExit


def SetLogger(namespace, options):
    """Create a logger with some good formatting options.

    Args:
        namespace: string, name associated with this logger.
    Returns:
        Logger object.
    This method assumes logfile and debug are already set.
    This logger will write to stdout as well as a log file.
    """

    loglevel = {'debug': logging.DEBUG,
                'info': logging.INFO,
                'warning': logging.WARNING,
                'error': logging.ERROR,
                'critical': logging.CRITICAL,
               }

    logger = logging.getLogger(namespace)
    c = logging.StreamHandler()
    h = logging.FileHandler(
        os.path.join(os.path.abspath('.'), options.logfile))
    hf = logging.Formatter(
        '%(asctime)s %(process)d %(levelname)s: %(message)s')
    cf = logging.Formatter('%(levelname)s: %(message)s')
    logger.addHandler(h)
    logger.addHandler(c)
    h.setFormatter(hf)
    c.setFormatter(cf)

    logger.setLevel(loglevel.get(options.debug, logging.INFO))

    return logger


def main():
    current_dir = os.path.dirname(sys.argv[0])
    options, args = ParseOptions(current_dir)
    logger = SetLogger('docCreator', options)
    CheckOptions(options, logger)
    doc = DocCreator(options, args, logger)
    doc.GetTests()
    doc.ParseControlFiles()
    doc.CreateMainPage(current_dir)
    doc.CreateDocs()
    doc.PostProcessDocs(current_dir)


if __name__ == '__main__':
    main()
