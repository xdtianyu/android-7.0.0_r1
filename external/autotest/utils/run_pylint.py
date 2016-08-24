#!/usr/bin/python -u
"""
Wrapper to patch pylint library functions to suit autotest.

This script is invoked as part of the presubmit checks for autotest python
files. It runs pylint on a list of files that it obtains either through
the command line or from an environment variable set in pre-upload.py.

Example:
run_pylint.py filename.py
"""

import fnmatch
import logging
import os
import re
import sys

import common
from autotest_lib.client.common_lib import autotemp, revision_control

# Do a basic check to see if pylint is even installed.
try:
    import pylint
    from pylint.__pkginfo__ import version as pylint_version
except ImportError:
    print ("Unable to import pylint, it may need to be installed."
           " Run 'sudo aptitude install pylint' if you haven't already.")
    sys.exit(1)

major, minor, release = pylint_version.split('.')
pylint_version = float("%s.%s" % (major, minor))

# some files make pylint blow up, so make sure we ignore them
BLACKLIST = ['/contrib/*', '/frontend/afe/management.py']

# patch up the logilab module lookup tools to understand autotest_lib.* trash
import logilab.common.modutils
_ffm = logilab.common.modutils.file_from_modpath
def file_from_modpath(modpath, path=None, context_file=None):
    """
    Wrapper to eliminate autotest_lib from modpath.

    @param modpath: name of module splitted on '.'
    @param path: optional list of paths where module should be searched for.
    @param context_file: path to file doing the importing.
    @return The path to the module as returned by the parent method invocation.
    @raises: ImportError if these is no such module.
    """
    if modpath[0] == "autotest_lib":
        return _ffm(modpath[1:], path, context_file)
    else:
        return _ffm(modpath, path, context_file)
logilab.common.modutils.file_from_modpath = file_from_modpath


import pylint.lint
from pylint.checkers import base, imports, variables

# need to put autotest root dir on sys.path so pylint will be happy
autotest_root = os.path.abspath(os.path.join(os.path.dirname(__file__), '..'))
sys.path.insert(0, autotest_root)

# patch up pylint import checker to handle our importing magic
ROOT_MODULE = 'autotest_lib.'

# A list of modules for pylint to ignore, specifically, these modules
# are imported for their side-effects and are not meant to be used.
_IGNORE_MODULES=['common', 'frontend_test_utils',
                 'setup_django_environment',
                 'setup_django_lite_environment',
                 'setup_django_readonly_environment', 'setup_test_environment',]


class pylint_error(Exception):
    """
    Error raised when pylint complains about a file.
    """


class run_pylint_error(pylint_error):
    """
    Error raised when an assumption made in this file is violated.
    """


def patch_modname(modname):
    """
    Patches modname so we can make sense of autotest_lib modules.

    @param modname: name of a module, contains '.'
    @return modified modname string.
    """
    if modname.startswith(ROOT_MODULE) or modname.startswith(ROOT_MODULE[:-1]):
        modname = modname[len(ROOT_MODULE):]
    return modname


def patch_consumed_list(to_consume=None, consumed=None):
    """
    Patches the consumed modules list to ignore modules with side effects.

    Autotest relies on importing certain modules solely for their side
    effects. Pylint doesn't understand this and flags them as unused, since
    they're not referenced anywhere in the code. To overcome this we need
    to transplant said modules into the dictionary of modules pylint has
    already seen, before pylint checks it.

    @param to_consume: a dictionary of names pylint needs to see referenced.
    @param consumed: a dictionary of names that pylint has seen referenced.
    """
    ignore_modules = []
    if (to_consume is not None and consumed is not None):
        ignore_modules = [module_name for module_name in _IGNORE_MODULES
                          if module_name in to_consume]

    for module_name in ignore_modules:
        consumed[module_name] = to_consume[module_name]
        del to_consume[module_name]


class CustomImportsChecker(imports.ImportsChecker):
    """Modifies stock imports checker to suit autotest."""
    def visit_from(self, node):
        node.modname = patch_modname(node.modname)
        return super(CustomImportsChecker, self).visit_from(node)


class CustomVariablesChecker(variables.VariablesChecker):
    """Modifies stock variables checker to suit autotest."""

    def visit_module(self, node):
        """
        Unflag 'import common'.

        _to_consume eg: [({to reference}, {referenced}, 'scope type')]
        Enteries are appended to this list as we drill deeper in scope.
        If we ever come across a module to ignore,  we immediately move it
        to the consumed list.

        @param node: node of the ast we're currently checking.
        """
        super(CustomVariablesChecker, self).visit_module(node)
        scoped_names = self._to_consume.pop()
        patch_consumed_list(scoped_names[0],scoped_names[1])
        self._to_consume.append(scoped_names)

    def visit_from(self, node):
        """Patches modnames so pylints understands autotest_lib."""
        node.modname = patch_modname(node.modname)
        return super(CustomVariablesChecker, self).visit_from(node)


class CustomDocStringChecker(base.DocStringChecker):
    """Modifies stock docstring checker to suit Autotest doxygen style."""

    def visit_module(self, node):
        """
        Don't visit imported modules when checking for docstrings.

        @param node: the node we're visiting.
        """
        pass


    def visit_function(self, node):
        """
        Don't request docstrings for commonly overridden autotest functions.

        @param node: node of the ast we're currently checking.
        """

        # Even plain functions will have a parent, which is the
        # module they're in, and a frame, which is the context
        # of said module; They need not however, always have
        # ancestors.
        if (node.name in ('run_once', 'initialize', 'cleanup') and
            hasattr(node.parent.frame(), 'ancestors') and
            any(ancestor.name == 'base_test' for ancestor in
                node.parent.frame().ancestors())):
            return

        super(CustomDocStringChecker, self).visit_function(node)


    @staticmethod
    def _should_skip_arg(arg):
        """
        @return: True if the argument given by arg is whitelisted, and does
                 not require a "@param" docstring.
        """
        return arg in ('self', 'cls', 'args', 'kwargs', 'dargs')


    def _check_docstring(self, node_type, node):
        """
        Teaches pylint to look for @param with each argument in the
        function/method signature.

        @param node_type: type of the node we're currently checking.
        @param node: node of the ast we're currently checking.
        """
        super(CustomDocStringChecker, self)._check_docstring(node_type, node)
        docstring = node.doc
        if pylint_version >= 1.1:
            key = 'missing-docstring'
        else:
            key = 'C0111'

        if (docstring is not None and
               (node_type is 'method' or
                node_type is 'function')):
            args = node.argnames()
            old_msg = self.linter._messages[key].msg
            for arg in args:
                arg_docstring_rgx = '.*@param '+arg+'.*'
                line = re.search(arg_docstring_rgx, node.doc)
                if not line and not self._should_skip_arg(arg):
                    self.linter._messages[key].msg = ('Docstring needs '
                                                      '"@param '+arg+':"')
                    self.add_message(key, node=node)
            self.linter._messages[key].msg = old_msg

base.DocStringChecker = CustomDocStringChecker
imports.ImportsChecker = CustomImportsChecker
variables.VariablesChecker = CustomVariablesChecker


def batch_check_files(file_paths, base_opts):
    """
    Run pylint on a list of files so we get consolidated errors.

    @param file_paths: a list of file paths.
    @param base_opts: a list of pylint config options.

    @raises: pylint_error if pylint finds problems with a file
             in this commit.
    """
    if not file_paths:
        return

    pylint_runner = pylint.lint.Run(list(base_opts) + list(file_paths),
                                    exit=False)
    if pylint_runner.linter.msg_status:
        raise pylint_error(pylint_runner.linter.msg_status)


def should_check_file(file_path):
    """
    Don't check blacklisted or non .py files.

    @param file_path: abs path of file to check.
    @return: True if this file is a non-blacklisted python file.
    """
    file_path = os.path.abspath(file_path)
    if file_path.endswith('.py'):
        return all(not fnmatch.fnmatch(file_path, '*' + pattern)
                   for pattern in BLACKLIST)
    return False


def check_file(file_path, base_opts):
    """
    Invokes pylint on files after confirming that they're not black listed.

    @param base_opts: pylint base options.
    @param file_path: path to the file we need to run pylint on.
    """
    if not isinstance(file_path, basestring):
        raise TypeError('expected a string as filepath, got %s'%
            type(file_path))

    if should_check_file(file_path):
        pylint_runner = pylint.lint.Run(base_opts + [file_path], exit=False)
        if pylint_runner.linter.msg_status:
            pylint_error(pylint_runner.linter.msg_status)


def visit(arg, dirname, filenames):
    """
    Visit function invoked in check_dir.

    @param arg: arg from os.walk.path
    @param dirname: dir from os.walk.path
    @param filenames: files in dir from os.walk.path
    """
    for filename in filenames:
        check_file(os.path.join(dirname, filename), arg)


def check_dir(dir_path, base_opts):
    """
    Calls visit on files in dir_path.

    @param base_opts: pylint base options.
    @param dir_path: path to directory.
    """
    os.path.walk(dir_path, visit, base_opts)


def extend_baseopts(base_opts, new_opt):
    """
    Replaces an argument in base_opts with a cmd line argument.

    @param base_opts: original pylint_base_opts.
    @param new_opt: new cmd line option.
    """
    for args in base_opts:
        if new_opt in args:
            base_opts.remove(args)
    base_opts.append(new_opt)


def get_cmdline_options(args_list, pylint_base_opts, rcfile):
    """
    Parses args_list and extends pylint_base_opts.

    Command line arguments might include options mixed with files.
    Go through this list and filter out the options, if the options are
    specified in the pylintrc file we cannot replace them and the file
    needs to be edited. If the options are already a part of
    pylint_base_opts we replace them, and if not we append to
    pylint_base_opts.

    @param args_list: list of files/pylint args passed in through argv.
    @param pylint_base_opts: default pylint options.
    @param rcfile: text from pylint_rc.
    """
    for args in args_list:
        if args.startswith('--'):
            opt_name = args[2:].split('=')[0]
            if opt_name in rcfile and pylint_version >= 0.21:
                raise run_pylint_error('The rcfile already contains the %s '
                                        'option. Please edit pylintrc instead.'
                                        % opt_name)
            else:
                extend_baseopts(pylint_base_opts, args)
                args_list.remove(args)


def git_show_to_temp_file(commit, original_file, new_temp_file):
    """
    'Git shows' the file in original_file to a tmp file with
    the name new_temp_file. We need to preserve the filename
    as it gets reflected in pylints error report.

    @param commit: commit hash of the commit we're running repo upload on.
    @param original_file: the path to the original file we'd like to run
                          'git show' on.
    @param new_temp_file: new_temp_file is the path to a temp file we write the
                          output of 'git show' into.
    """
    git_repo = revision_control.GitRepo(common.autotest_dir, None, None,
        common.autotest_dir)

    with open(new_temp_file, 'w') as f:
        output = git_repo.gitcmd('show --no-ext-diff %s:%s'
                                 % (commit, original_file),
                                 ignore_status=False).stdout
        f.write(output)


def check_committed_files(work_tree_files, commit, pylint_base_opts):
    """
    Get a list of files corresponding to the commit hash.

    The contents of a file in the git work tree can differ from the contents
    of a file in the commit we mean to upload. To work around this we run
    pylint on a temp file into which we've 'git show'n the committed version
    of each file.

    @param work_tree_files: list of files in this commit specified by their
                            absolute path.
    @param commit: hash of the commit this upload applies to.
    @param pylint_base_opts: a list of pylint config options.
    """
    files_to_check = filter(should_check_file, work_tree_files)

    # Map the absolute path of each file so it's relative to the autotest repo.
    # All files that are a part of this commit should have an abs path within
    # the autotest repo, so this regex should never fail.
    work_tree_files = [re.search(r'%s/(.*)' % common.autotest_dir, f).group(1)
                       for f in files_to_check]

    tempdir = None
    try:
        tempdir = autotemp.tempdir()
        temp_files = [os.path.join(tempdir.name, file_path.split('/')[-1:][0])
                      for file_path in work_tree_files]

        for file_tuple in zip(work_tree_files, temp_files):
            git_show_to_temp_file(commit, *file_tuple)
        # Only check if we successfully git showed all files in the commit.
        batch_check_files(temp_files, pylint_base_opts)
    finally:
        if tempdir:
            tempdir.clean()


def main():
    """Main function checks each file in a commit for pylint violations."""

    # For now all error/warning/refactor/convention exceptions except those in
    # the enable string are disabled.
    # W0611: All imported modules (except common) need to be used.
    # W1201: Logging methods should take the form
    #   logging.<loggingmethod>(format_string, format_args...); and not
    #   logging.<loggingmethod>(format_string % (format_args...))
    # C0111: Docstring needed. Also checks @param for each arg.
    # C0112: Non-empty Docstring needed.
    # Ideally we would like to enable as much as we can, but if we did so at
    # this stage anyone who makes a tiny change to a file will be tasked with
    # cleaning all the lint in it. See chromium-os:37364.

    # Note:
    # 1. There are three major sources of E1101/E1103/E1120 false positives:
    #    * common_lib.enum.Enum objects
    #    * DB model objects (scheduler models are the worst, but Django models
    #      also generate some errors)
    # 2. Docstrings are optional on private methods, and any methods that begin
    #    with either 'set_' or 'get_'.
    pylint_rc = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                             'pylintrc')

    no_docstring_rgx = r'((_.*)|(set_.*)|(get_.*))'
    if pylint_version >= 0.21:
        pylint_base_opts = ['--rcfile=%s' % pylint_rc,
                            '--reports=no',
                            '--disable=W,R,E,C,F',
                            '--enable=W0611,W1201,C0111,C0112,E0602,W0601',
                            '--no-docstring-rgx=%s' % no_docstring_rgx,]
    else:
        all_failures = 'error,warning,refactor,convention'
        pylint_base_opts = ['--disable-msg-cat=%s' % all_failures,
                            '--reports=no',
                            '--include-ids=y',
                            '--ignore-docstrings=n',
                            '--no-docstring-rgx=%s' % no_docstring_rgx,]

    # run_pylint can be invoked directly with command line arguments,
    # or through a presubmit hook which uses the arguments in pylintrc. In the
    # latter case no command line arguments are passed. If it is invoked
    # directly without any arguments, it should check all files in the cwd.
    args_list = sys.argv[1:]
    if args_list:
        get_cmdline_options(args_list,
                            pylint_base_opts,
                            open(pylint_rc).read())
        batch_check_files(args_list, pylint_base_opts)
    elif os.environ.get('PRESUBMIT_FILES') is not None:
        check_committed_files(
                              os.environ.get('PRESUBMIT_FILES').split('\n'),
                              os.environ.get('PRESUBMIT_COMMIT'),
                              pylint_base_opts)
    else:
        check_dir('.', pylint_base_opts)


if __name__ == '__main__':
    try:
        main()
    except Exception as e:
        logging.error(e)
        sys.exit(1)
