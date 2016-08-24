#!/usr/bin/python
"""
Usage: ./cron_scripts/log_distiller.py job_id path_to_logfile
    If the job_id is a suite it will find all subjobs.
You need to change the location of the log it will parse.
The job_id needs to be in the afe database.
"""
import abc
import datetime
import os
import re
import pprint
import subprocess
import sys
import time

import common
from autotest_lib.server import frontend


LOGFIE = './logs/scheduler.log.2014-04-17-16.51.47'
# logfile name format: scheduler.log.2014-02-14-18.10.56
time_format = '%Y-%m-%d-%H.%M.%S'
logfile_regex = r'scheduler.log.([0-9,.,-]+)'
logdir = os.path.join('/usr/local/autotest', 'logs')

class StateMachineViolation(Exception):
    pass


class LogLineException(Exception):
    pass


def should_process_log(time_str, time_format, cutoff_days=7):
    """Returns true if the logs was created after cutoff days.

    @param time_str: A string representing the time.
        eg: 2014-02-14-18.10.56
    @param time_format: A string representing the format of the time string.
        ref: http://docs.python.org/2/library/datetime.html#strftime-strptime-behavior
    @param cutoff_days: Int representind the cutoff in days.

    @return: Returns True if time_str has aged more than cutoff_days.
    """
    log_time = datetime.datetime.strptime(time_str, time_format)
    now = datetime.datetime.strptime(time.strftime(time_format), time_format)
    cutoff = now - datetime.timedelta(days=cutoff_days)
    return log_time < cutoff


def apply_regex(regex, line):
    """Simple regex applicator.

    @param regex: Regex to apply.
    @param line: The line to apply regex on.

    @return: A tuple with the matching groups, if there was a match.
    """
    log_match  = re.match(regex, line)
    if log_match:
        return log_match.groups()


class StateMachineParser(object):
    """Abstract class that enforces state transition ordering.

    Classes inheriting from StateMachineParser need to define an
    expected_transitions dictionary. The SMP will pop 'to' states
    from the dictionary as they occur, so you cannot same state transitions
    unless you specify 2 of them.
    """
    __metaclass__ = abc.ABCMeta


    @abc.abstractmethod
    def __init__(self):
        self.visited_states = []
        self.expected_transitions = {}


    def advance_state(self, from_state, to_state):
        """Checks that a transition is valid.

        @param from_state: A string representind the state the host is leaving.
        @param to_state: The state The host is going to, represented as a string.

        @raises LogLineException: If an invalid state transition was
            detected.
        """
        # TODO: Updating to the same state is a waste of bw.
        if from_state and from_state == to_state:
            return ('Updating to the same state is a waste of BW: %s->%s' %
                    (from_state, to_state))
            return

        if (from_state in self.expected_transitions and
            to_state in self.expected_transitions[from_state]):
            self.expected_transitions[from_state].remove(to_state)
            self.visited_states.append(to_state)
        else:
            return (from_state, to_state)


class SingleJobHostSMP(StateMachineParser):
    def __init__(self):
        self.visited_states = []
        self.expected_transitions = {
                'Ready': ['Resetting', 'Verifying', 'Pending', 'Provisioning'],
                'Resetting': ['Ready', 'Provisioning'],
                'Pending': ['Running'],
                'Provisioning': ['Repairing'],
                'Running': ['Ready']
        }


    def check_transitions(self, hostline):
        if hostline.line_info['field'] == 'status':
            self.advance_state(hostline.line_info['state'],
                    hostline.line_info['value'])


class SingleJobHqeSMP(StateMachineParser):
    def __init__(self):
        self.visited_states = []
        self.expected_transitions = {
                'Queued': ['Starting', 'Resetting', 'Aborted'],
                'Resetting': ['Pending', 'Provisioning'],
                'Provisioning': ['Pending', 'Queued', 'Repairing'],
                'Pending': ['Starting'],
                'Starting': ['Running'],
                'Running': ['Gathering', 'Parsing'],
                'Gathering': ['Parsing'],
                'Parsing': ['Completed', 'Aborted']
        }


    def check_transitions(self, hqeline):
        invalid_states = self.advance_state(
                hqeline.line_info['from_state'], hqeline.line_info['to_state'])
        if not invalid_states:
            return

        # Deal with repair.
        if (invalid_states[0] == 'Queued' and
            'Running' in self.visited_states):
            raise StateMachineViolation('Unrecognized state transition '
                    '%s->%s, expected transitions are %s' %
                    (invalid_states[0], invalid_states[1],
                     self.expected_transitions))


class LogLine(object):
    """Line objects.

    All classes inheriting from LogLine represent a line of some sort.
    A line is responsible for parsing itself, and invoking an SMP to
    validate state transitions. A line can be part of several state machines.
    """
    line_format = '%s'


    def __init__(self, state_machine_parsers):
        """
        @param state_machine_parsers: A list of smp objects to use to validate
            state changes on these types of lines..
        """
        self.smps = state_machine_parsers

        # Because, this is easier to flush.
        self.line_info = {}


    def parse_line(self, line):
        """Apply a line regex and save any information the parsed line contains.

        @param line: A string representing a line.
        """
        # Regex for all the things.
        line_rgx = '(.*)'
        parsed_line = apply_regex(line_rgx, line)
        if parsed_line:
            self.line_info['line'] = parsed_line[0]


    def flush(self):
        """Call any state machine parsers, persist line info if needed.
        """
        for smp in self.smps:
            smp.check_transitions(self)
        # TODO: persist this?
        self.line_info={}


    def format_line(self):
        try:
            return self.line_format % self.line_info
        except KeyError:
            return self.line_info['line']


class TimeLine(LogLine):
    """Filters timestamps for scheduler logs.
    """

    def parse_line(self, line):
        super(TimeLine, self).parse_line(line)

        # Regex for isolating the date and time from scheduler logs, eg:
        # 02/16 16:04:36.573 INFO |scheduler_:0574|...
        line_rgx = '([0-9,/,:,., ]+)(.*)'
        parsed_line = apply_regex(line_rgx, self.line_info['line'])
        if parsed_line:
            self.line_info['time'] = parsed_line[0]
            self.line_info['line'] = parsed_line[1]


class HostLine(TimeLine):
    """Manages hosts line parsing.
    """
    line_format = (' \t\t %(time)s %(host)s, currently in %(state)s, '
                'updated %(field)s->%(value)s')


    def record_state_transition(self, line):
        """Apply the state_transition_rgx to a line and record state changes.

        @param line: The line we're expecting to contain a state transition.
        """
        state_transition_rgx = ".* ([a-zA-Z]+) updating {'([a-zA-Z]+)': ('[a-zA-Z]+'|[0-9])}.*"
        match = apply_regex(state_transition_rgx, line)
        if match:
            self.line_info['state'] = match[0]
            self.line_info['field'] = match[1]
            self.line_info['value'] = match[2].replace("'", "")


    def parse_line(self, line):
        super(HostLine, self).parse_line(line)

        # Regex for getting host status. Eg:
        # 172.22.4 in Running updating {'status': 'Running'}
        line_rgx = '.*Host (([0-9,.,a-z,-]+).*)'
        parsed_line = apply_regex(line_rgx, self.line_info['line'])
        if parsed_line:
            self.line_info['line'] = parsed_line[0]
            self.line_info['host'] = parsed_line[1]
            self.record_state_transition(self.line_info['line'])
            return self.format_line()


class HQELine(TimeLine):
    """Manages HQE line parsing.
    """
    line_format = ('%(time)s %(hqe)s, currently in %(from_state)s, '
            'updated to %(to_state)s. Flags: %(flags)s')


    def record_state_transition(self, line):
        """Apply the state_transition_rgx to a line and record state changes.

        @param line: The line we're expecting to contain a state transition.
        """
        # Regex for getting hqe status. Eg:
        # status:Running [active] -> Gathering
        state_transition_rgx = ".*status:([a-zA-Z]+)( \[[a-z\,]+\])? -> ([a-zA-Z]+)"
        match = apply_regex(state_transition_rgx, line)
        if match:
            self.line_info['from_state'] = match[0]
            self.line_info['flags'] = match[1]
            self.line_info['to_state'] = match[2]


    def parse_line(self, line):
        super(HQELine, self).parse_line(line)
        line_rgx = r'.*\| HQE: (([0-9]+).*)'
        parsed_line = apply_regex(line_rgx, self.line_info['line'])
        if parsed_line:
            self.line_info['line'] = parsed_line[0]
            self.line_info['hqe'] = parsed_line[1]
            self.record_state_transition(self.line_info['line'])
            return self.format_line()


class LogCrawler(object):
    """Crawl logs.

    Log crawlers are meant to apply some basic preprocessing to a log, and crawl
    the output validating state changes. They manage line and state machine
    creation. The initial filtering applied to the log needs to be grab all lines
    that match an action, such as the running of a job.
    """

    def __init__(self, log_name):
        self.log = log_name
        self.filter_command = 'cat %s' % log_name


    def preprocess_log(self):
        """Apply some basic filtering to the log.
        """
        proc = subprocess.Popen(self.filter_command,
                shell=True, stdout=subprocess.PIPE)
        out, err = proc.communicate()
        return out


class SchedulerLogCrawler(LogCrawler):
    """A log crawler for the scheduler logs.

    This crawler is only capable of processing information about a single job.
    """

    def __init__(self, log_name, **kwargs):
        super(SchedulerLogCrawler, self).__init__(log_name)
        self.job_id = kwargs['job_id']
        self.line_processors = [HostLine([SingleJobHostSMP()]),
                HQELine([SingleJobHqeSMP()])]
        self.filter_command = ('%s | grep "for job: %s"' %
                (self.filter_command, self.job_id))


    def parse_log(self):
        """Parse each line of the preprocessed log output.

        Pass each line through each possible line_processor. The one that matches
        will populate itself, call flush, this will walk the state machine of that
        line to the next step.
        """
        out = self.preprocess_log()
        response = []
        for job_line in out.split('\n'):
            parsed_line = None
            for processor in self.line_processors:
                line = processor.parse_line(job_line)
                if line and parsed_line:
                    raise LogLineException('Multiple Parsers claiming the line %s: '
                            'previous parsing: %s, current parsing: %s ' %
                            (job_line, parsed_line, line))
                elif line:
                    parsed_line = line
                    try:
                        processor.flush()
                    except StateMachineViolation as e:
                        response.append(str(e))
                        raise StateMachineViolation(response)
            response.append(parsed_line if parsed_line else job_line)
        return response


def process_logs():
    if len(sys.argv) < 2:
        print ('Usage: ./cron_scripts/log_distiller.py 0 8415620 '
               'You need to change the location of the log it will parse.'
                'The job_id needs to be in the afe database.')
        sys.exit(1)

    job_id = int(sys.argv[1])
    rpc = frontend.AFE()
    suite_jobs = rpc.run('get_jobs', id=job_id)
    if not suite_jobs[0]['parent_job']:
        suite_jobs = rpc.run('get_jobs', parent_job=job_id)
    try:
        logfile = sys.argv[2]
    except Exception:
        logfile = LOGFILE

    for job in suite_jobs:
        log_crawler = SchedulerLogCrawler(logfile, job_id=job['id'])
        for line in log_crawler.parse_log():
            print line
    return


if __name__ == '__main__':
    process_logs()
