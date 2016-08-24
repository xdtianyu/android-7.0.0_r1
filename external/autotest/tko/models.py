import os

from autotest_lib.client.common_lib import utils
from autotest_lib.tko import utils as tko_utils


class job(object):
    """Represents a job."""

    def __init__(self, dir, user, label, machine, queued_time, started_time,
                 finished_time, machine_owner, machine_group, aborted_by,
                 aborted_on, keyval_dict):
        self.dir = dir
        self.tests = []
        self.user = user
        self.label = label
        self.machine = machine
        self.queued_time = queued_time
        self.started_time = started_time
        self.finished_time = finished_time
        self.machine_owner = machine_owner
        self.machine_group = machine_group
        self.aborted_by = aborted_by
        self.aborted_on = aborted_on
        self.keyval_dict = keyval_dict


    @staticmethod
    def read_keyval(dir):
        """
        Read job keyval files.

        @param dir: String name of directory containing job keyval files.

        @return A dictionary containing job keyvals.

        """
        dir = os.path.normpath(dir)
        top_dir = tko_utils.find_toplevel_job_dir(dir)
        if not top_dir:
            top_dir = dir
        assert(dir.startswith(top_dir))

        # Pull in and merge all the keyval files, with higher-level
        # overriding values in the lower-level ones.
        keyval = {}
        while True:
            try:
                upper_keyval = utils.read_keyval(dir)
                # HACK: exclude hostname from the override - this is a special
                # case where we want lower to override higher.
                if 'hostname' in upper_keyval and 'hostname' in keyval:
                    del upper_keyval['hostname']
                keyval.update(upper_keyval)
            except IOError:
                pass  # If the keyval can't be read just move on to the next.
            if dir == top_dir:
                break
            else:
                assert(dir != '/')
                dir = os.path.dirname(dir)
        return keyval


class kernel(object):
    """Represents a kernel."""

    def __init__(self, base, patches, kernel_hash):
        self.base = base
        self.patches = patches
        self.kernel_hash = kernel_hash


    @staticmethod
    def compute_hash(base, hashes):
        """Compute a hash given the base string and hashes for each patch.

        @param base: A string representing the kernel base.
        @param hashes: A list of hashes, where each hash is associated with a
            patch of this kernel.

        @return A string representing the computed hash.

        """
        key_string = ','.join([base] + hashes)
        return utils.hash('md5', key_string).hexdigest()


class test(object):
    """Represents a test."""

    def __init__(self, subdir, testname, status, reason, test_kernel,
                 machine, started_time, finished_time, iterations,
                 attributes, perf_values, labels):
        self.subdir = subdir
        self.testname = testname
        self.status = status
        self.reason = reason
        self.kernel = test_kernel
        self.machine = machine
        self.started_time = started_time
        self.finished_time = finished_time
        self.iterations = iterations
        self.attributes = attributes
        self.perf_values = perf_values
        self.labels = labels


    @staticmethod
    def load_iterations(keyval_path):
        """Abstract method to load a list of iterations from a keyval file.

        @param keyval_path: String path to a keyval file.

        @return A list of iteration objects.

        """
        raise NotImplementedError


    @staticmethod
    def load_perf_values(perf_values_file):
        """Loads perf values from a perf measurements file.

        @param perf_values_file: The string path to a perf measurements file.

        @return A list of perf_value_iteration objects.

        """
        raise NotImplementedError


    @classmethod
    def parse_test(cls, job, subdir, testname, status, reason, test_kernel,
                   started_time, finished_time, existing_instance=None):
        """
        Parse test result files to construct a complete test instance.

        Given a job and the basic metadata about the test that can be
        extracted from the status logs, parse the test result files (keyval
        files and perf measurement files) and use them to construct a complete
        test instance.

        @param job: A job object.
        @param subdir: The string subdirectory name for the given test.
        @param testname: The name of the test.
        @param status: The status of the test.
        @param reason: The reason string for the test.
        @param test_kernel: The kernel of the test.
        @param started_time: The start time of the test.
        @param finished_time: The finish time of the test.
        @param existing_instance: An existing test instance.

        @return A test instance that has the complete information.

        """
        tko_utils.dprint("parsing test %s %s" % (subdir, testname))

        if subdir:
            # Grab iterations from the results keyval.
            iteration_keyval = os.path.join(job.dir, subdir,
                                            'results', 'keyval')
            iterations = cls.load_iterations(iteration_keyval)

            # Grab perf values from the perf measurements file.
            perf_values_file = os.path.join(job.dir, subdir,
                                            'results', 'perf_measurements')
            perf_values = cls.load_perf_values(perf_values_file)

            # Grab test attributes from the subdir keyval.
            test_keyval = os.path.join(job.dir, subdir, 'keyval')
            attributes = test.load_attributes(test_keyval)
        else:
            iterations = []
            perf_values = []
            attributes = {}

        # Grab test+host attributes from the host keyval.
        host_keyval = cls.parse_host_keyval(job.dir, job.machine)
        attributes.update(dict(('host-%s' % k, v)
                               for k, v in host_keyval.iteritems()))

        if existing_instance:
            def constructor(*args, **dargs):
                """Initializes an existing test instance."""
                existing_instance.__init__(*args, **dargs)
                return existing_instance
        else:
            constructor = cls

        return constructor(subdir, testname, status, reason, test_kernel,
                           job.machine, started_time, finished_time,
                           iterations, attributes, perf_values, [])


    @classmethod
    def parse_partial_test(cls, job, subdir, testname, reason, test_kernel,
                           started_time):
        """
        Create a test instance representing a partial test result.

        Given a job and the basic metadata available when a test is
        started, create a test instance representing the partial result.
        Assume that since the test is not complete there are no results files
        actually available for parsing.

        @param job: A job object.
        @param subdir: The string subdirectory name for the given test.
        @param testname: The name of the test.
        @param reason: The reason string for the test.
        @param test_kernel: The kernel of the test.
        @param started_time: The start time of the test.

        @return A test instance that has partial test information.

        """
        tko_utils.dprint('parsing partial test %s %s' % (subdir, testname))

        return cls(subdir, testname, 'RUNNING', reason, test_kernel,
                   job.machine, started_time, None, [], {}, [], [])


    @staticmethod
    def load_attributes(keyval_path):
        """
        Load test attributes from a test keyval path.

        Load the test attributes into a dictionary from a test
        keyval path. Does not assume that the path actually exists.

        @param keyval_path: The string path to a keyval file.

        @return A dictionary representing the test keyvals.

        """
        if not os.path.exists(keyval_path):
            return {}
        return utils.read_keyval(keyval_path)


    @staticmethod
    def parse_host_keyval(job_dir, hostname):
        """
        Parse host keyvals.

        @param job_dir: The string directory name of the associated job.
        @param hostname: The string hostname.

        @return A dictionary representing the host keyvals.

        """
        # The "real" job dir may be higher up in the directory tree.
        job_dir = tko_utils.find_toplevel_job_dir(job_dir)
        if not job_dir:
            return {}  # We can't find a top-level job dir with host keyvals.

        # The keyval is <job_dir>/host_keyvals/<hostname> if it exists.
        keyval_path = os.path.join(job_dir, 'host_keyvals', hostname)
        if os.path.isfile(keyval_path):
            return utils.read_keyval(keyval_path)
        else:
            return {}


class patch(object):
    """Represents a patch."""

    def __init__(self, spec, reference, hash):
        self.spec = spec
        self.reference = reference
        self.hash = hash


class iteration(object):
    """Represents an iteration."""

    def __init__(self, index, attr_keyval, perf_keyval):
        self.index = index
        self.attr_keyval = attr_keyval
        self.perf_keyval = perf_keyval


    @staticmethod
    def parse_line_into_dicts(line, attr_dict, perf_dict):
        """
        Abstract method to parse a keyval line and insert it into a dictionary.

        @param line: The string line to parse.
        @param attr_dict: Dictionary of generic iteration attributes.
        @param perf_dict: Dictionary of iteration performance results.

        """
        raise NotImplementedError


    @classmethod
    def load_from_keyval(cls, keyval_path):
        """
        Load a list of iterations from an iteration keyval file.

        Keyval data from separate iterations is separated by blank
        lines. Makes use of the parse_line_into_dicts method to
        actually parse the individual lines.

        @param keyval_path: The string path to a keyval file.

        @return A list of iteration objects.

        """
        if not os.path.exists(keyval_path):
            return []

        iterations = []
        index = 1
        attr, perf = {}, {}
        for line in file(keyval_path):
            line = line.strip()
            if line:
                cls.parse_line_into_dicts(line, attr, perf)
            else:
                iterations.append(cls(index, attr, perf))
                index += 1
                attr, perf = {}, {}
        if attr or perf:
            iterations.append(cls(index, attr, perf))
        return iterations


class perf_value_iteration(object):
    """Represents a perf value iteration."""

    def __init__(self, index, perf_measurements):
        """
        Initializes the perf values for a particular test iteration.

        @param index: The integer iteration number.
        @param perf_measurements: A list of dictionaries, where each dictionary
            contains the information for a measured perf metric from the
            current iteration.

        """
        self.index = index
        self.perf_measurements = perf_measurements


    def add_measurement(self, measurement):
        """
        Appends to the list of perf measurements for this iteration.

        @param measurement: A dictionary containing information for a measured
            perf metric.

        """
        self.perf_measurements.append(measurement)


    @staticmethod
    def parse_line_into_dict(line):
        """
        Abstract method to parse an individual perf measurement line.

        @param line: A string line from the perf measurement output file.

        @return A dicionary representing the information for a measured perf
            metric from one line of the perf measurement output file, or an
            empty dictionary if the line cannot be parsed successfully.

        """
        raise NotImplementedError


    @classmethod
    def load_from_perf_values_file(cls, perf_values_file):
        """
        Load perf values from each iteration in a perf measurements file.

        Multiple measurements for the same perf metric description are assumed
        to come from different iterations.  Makes use of the
        parse_line_into_dict function to actually parse the individual lines.

        @param perf_values_file: The string name of the output file containing
            perf measurements.

        @return A list of |perf_value_iteration| objects, where position 0 of
            the list contains the object representing the first iteration,
            position 1 contains the object representing the second iteration,
            and so forth.

        """
        if not os.path.exists(perf_values_file):
            return []

        perf_value_iterations = []
        # For each description string representing a unique perf metric, keep
        # track of the next iteration that it belongs to (multiple occurrences
        # of the same description are assumed to come from different
        # iterations).
        desc_to_next_iter = {}
        with open(perf_values_file) as fp:
            for line in [ln for ln in fp if ln.strip()]:
                perf_value_dict = cls.parse_line_into_dict(line)
                if not perf_value_dict:
                    continue
                desc = perf_value_dict['description']
                iter_to_set = desc_to_next_iter.setdefault(desc, 1)
                desc_to_next_iter[desc] = iter_to_set + 1
                if iter_to_set > len(perf_value_iterations):
                    # We have information that needs to go into a new
                    # |perf_value_iteration| object.
                    perf_value_iterations.append(cls(iter_to_set, []))
                # Add the perf measurement to the appropriate
                # |perf_value_iteration| object.
                perf_value_iterations[iter_to_set - 1].add_measurement(
                        perf_value_dict)
        return perf_value_iterations
