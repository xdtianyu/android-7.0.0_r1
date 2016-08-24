# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""A module handling the logs.

The structure of this module:

    RoundLog: the test results of every round are saved in a log file.
              includes: fw, and round_name (i.e., the date time of the round

      --> GestureLogs: includes gesture name, and variation

            --> ValidatorLogs: includes name, details, criteria, score, metrics


    SummaryLog: derived from multiple RoundLogs
      --> SimpleTable: (key, vlog) pairs
            key: (fw, round_name, gesture_name, variation_name, validator_name)
            vlog: name, details, criteria, score, metrics

    TestResult: encapsulation of scores and metrics
                used by a client program to query the test results
      --> StatisticsScores: includes average, ssd, and count
      --> StatisticsMetrics: includes average, min, max, and more


How the logs work:
    (1) ValidatorLogs are contained in a GestureLog.
    (2) Multiple GestureLogs are packed in a RoundLog which is saved in a
        separate pickle log file.
    (3) To construct a SummaryLog, it reads RoundLogs from all pickle logs
        in the specified log directory. It then creates a SimpleTable
        consisting of (key, ValidatorLog) pairs, where
        key is a 5-tuple:
            (fw, round_name, gesture_name, variation_name, validator_name).
    (4) The client program, i.e., firmware_summary module, contains a
        SummaryLog, and queries all statistics using get_result() which returns
        a TestResult object containing both StatisticsScores and
        StatisticsMetrics.

"""


import glob
import numpy as np
import pickle
import os

import test_conf as conf
import validators as val

from collections import defaultdict, namedtuple

from common_util import Debug, print_and_exit
from firmware_constants import AXIS


MetricProps = namedtuple('MetricProps', ['description', 'note', 'stat_func'])


def _setup_debug(debug_flag):
    """Set up the global debug_print function."""
    if 'debug_print' not in globals():
        global debug_print
        debug = Debug(debug_flag)
        debug_print = debug.print_msg


def _calc_sample_standard_deviation(sample):
    """Calculate the sample standard deviation (ssd) from a given sample.

    To compute a sample standard deviation, the following formula is used:
        sqrt(sum((x_i - x_average)^2) / N-1)

    Note that N-1 is used in the denominator for sample standard deviation,
    where N-1 is the degree of freedom. We need to set ddof=1 below;
    otherwise, N would be used in the denominator as ddof's default value
    is 0.

    Reference:
        http://en.wikipedia.org/wiki/Standard_deviation
    """
    return np.std(np.array(sample), ddof=1)


class float_d2(float):
    """A float type with special __repr__ and __str__ methods that display
    the float number to the 2nd decimal place."""
    template = '%.2f'

    def __str__(self):
        """Display the float to the 2nd decimal place."""
        return self.template % self.real

    def __repr__(self):
        """Display the float to the 2nd decimal place."""
        return self.template % self.real


def convert_float_to_float_d2(value):
    """Convert the float(s) in value to float_d2."""
    if isinstance(value, float):
        return float_d2(value)
    elif isinstance(value, tuple):
        return tuple(float_d2(v) if isinstance(v, float) else v for v in value)
    else:
        return value


class Metric:
    """A class to handle the name and the value of a metric."""
    def __init__(self, name, value):
        self.name = name
        self.value = convert_float_to_float_d2(value)

    def insert_key(self, key):
        """Insert the key to this metric."""
        self.key = key
        return self


class MetricNameProps:
    """A class keeping the information of metric name templates, descriptions,
    and statistic functions.
    """

    def __init__(self):
        self._init_raw_metrics_props()
        self._derive_metrics_props()

    def _init_raw_metrics_props(self):
        """Initialize raw_metrics_props.

        The raw_metrics_props is a dictionary from metric attribute to the
        corresponding metric properties. Take MAX_ERR as an example of metric
        attribute. Its metric properties include
          . metric name template: 'max error in {} (mm)'
            The metric name template will be expanded later. For example,
            with name variations ['x', 'y'], the above template will be
            expanded to:
                'max error in x (mm)', and
                'max error in y (mm)'
          . name variations: for example, ['x', 'y'] for MAX_ERR
          . metric name description: 'The max err of all samples'
          . metric note: None
          . the stat function used to calculate the statistics for the metric:
            we use max() to calculate MAX_ERR in x/y for linearity.

        About metric note:
            We show tuples instead of percentages if the metrics values are
            percentages. This is because such a tuple unveils more information
            (i.e., the values of the nominator and the denominator) than a mere
            percentage value. For examples,

            1f-click miss rate (%):
                one_finger_physical_click.center (20130710_063117) : (0, 1)
                  the tuple means (the number of missed clicks, total clicks)

            intervals > xxx ms (%)
                one_finger_tap.top_left (20130710_063117) : (1, 6)
                  the tuple means (the number of long intervals, total packets)
        """
        # stat_functions include: max, average,
        #                         pct_by_numbers, pct_by_missed_numbers,
        #                         pct_by_cases_neq, and pct_by_cases_less
        average = lambda lst: float(sum(lst)) / len(lst)
        get_sums = lambda lst: [sum(count) for count in zip(*lst)]
        _pct = lambda lst: float(lst[0]) / lst[1] * 100
        # The following lambda function is used to compute the missed pct of
        #
        #       '(clicks with correct finger IDs, actual clicks)'
        #
        # In some cases when the number of actual clicks is 0, there are no
        # missed finger IDs. So just return 0 for this special case to prevent
        # the devision by 0 error.
        _missed_pct = lambda lst: (float(lst[1] - lst[0]) / lst[1] * 100
                                   if lst[1] != 0 else 0)

        # pct by numbers: lst means [(incorrect number, total number), ...]
        #  E.g., lst = [(2, 10), (0, 10), (0, 10), (0, 10)]
        #  pct_by_numbers would be (2 + 0 + 0 + 0) / (10 + 10 + 10 + 10) * 100%
        pct_by_numbers = lambda lst: _pct(get_sums(lst))

        # pct by misssed numbers: lst means
        #                         [(actual number, expected number), ...]
        #  E.g., lst = [(0, 1), (1, 1), (1, 1), (1, 1)]
        #  pct_by_missed_numbers would be
        #       0 + 1 + 1 + 1  = 3
        #       1 + 1 + 1 + 1  = 4
        #       missed pct = (4 - 3) / 4 * 100% = 25%
        pct_by_missed_numbers = lambda lst: _missed_pct(get_sums(lst))

        # pct of incorrect cases in [(acutal_value, expected_value), ...]
        #   E.g., lst = [(1, 1), (0, 1), (1, 1), (1, 1)]
        #   pct_by_cases_neq would be 1 / 4 * 100%
        # This is used for CountTrackingIDValidator
        pct_by_cases_neq = lambda lst: _pct(
                [len([pair for pair in lst if pair[0] != pair[1]]), len(lst)])

        # pct of incorrect cases in [(acutal_value, min expected_value), ...]
        #   E.g., lst = [(3, 3), (4, 3)]
        #     pct_by_cases_less would be 0 / 2 * 100%
        #   E.g., lst = [(2, 3), (5, 3)]
        #     pct_by_cases_less would be 1 / 2 * 100%
        # This is used for CountPacketsIDValidator and PinchValidator
        pct_by_cases_less = lambda lst: _pct(
                [len([pair for pair in lst if pair[0] < pair[1]]), len(lst)])

        self.max_report_interval_str = '%.2f' % conf.max_report_interval

        # A dictionary from metric attribute to its properties:
        #    {metric_attr: (template,
        #                   name_variations,
        #                   description,
        #                   metric_note,
        #                   stat_func)
        #    }
        # Ordered by validators
        self.raw_metrics_props = {
            # Count Packets Validator
            'COUNT_PACKETS': (
                'pct of incorrect cases (%)--packets',
                None,
                'an incorrect case is one where a swipe has less than '
                    '3 packets reported',
                '(actual number of packets, expected number of packets)',
                pct_by_cases_less),
            # Count TrackingID Validator
            'TID': (
                'pct of incorrect cases (%)--tids',
                None,
                'an incorrect case is one where there are an incorrect number '
                    'of fingers detected',
                '(actual tracking IDs, expected tracking IDs)',
                pct_by_cases_neq),
            # Drag Latency Validator
            'AVG_LATENCY': (
                'average latency (ms)',
                None,
                'The average drag-latency in milliseconds',
                None,
                average),
            # Drumroll Validator
            'CIRCLE_RADIUS': (
                'circle radius (mm)',
                None,
                'the max radius of enclosing circles of tapping points',
                None,
                max),
            # Hysteresis Validator
            'MAX_INIT_GAP_RATIO': (
                'max init gap ratio',
                None,
                'the max ratio of dist(p0,p1) / dist(p1,p2)',
                None,
                max),
            'AVE_INIT_GAP_RATIO': (
                'ave init gap ratio',
                None,
                'the average ratio of dist(p0,p1) / dist(p1,p2)',
                None,
                average),
            # Linearity Validator
            'MAX_ERR': (
                'max error in {} (mm)',
                AXIS.LIST,
                'The max err of all samples',
                None,
                max),
            'RMS_ERR': (
                'rms error in {} (mm)',
                AXIS.LIST,
                'The mean of all rms means of all trials',
                None,
                average),
            # MTB Sanity Validator
            'MTB_SANITY_ERR': (
                'pct of MTB errors (%)',
                None,
                'pct of MTB errors',
                '(MTB errors, expected errors)',
                pct_by_cases_neq),
            # No Ghost Finger Validator
            'GHOST_FINGERS': (
                'pct of ghost fingers (%)',
                None,
                'pct of ghost fingers',
                '(ghost fingers, expected fingers)',
                pct_by_cases_neq),
            # Physical Click Validator
            'CLICK_CHECK_CLICK': (
                '{}f-click miss rate (%)',
                conf.fingers_physical_click,
                'the pct of finger IDs w/o a click',
                '(acutual clicks, expected clicks)',
                pct_by_missed_numbers),
            'CLICK_CHECK_TIDS': (
                '{}f-click w/o finger IDs (%)',
                conf.fingers_physical_click,
                'the pct of clicks w/o correct finger IDs',
                '(clicks with correct finger IDs, actual clicks)',
                pct_by_missed_numbers),
            # Pinch Validator
            'PINCH': (
                'pct of incorrect cases (%)--pinch',
                None,
                'pct of incorrect cases over total cases',
                '(actual relative motion (px), expected relative motion (px))',
                pct_by_cases_less),
            # Range Validator
            'RANGE': (
                '{} edge not reached (mm)',
                ['left', 'right', 'top', 'bottom'],
                'Min unreachable distance',
                None,
                max),
            # Report Rate Validator
            'LONG_INTERVALS': (
                'pct of large intervals (%)',
                None,
                'pct of intervals larger than expected',
                '(the number of long intervals, total packets)',
                pct_by_numbers),
            'AVE_TIME_INTERVAL': (
                'average time interval (ms)',
                None,
                'the average of report intervals',
                None,
                average),
            'MAX_TIME_INTERVAL': (
                'max time interval (ms)',
                None,
                'the max report interval',
                None,
                max),
            # Stationary Finger Validator
            'MAX_DISTANCE': (
                'max distance (mm)',
                None,
                'max distance of any two points from any run',
                None,
                max),
        }

        # Set the metric attribute to its template
        #   E.g., self.MAX_ERR = 'max error in {} (mm)'
        for key, props in self.raw_metrics_props.items():
            template = props[0]
            setattr(self, key, template)

    def _derive_metrics_props(self):
        """Expand the metric name templates to the metric names, and then
        derive the expanded metrics_props.

        In _init_raw_metrics_props():
            The raw_metrics_props is defined as:
                'MAX_ERR': (
                    'max error in {} (mm)',             # template
                    ['x', 'y'],                         # name variations
                    'The max err of all samples',       # description
                    max),                               # stat_func
                ...

            By expanding the template with its corresponding name variations,
            the names related with MAX_ERR will be:
                'max error in x (mm)', and
                'max error in y (mm)'

        Here we are going to derive metrics_props as:
                metrics_props = {
                    'max error in x (mm)':
                        MetricProps('The max err of all samples', max),
                    ...
                }
        """
        self.metrics_props = {}
        for raw_props in self.raw_metrics_props.values():
            template, name_variations, description, note, stat_func = raw_props
            metric_props = MetricProps(description, note, stat_func)
            if name_variations:
                # Expand the template with every variations.
                #   E.g., template = 'max error in {} (mm)' is expanded to
                #         name = 'max error in x (mm)'
                for variation in name_variations:
                    name = template.format(variation)
                    self.metrics_props[name] = metric_props
            else:
                # Otherwise, the template is already the name.
                #   E.g., the template 'max distance (mm)' is same as the name.
                self.metrics_props[template] = metric_props


class ValidatorLog:
    """A class handling the logs reported by validators."""
    def __init__(self):
        self.name = None
        self.details = []
        self.criteria = None
        self.score = None
        self.metrics = []
        self.error = None

    def reset(self):
        """Reset all attributes."""
        self.details = []
        self.score = None
        self.metrics = []
        self.error = None

    def insert_details(self, msg):
        """Insert a msg into the details."""
        self.details.append(msg)


class GestureLog:
    """A class handling the logs related with a gesture."""
    def __init__(self):
        self.name = None
        self.variation = None
        self.prompt = None
        self.vlogs = []


class RoundLog:
    """Manipulate the test result log generated in a single round."""
    def __init__(self, test_version, fw=None, round_name=None):
        self._test_version = test_version
        self._fw = fw
        self._round_name = round_name
        self._glogs = []

    def dump(self, filename):
        """Dump the log to the specified filename."""
        try:
            with open(filename, 'w') as log_file:
                pickle.dump([self._fw, self._round_name, self._test_version,
                             self._glogs], log_file)
        except Exception, e:
            msg = 'Error in dumping to the log file (%s): %s' % (filename, e)
            print_and_exit(msg)

    @staticmethod
    def load(filename):
        """Load the log from the pickle file."""
        try:
            with open(filename) as log_file:
                return pickle.load(log_file)
        except Exception, e:
            msg = 'Error in loading the log file (%s): %s' % (filename, e)
            print_and_exit(msg)

    def insert_glog(self, glog):
        """Insert the gesture log into the round log."""
        if glog.vlogs:
            self._glogs.append(glog)


class StatisticsScores:
    """A statistics class to compute the average, ssd, and count of
    aggregate scores.
    """
    def __init__(self, scores):
        self.all_data = ()
        if scores:
            self.average = np.average(np.array(scores))
            self.ssd = _calc_sample_standard_deviation(scores)
            self.count = len(scores)
            self.all_data = (self.average, self.ssd, self.count)


class StatisticsMetrics:
    """A statistics class to compute the statistics including the min, max, or
    average of aggregate metrics.
    """

    def __init__(self, metrics):
        """Collect all values for every metric.

        @param metrics: a list of Metric objects.
        """
        # metrics_values: the raw metrics values
        self.metrics_values = defaultdict(list)
        self.metrics_dict = defaultdict(list)
        for metric in metrics:
            self.metrics_values[metric.name].append(metric.value)
            self.metrics_dict[metric.name].append(metric)

        # Calculate the statistics of metrics using corresponding stat functions
        self._calc_statistics(MetricNameProps().metrics_props)

    def _calc_statistics(self, metrics_props):
        """Calculate the desired statistics for every metric.

        @param metrics_props: a dictionary mapping a metric name to a
                metric props including the description and stat_func
        """
        self.metrics_props = metrics_props
        self.stats_values = {}
        for metric_name, values in self.metrics_values.items():
            assert metric_name in metrics_props, (
                    'The metric name "%s" cannot be found.' % metric_name)
            stat_func = metrics_props[metric_name].stat_func
            self.stats_values[metric_name] = stat_func(values)


class TestResult:
    """A class includes the statistics of the score and the metrics."""
    def __init__(self, scores, metrics):
        self.stat_scores = StatisticsScores(scores)
        self.stat_metrics = StatisticsMetrics(metrics)


class SimpleTable:
    """A very simple data table."""
    def __init__(self):
        """This initializes a simple table."""
        self._table = defaultdict(list)

    def insert(self, key, value):
        """Insert a row. If the key exists already, the value is appended."""
        self._table[key].append(value)
        debug_print('    key: %s' % str(key))

    def search(self, key):
        """Search rows with the specified key.

        A key is a list of attributes.
        If any attribute is None, it means no need to match this attribute.
        """
        match = lambda i, j: i == j or j is None
        return filter(lambda (k, vlog): all(map(match, k, key)),
                      self._table.items())

    def items(self):
        """Return the table items."""
        return self._table.items()


class SummaryLog:
    """A class to manipulate the summary logs.

    A summary log may consist of result logs of different firmware versions
    where every firmware version may consist of multiple rounds.
    """
    def __init__(self, log_dir, segment_weights, validator_weights,
                 individual_round_flag, debug_flag):
        self.log_dir = log_dir
        self.segment_weights = segment_weights
        self.validator_weights = validator_weights
        self.individual_round_flag = individual_round_flag
        _setup_debug(debug_flag)
        self._read_logs()
        self.ext_validator_weights = {}
        for fw, validators in self.fw_validators.items():
            self.ext_validator_weights[fw] = \
                    self._compute_extended_validator_weight(validators)

    def _get_firmware_version(self, filename):
        """Get the firmware version from the given filename."""
        return filename.split('-')[2]

    def _read_logs(self):
        """Read the result logs in the specified log directory."""
        # Get logs in the log_dir or its sub-directories.
        log_filenames = glob.glob(os.path.join(self.log_dir, '*.log'))
        if not log_filenames:
            log_filenames = glob.glob(os.path.join(self.log_dir, '*', '*.log'))

        if not log_filenames:
            err_msg = 'Error: no log files in the test result directory: %s'
            print_and_exit(err_msg % self.log_dir)

        self.log_table = SimpleTable()
        self.fws = set()
        self.gestures = set()
        # fw_validators keeps track of the validators of every firmware
        self.fw_validators = defaultdict(set)

        for i, log_filename in enumerate(log_filenames):
            round_no = i if self.individual_round_flag else None
            self._add_round_log(log_filename, round_no)

        # Convert set to list below
        self.fws = sorted(list(self.fws))
        self.gestures = sorted(list(self.gestures))
        # Construct validators by taking the union of the validators of
        # all firmwares.
        self.validators = sorted(list(set.union(*self.fw_validators.values())))

        for fw in self.fws:
            self.fw_validators[fw] = sorted(list(self.fw_validators[fw]))

    def _add_round_log(self, log_filename, round_no):
        """Add the round log, decompose the validator logs, and build
        a flat summary log.
        """
        log_data = RoundLog.load(log_filename)
        if len(log_data) == 3:
            fw, round_name, glogs = log_data
            self.test_version = 'test_version: NA'
        elif len(log_data) == 4:
            fw, round_name, self.test_version, glogs = log_data
        else:
            print 'Error: the log format is unknown.'
            sys.exit(1)

        if round_no is not None:
            fw = '%s_%d' % (fw, round_no)
        self.fws.add(fw)
        debug_print('  fw(%s) round(%s)' % (fw, round_name))

        # Iterate through every gesture_variation of the round log,
        # and generate a flat dictionary of the validator logs.
        for glog in glogs:
            self.gestures.add(glog.name)
            for vlog in glog.vlogs:
                self.fw_validators[fw].add(vlog.name)
                key = (fw, round_name, glog.name, glog.variation, vlog.name)
                self.log_table.insert(key, vlog)

    def _compute_extended_validator_weight(self, validators):
        """Compute extended validator weight from validator weight and segment
        weight. The purpose is to merge the weights of split validators, e.g.
        Linearity(*)Validator, so that their weights are not counted multiple
        times.

        Example:
          validators = ['CountTrackingIDValidator',
                        'Linearity(BothEnds)Validator',
                        'Linearity(Middle)Validator',
                        'NoGapValidator']

          Note that both names of the validators
                'Linearity(BothEnds)Validator' and
                'Linearity(Middle)Validator'
          are created at run time from LinearityValidator and use
          the relative weights defined by segment_weights.

          validator_weights = {'CountTrackingIDValidator': 12,
                               'LinearityValidator': 10,
                               'NoGapValidator': 10}

          segment_weights = {'Middle': 0.7,
                             'BothEnds': 0.3}

          split_validator = {'Linearity': ['BothEnds', 'Middle'],}

          adjusted_weight of Lineary(*)Validator:
            Linearity(BothEnds)Validator = 0.3 / (0.3 + 0.7) * 10 = 3
            Linearity(Middle)Validator =   0.7 / (0.3 + 0.7) * 10 = 7

          extended_validator_weights: {'CountTrackingIDValidator': 12,
                                       'Linearity(BothEnds)Validator': 3,
                                       'Linearity(Middle)Validator': 7,
                                       'NoGapValidator': 10}
        """
        extended_validator_weights = {}
        split_validator = {}

        # Copy the base validator weight into extended_validator_weights.
        # For the split validators, collect them in split_validator.
        for v in validators:
            base_name, segment = val.get_base_name_and_segment(v)
            if segment is None:
                # It is a base validator. Just copy it into the
                # extended_validaotr_weight dict.
                extended_validator_weights[v] = self.validator_weights[v]
            else:
                # It is a derived validator, e.g., Linearity(BothEnds)Validator
                # Needs to compute its adjusted weight.

                # Initialize the split_validator for this base_name if not yet.
                if split_validator.get(base_name) is None:
                    split_validator[base_name] = []

                # Append this segment name so that we know all segments for
                # the base_name.
                split_validator[base_name].append(segment)

        # Compute the adjusted weight for split_validator
        for base_name in split_validator:
            name = val.get_validator_name(base_name)
            weight_list = [self.segment_weights[segment]
                           for segment in split_validator[base_name]]
            weight_sum = sum(weight_list)
            for segment in split_validator[base_name]:
                derived_name = val.get_derived_name(name, segment)
                adjusted_weight = (self.segment_weights[segment] / weight_sum *
                                   self.validator_weights[name])
                extended_validator_weights[derived_name] = adjusted_weight

        return extended_validator_weights

    def get_result(self, fw=None, round=None, gesture=None, variation=None,
                   validators=None):
        """Get the result statistics of a validator which include both
        the score and the metrics.

        If validators is a list, every validator in the list is used to query
        the log table, and all results are merged to get the final result.
        For example, both StationaryFingerValidator and StationaryTapValidator
        inherit StationaryValidator. The results of those two extended classes
        will be merged into StationaryValidator.
        """
        if not isinstance(validators, list):
            validators = [validators,]

        rows = []
        for validator in validators:
            key = (fw, round, gesture, variation, validator)
            rows.extend(self.log_table.search(key))

        scores = [vlog.score for _key, vlogs in rows for vlog in vlogs]
        metrics = [metric.insert_key(_key) for _key, vlogs in rows
                                               for vlog in vlogs
                                                   for metric in vlog.metrics]
        return TestResult(scores, metrics)

    def get_final_weighted_average(self):
        """Calculate the final weighted average."""
        weighted_average = {}
        # for fw in self.fws:
        for fw, validators in self.fw_validators.items():
            scores = [self.get_result(fw=fw, validators=val).stat_scores.average
                      for val in validators]
            _, weights = zip(*sorted(self.ext_validator_weights[fw].items()))
            weighted_average[fw] = np.average(scores, weights=weights)
        return weighted_average
