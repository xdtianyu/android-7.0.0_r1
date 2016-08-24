# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Library to run fio scripts.

fio_runner launch fio and collect results.
The output dictionary can be add to autotest keyval:
        results = {}
        results.update(fio_util.fio_runner(job_file, env_vars))
        self.write_perf_keyval(results)

Decoding class can be invoked independently.

"""

import json, logging, re, utils

class fio_graph_generator():
    """
    Generate graph from fio log that created when specified these options.
    - write_bw_log
    - write_iops_log
    - write_lat_log

    The following limitations apply
    - Log file name must be in format jobname_testpass
    - Graph is generate using Google graph api -> Internet require to view.
    """

    html_head = """
<html>
  <head>
    <script type="text/javascript" src="https://www.google.com/jsapi"></script>
    <script type="text/javascript">
      google.load("visualization", "1", {packages:["corechart"]});
      google.setOnLoadCallback(drawChart);
      function drawChart() {
"""

    html_tail = """
        var chart_div = document.getElementById('chart_div');
        var chart = new google.visualization.ScatterChart(chart_div);
        chart.draw(data, options);
      }
    </script>
  </head>
  <body>
    <div id="chart_div" style="width: 100%; height: 100%;"></div>
  </body>
</html>
"""

    h_title = { True: 'Percentile', False: 'Time (s)' }
    v_title = { 'bw'  : 'Bandwidth (KB/s)',
                'iops': 'IOPs',
                'lat' : 'Total latency (us)',
                'clat': 'Completion latency (us)',
                'slat': 'Submission latency (us)' }
    graph_title = { 'bw'  : 'bandwidth',
                    'iops': 'IOPs',
                    'lat' : 'total latency',
                    'clat': 'completion latency',
                    'slat': 'submission latency' }

    test_name = ''
    test_type = ''
    pass_list = ''

    @classmethod
    def _parse_log_file(cls, file_name, pass_index, pass_count, percentile):
        """
        Generate row for google.visualization.DataTable from one log file.
        Log file is the one that generated using write_{bw,lat,iops}_log
        option in the FIO job file.

        The fio log file format is  timestamp, value, direction, blocksize
        The output format for each row is { c: list of { v: value} }

        @param file_name:  log file name to read data from
        @param pass_index: index of current run pass
        @param pass_count: number of all test run passes
        @param percentile: flag to use percentile as key instead of timestamp

        @return: list of data rows in google.visualization.DataTable format
        """
        # Read data from log
        with open(file_name, 'r') as f:
            data = []

            for line in f.readlines():
                if not line:
                    break
                t, v, _, _ = [int(x) for x in line.split(', ')]
                data.append([t / 1000.0, v])

        # Sort & calculate percentile
        if percentile:
            data.sort(key=lambda x: x[1])
            l = len(data)
            for i in range(l):
                data[i][0] = 100 * (i + 0.5) / l

        # Generate the data row
        all_row = []
        row = [None] * (pass_count + 1)
        for d in data:
            row[0] = {'v' : '%.3f' % d[0]}
            row[pass_index + 1] = {'v': d[1]}
            all_row.append({'c': row[:]})

        return all_row

    @classmethod
    def _gen_data_col(cls, pass_list, percentile):
        """
        Generate col for google.visualization.DataTable

        The output format is list of dict of label and type. In this case,
        type is always number.

        @param pass_list:  list of test run passes
        @param percentile: flag to use percentile as key instead of timestamp

        @return: list of column in google.visualization.DataTable format
        """
        if percentile:
            col_name_list = ['percentile'] + [p[0] for p in pass_list]
        else:
            col_name_list = ['time'] + [p[0] for p in pass_list]

        return [{'label': name, 'type': 'number'} for name in col_name_list]

    @classmethod
    def _gen_data_row(cls, test_type, pass_list, percentile):
        """
        Generate row for google.visualization.DataTable by generate all log
        file name and call _parse_log_file for each file

        @param test_type: type of value collected for current test. i.e. IOPs
        @param pass_list: list of run passes for current test
        @param percentile: flag to use percentile as key instead of timestamp

        @return: list of data rows in google.visualization.DataTable format
        """
        all_row = []
        pass_count = len(pass_list)
        for pass_index, log_file_name in enumerate([p[1] for p in pass_list]):
            all_row.extend(cls._parse_log_file(log_file_name, pass_index,
                                                pass_count, percentile))
        return all_row

    @classmethod
    def _write_data(cls, f, test_type, pass_list, percentile):
        """
        Write google.visualization.DataTable object to output file.
        https://developers.google.com/chart/interactive/docs/reference

        @param f: html file to update
        @param test_type: type of value collected for current test. i.e. IOPs
        @param pass_list: list of run passes for current test
        @param percentile: flag to use percentile as key instead of timestamp
        """
        col = cls._gen_data_col(pass_list, percentile)
        row = cls._gen_data_row(test_type, pass_list, percentile)
        data_dict = {'cols' : col, 'rows' : row}

        f.write('var data = new google.visualization.DataTable(')
        json.dump(data_dict, f)
        f.write(');\n')

    @classmethod
    def _write_option(cls, f, test_name, test_type, percentile):
        """
        Write option to render scatter graph to output file.
        https://google-developers.appspot.com/chart/interactive/docs/gallery/scatterchart

        @param test_name: name of current workload. i.e. randwrite
        @param test_type: type of value collected for current test. i.e. IOPs
        @param percentile: flag to use percentile as key instead of timestamp
        """
        option = {'pointSize': 1}
        if percentile:
            option['title'] = ('Percentile graph of %s for %s workload' %
                               (cls.graph_title[test_type], test_name))
        else:
            option['title'] = ('Graph of %s for %s workload over time' %
                               (cls.graph_title[test_type], test_name))

        option['hAxis'] = {'title': cls.h_title[percentile]}
        option['vAxis'] = {'title': cls.v_title[test_type]}

        f.write('var options = ')
        json.dump(option, f)
        f.write(';\n')

    @classmethod
    def _write_graph(cls, test_name, test_type, pass_list, percentile=False):
        """
        Generate graph for test name / test type

        @param test_name: name of current workload. i.e. randwrite
        @param test_type: type of value collected for current test. i.e. IOPs
        @param pass_list: list of run passes for current test
        @param percentile: flag to use percentile as key instead of timestamp
        """
        logging.info('fio_graph_generator._write_graph %s %s %s',
                     test_name, test_type, str(pass_list))


        if percentile:
            out_file_name = '%s_%s_percentile.html' % (test_name, test_type)
        else:
            out_file_name = '%s_%s.html' % (test_name, test_type)

        with open(out_file_name, 'w') as f:
            f.write(cls.html_head)
            cls._write_data(f, test_type, pass_list, percentile)
            cls._write_option(f, test_name, test_type, percentile)
            f.write(cls.html_tail)

    def __init__(self, test_name, test_type, pass_list):
        """
        @param test_name: name of current workload. i.e. randwrite
        @param test_type: type of value collected for current test. i.e. IOPs
        @param pass_list: list of run passes for current test
        """
        self.test_name = test_name
        self.test_type = test_type
        self.pass_list = pass_list

    def run(self):
        """
        Run the graph generator.
        """
        self._write_graph(self.test_name, self.test_type, self.pass_list, False)
        self._write_graph(self.test_name, self.test_type, self.pass_list, True)


def fio_parse_dict(d, prefix):
    """
    Parse fio json dict

    Recursively flaten json dict to generate autotest perf dict

    @param d: input dict
    @param prefix: name prefix of the key
    """

    # No need to parse something that didn't run such as read stat in write job.
    if 'io_bytes' in d and d['io_bytes'] == 0:
        return {}

    results = {}
    for k, v in d.items():

        # remove >, >=, <, <=
        for c in '>=<':
            k = k.replace(c, '')

        key = prefix + '_' + k

        if type(v) is dict:
            results.update(fio_parse_dict(v, key))
        else:
            results[key] = v
    return results


def fio_parser(lines, prefix=None):
    """
    Parse the json fio output

    This collects all metrics given by fio and labels them according to unit
    of measurement and test case name.

    @param lines: text output of json fio output.
    @param prefix: prefix for result keys.
    """
    results = {}
    fio_dict = json.loads(lines)

    if prefix:
        prefix = prefix + '_'
    else:
        prefix = ''

    results[prefix + 'fio_version'] = fio_dict['fio version']

    if 'disk_util' in fio_dict:
        results.update(fio_parse_dict(fio_dict['disk_util'][0],
                                      prefix + 'disk'))

    for job in fio_dict['jobs']:
        job_prefix = '_' + prefix + job['jobname']
        job.pop('jobname')


        for k, v in job.iteritems():
            results.update(fio_parse_dict({k:v}, job_prefix))

    return results

def fio_generate_graph():
    """
    Scan for fio log file in output directory and send data to generate each
    graph to fio_graph_generator class.
    """
    log_types = ['bw', 'iops', 'lat', 'clat', 'slat']

    # move fio log to result dir
    for log_type in log_types:
        logging.info('log_type %s', log_type)
        logs = utils.system_output('ls *_%s.*log' % log_type, ignore_status=True)
        if not logs:
            continue

        pattern = r"""(?P<jobname>.*)_                    # jobname
                      ((?P<runpass>p\d+)_|)               # pass
                      (?P<type>bw|iops|lat|clat|slat)     # type
                      (.(?P<thread>\d+)|)                 # thread id for newer fio.
                      .log
                   """
        matcher = re.compile(pattern, re.X)

        pass_list = []
        current_job = ''

        for log in logs.split():
            match = matcher.match(log)
            if not match:
                logging.warn('Unknown log file %s', log)
                continue

            jobname = match.group('jobname')
            runpass = match.group('runpass') or '1'
            if match.group('thread'):
                runpass += '_' +  match.group('thread')

            # All files for particular job name are group together for create
            # graph that can compare performance between result from each pass.
            if jobname != current_job:
                if pass_list:
                    fio_graph_generator(current_job, log_type, pass_list).run()
                current_job = jobname
                pass_list = []
            pass_list.append((runpass, log))

        if pass_list:
            fio_graph_generator(current_job, log_type, pass_list).run()


        cmd = 'mv *_%s.*log results' % log_type
        utils.run(cmd, ignore_status=True)
        utils.run('mv *.html results', ignore_status=True)


def fio_runner(test, job, env_vars,
               name_prefix=None,
               graph_prefix=None):
    """
    Runs fio.

    Build a result keyval and performence json.
    The JSON would look like:
    {"description": "<name_prefix>_<modle>_<size>G",
     "graph": "<graph_prefix>_1m_write_wr_lat_99.00_percent_usec",
     "higher_is_better": false, "units": "us", "value": "xxxx"}
    {...


    @param test: test to upload perf value
    @param job: fio config file to use
    @param env_vars: environment variable fio will substituete in the fio
        config file.
    @param name_prefix: prefix of the descriptions to use in chrome perfi
        dashboard.
    @param graph_prefix: prefix of the graph name in chrome perf dashboard
        and result keyvals.
    @return fio results.

    """

    # running fio with ionice -c 3 so it doesn't lock out other
    # processes from the disk while it is running.
    # If you want to run the fio test for performance purposes,
    # take out the ionice and disable hung process detection:
    # "echo 0 > /proc/sys/kernel/hung_task_timeout_secs"
    # -c 3 = Idle
    # Tried lowest priority for "best effort" but still failed
    ionice = 'ionice -c 3'
    options = ['--output-format=json']
    fio_cmd_line = ' '.join([env_vars, ionice, 'fio',
                             ' '.join(options),
                             '"' + job + '"'])
    fio = utils.run(fio_cmd_line)

    logging.debug(fio.stdout)

    fio_generate_graph()

    filename = re.match('.*FILENAME=(?P<f>[^ ]*)', env_vars).group('f')
    diskname = utils.get_disk_from_filename(filename)

    if diskname:
        model = utils.get_disk_model(diskname)
        size = utils.get_disk_size_gb(diskname)
        perfdb_name = '%s_%dG' % (model, size)
    else:
        perfdb_name = filename.replace('/', '_')

    if name_prefix:
        perfdb_name = name_prefix + '_' + perfdb_name

    result = fio_parser(fio.stdout, prefix=name_prefix)
    if not graph_prefix:
        graph_prefix = ''

    for k, v in result.iteritems():
        # Remove the prefix for value, and replace it the graph prefix.
        if name_prefix:
            k = k.replace('_' + name_prefix, graph_prefix)

        # Make graph name to be same as the old code.
        if k.endswith('bw'):
            test.output_perf_value(description=perfdb_name, graph=k, value=v,
                                   units='KB_per_sec', higher_is_better=True)
        elif k.rstrip('0').endswith('clat_percentile_99.'):
            test.output_perf_value(description=perfdb_name, graph=k, value=v,
                                   units='us', higher_is_better=False)
    return result
