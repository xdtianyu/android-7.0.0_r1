# pylint: disable-msg=C0111
# Copyright 2008 Google Inc. Released under the GPL v2

import warnings
with warnings.catch_warnings():
    # The 'compiler' module is gone in Python 3.0.  Let's not say
    # so in every log file.
    warnings.simplefilter("ignore", DeprecationWarning)
    import compiler
import logging, textwrap

from autotest_lib.client.common_lib import enum

REQUIRED_VARS = set(['author', 'doc', 'name', 'time', 'test_type'])
OBSOLETE_VARS = set(['experimental'])

CONTROL_TYPE = enum.Enum('Server', 'Client', start_value=1)
CONTROL_TYPE_NAMES =  enum.Enum(*CONTROL_TYPE.names, string_values=True)

class ControlVariableException(Exception):
    pass


class ControlData(object):
    # Available TIME settings in control file, the list must be in lower case
    # and in ascending order, test running faster comes first.
    TEST_TIME_LIST = ['fast', 'short', 'medium', 'long', 'lengthy']
    TEST_TIME = enum.Enum(*TEST_TIME_LIST, string_values=False)

    @staticmethod
    def get_test_time_index(time):
        """
        Get the order of estimated test time, based on the TIME setting in
        Control file. Faster test gets a lower index number.
        """
        try:
            return ControlData.TEST_TIME.get_value(time.lower())
        except AttributeError:
            # Raise exception if time value is not a valid TIME setting.
            error_msg = '%s is not a valid TIME.' % time
            logging.error(error_msg)
            raise ControlVariableException(error_msg)


    def __init__(self, vars, path, raise_warnings=False):
        # Defaults
        self.path = path
        self.dependencies = set()
        # TODO(jrbarnette): This should be removed once outside
        # code that uses can be changed.
        self.experimental = False
        self.run_verify = True
        self.sync_count = 1
        self.test_parameters = set()
        self.test_category = ''
        self.test_class = ''
        self.retries = 0
        self.job_retries = 0
        # Default to require server-side package. Unless require_ssp is
        # explicitly set to False, server-side package will be used for the
        # job. This can be overridden by global config
        # AUTOSERV/enable_ssp_container
        self.require_ssp = None
        self.attributes = set()

        diff = REQUIRED_VARS - set(vars)
        if diff:
            warning = ('WARNING: Not all required control '
                       'variables were specified in %s.  Please define '
                       '%s.') % (self.path, ', '.join(diff))
            if raise_warnings:
                raise ControlVariableException(warning)
            print textwrap.wrap(warning, 80)

        obsolete = OBSOLETE_VARS & set(vars)
        if obsolete:
            warning = ('WARNING: Obsolete variables were '
                       'specified in %s.  Please remove '
                       '%s.') % (self.path, ', '.join(obsolete))
            if raise_warnings:
                raise ControlVariableException(warning)
            print textwrap.wrap(warning, 80)

        for key, val in vars.iteritems():
            try:
                self.set_attr(key, val, raise_warnings)
            except Exception, e:
                if raise_warnings:
                    raise
                print 'WARNING: %s; skipping' % e


    def set_attr(self, attr, val, raise_warnings=False):
        attr = attr.lower()
        try:
            set_fn = getattr(self, 'set_%s' % attr)
            set_fn(val)
        except AttributeError:
            # This must not be a variable we care about
            pass


    def _set_string(self, attr, val):
        val = str(val)
        setattr(self, attr, val)


    def _set_option(self, attr, val, options):
        val = str(val)
        if val.lower() not in [x.lower() for x in options]:
            raise ValueError("%s must be one of the following "
                             "options: %s" % (attr,
                             ', '.join(options)))
        setattr(self, attr, val)


    def _set_bool(self, attr, val):
        val = str(val).lower()
        if val == "false":
            val = False
        elif val == "true":
            val = True
        else:
            msg = "%s must be either true or false" % attr
            raise ValueError(msg)
        setattr(self, attr, val)


    def _set_int(self, attr, val, min=None, max=None):
        val = int(val)
        if min is not None and min > val:
            raise ValueError("%s is %d, which is below the "
                             "minimum of %d" % (attr, val, min))
        if max is not None and max < val:
            raise ValueError("%s is %d, which is above the "
                             "maximum of %d" % (attr, val, max))
        setattr(self, attr, val)


    def _set_set(self, attr, val):
        val = str(val)
        items = [x.strip() for x in val.split(',')]
        setattr(self, attr, set(items))


    def set_author(self, val):
        self._set_string('author', val)


    def set_dependencies(self, val):
        self._set_set('dependencies', val)


    def set_doc(self, val):
        self._set_string('doc', val)


    def set_name(self, val):
        self._set_string('name', val)


    def set_run_verify(self, val):
        self._set_bool('run_verify', val)


    def set_sync_count(self, val):
        self._set_int('sync_count', val, min=1)


    def set_suite(self, val):
        self._set_string('suite', val)


    def set_time(self, val):
        self._set_option('time', val, ControlData.TEST_TIME_LIST)


    def set_test_class(self, val):
        self._set_string('test_class', val.lower())


    def set_test_category(self, val):
        self._set_string('test_category', val.lower())


    def set_test_type(self, val):
        self._set_option('test_type', val, list(CONTROL_TYPE.names))


    def set_test_parameters(self, val):
        self._set_set('test_parameters', val)


    def set_retries(self, val):
        self._set_int('retries', val)


    def set_job_retries(self, val):
        self._set_int('job_retries', val)


    def set_bug_template(self, val):
        if type(val) == dict:
            setattr(self, 'bug_template', val)


    def set_require_ssp(self, val):
        self._set_bool('require_ssp', val)


    def set_attributes(self, val):
        # Add subsystem:default if subsystem is not specified.
        self._set_set('attributes', val)
        if not any(a.startswith('subsystem') for a in self.attributes):
            self.attributes.add('subsystem:default')


def _extract_const(expr):
    assert(expr.__class__ == compiler.ast.Const)
    assert(expr.value.__class__ in (str, int, float, unicode))
    return str(expr.value).strip()


def _extract_dict(expr):
    assert(expr.__class__ == compiler.ast.Dict)
    assert(expr.items.__class__ == list)
    cf_dict = {}
    for key, value in expr.items:
        try:
            key = _extract_const(key)
            val = _extract_expression(value)
        except (AssertionError, ValueError):
            pass
        else:
            cf_dict[key] = val
    return cf_dict


def _extract_list(expr):
    assert(expr.__class__ == compiler.ast.List)
    list_values = []
    for value in expr.nodes:
        try:
            list_values.append(_extract_expression(value))
        except (AssertionError, ValueError):
            pass
    return list_values


def _extract_name(expr):
    assert(expr.__class__ == compiler.ast.Name)
    assert(expr.name in ('False', 'True', 'None'))
    return str(expr.name)


def _extract_expression(expr):
    if expr.__class__ == compiler.ast.Const:
        return _extract_const(expr)
    if expr.__class__ == compiler.ast.Name:
        return _extract_name(expr)
    if expr.__class__ == compiler.ast.Dict:
        return _extract_dict(expr)
    if expr.__class__ == compiler.ast.List:
        return _extract_list(expr)
    raise ValueError('Unknown rval %s' % expr)


def _extract_assignment(n):
    assert(n.__class__ == compiler.ast.Assign)
    assert(n.nodes.__class__ == list)
    assert(len(n.nodes) == 1)
    assert(n.nodes[0].__class__ == compiler.ast.AssName)
    assert(n.nodes[0].flags.__class__ == str)
    assert(n.nodes[0].name.__class__ == str)

    val = _extract_expression(n.expr)
    key = n.nodes[0].name.lower()

    return (key, val)


def parse_control_string(control, raise_warnings=False):
    try:
        mod = compiler.parse(control)
    except SyntaxError, e:
        raise ControlVariableException("Error parsing data because %s" % e)
    return finish_parse(mod, '', raise_warnings)


def parse_control(path, raise_warnings=False):
    try:
        mod = compiler.parseFile(path)
    except SyntaxError, e:
        raise ControlVariableException("Error parsing %s because %s" %
                                       (path, e))
    return finish_parse(mod, path, raise_warnings)


def finish_parse(mod, path, raise_warnings):
    assert(mod.__class__ == compiler.ast.Module)
    assert(mod.node.__class__ == compiler.ast.Stmt)
    assert(mod.node.nodes.__class__ == list)

    vars = {}
    for n in mod.node.nodes:
        try:
            key, val = _extract_assignment(n)
            vars[key] = val
        except (AssertionError, ValueError):
            pass
    return ControlData(vars, path, raise_warnings)
