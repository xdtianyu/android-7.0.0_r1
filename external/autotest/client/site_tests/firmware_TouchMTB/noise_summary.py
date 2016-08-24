'''This file summarizes the results from an extended noise test.
It uses the HTML report log generated at the end of the test as input.
It will output a summary in the same directory as the input report log,
as well as a graphic representation.

Usage: python noise_summary.py report.html
'''

from HTMLParser import HTMLParser
import matplotlib.pyplot as plt
import os.path
import re
import sys

# Constants
CORRECT_NUM_FINGERS = 1
CORRECT_MAX_DISTANCE = 1.0
FINGERS_INDEX = 0
DISTANCE_INDEX = 1


# A parser to consolidate the data in the html report
class ParseReport(HTMLParser):
    def __init__(self, num_iterations):
        HTMLParser.__init__(self)
        self.curr_freq = 0
        self.last_freq = self.curr_freq
        self.curr_dict_index = 0
        self.miscounted_fingers = 0
        self.over_distance = 0
        self.num_iterations = num_iterations
        self.data_dict_list = []

        for x in range(0, self.num_iterations):
            # Each dictionary in the list represents
            # one iteration of data
            self.data_dict_list.append({})

    # extracts the frequency from a line in the html report like this:
    #   noise_stationary_extended.
    #       ('0Hz', 'max_amplitude', 'square_wave', 'center')
    def _extract_frequency(self, data):
        return int(re.findall(r'\d+', data)[0])

    # extracts the tids from a line in the html report like this:
    #   count of trackid IDs: 1
    #   criteria: == 1
    def _extract_num_ids(self, data):
        return float(re.findall(r'\d+', data)[0])

    # extracts the distance from a line in the html report like this:
    #   Max distance slot0: 0.00 mm
    #   criteria: <= 1.0
    def _extract_distance(self, data):
        return float(re.findall(r'[-+]?\d*\.\d+|\d+', data)[0])

    # Add the value read to the dictionary.
    def _update_data_dict(self, value, val_index):
        curr_freq = self.curr_freq
        if curr_freq not in self.data_dict_list[self.curr_dict_index]:
            self.data_dict_list[self.curr_dict_index][curr_freq] = [None, None]

        self.data_dict_list[self.curr_dict_index][curr_freq][val_index] = value

    # Handler for HTMLParser for whenever it encounters text between tags
    def handle_data(self, data):
        # Get the current frequency
        if 'noise_stationary_extended' in data:
            self.curr_freq = self._extract_frequency(data)

            # Update the current iteration we're on.
            if self.curr_freq == self.last_freq:
                self.curr_dict_index = self.curr_dict_index + 1
            else:
                self.last_freq = self.curr_freq
                self.curr_dict_index = 0

        # Update number of fingers data
        if 'count of trackid IDs:' in data:
            num_ids = self._extract_num_ids(data)

            if num_ids != CORRECT_NUM_FINGERS:
                self.miscounted_fingers = self.miscounted_fingers + 1
                self._update_data_dict(num_ids, FINGERS_INDEX)
            else:
                self._update_data_dict(None, FINGERS_INDEX)

        # Update maximum distance data
        if 'Max distance' in data:
            distance = self._extract_distance(data)

            if distance > CORRECT_MAX_DISTANCE:
                self.over_distance = self.over_distance + 1
                self._update_data_dict(distance, DISTANCE_INDEX)
            else:
                self._update_data_dict(None, DISTANCE_INDEX)


# A parser to count the number of iterations
class CountIterations(ParseReport):
    def __init__(self):
        ParseReport.__init__(self, num_iterations=0)
        self.counting_iterations = True

    # Handler for HTMLParser for whenever it encounters text between tags
    def handle_data(self, data):
        # Get the current frequency
        if 'noise_stationary_extended' in data:
            self.curr_freq = self._extract_frequency(data)

            if self.counting_iterations:
                if self.curr_freq == self.last_freq:
                    self.num_iterations = self.num_iterations + 1
                else:
                    self.counting_iterations = False


# A weighting function to determine how badly
# a frequency failed. It outputs the total number
# of errors, where each misread or additionally read
# finger counts as one error, and each 0.2mm over the
# maximum distance counts as one error.
def weighting_function(data):
    num_fingers = data[FINGERS_INDEX]
    max_dist = data[DISTANCE_INDEX]

    if num_fingers is None:
        num_fingers = CORRECT_NUM_FINGERS
    if max_dist is None:
        max_dist = 0

    finger_val = abs(num_fingers - CORRECT_NUM_FINGERS)
    dist_val = 5 * (max_dist - CORRECT_MAX_DISTANCE)
    dist_val = 0 if dist_val < 0 else dist_val

    return finger_val + dist_val


# Returns a list of frequencies in order of how
# 'badly' they failed
def value_sorted_freq(data_dict):
    list_of_tuples = sorted(data_dict.iteritems(), reverse=True,
                            key=lambda (k, v): weighting_function(v))
    return [i[0] for i in list_of_tuples]


# Print out the summary of results for a single iteration,
# ordered by how badly each frequency failed.
def print_iteration_summary(data_dict, iteration, outfile):
    outfile.write('\n')
    outfile.write("Iteration %d\n" % iteration)
    outfile.write('-------------\n')

    for freq in value_sorted_freq(data_dict):
        num_fingers = data_dict[freq][FINGERS_INDEX]
        max_dist = data_dict[freq][DISTANCE_INDEX]

        # Don't output anything if there was no error
        if num_fingers is None and max_dist is None:
            continue
        else:
            num_fingers = '' if num_fingers is None else '%s tids' % num_fingers
            max_dist = '' if max_dist is None else '%s mm' % max_dist

        outfile.write('{:,}Hz \t %s \t %s \n'.format(freq) %
                     (num_fingers, max_dist))


# Print out a summary of errors for each iteration
def print_summary(parse_report, output_file):
    outfile = open(output_file, 'w')
    outfile.write('Summary: \n')
    outfile.write('    %d issues with finger tracking over all iterations. \n' %
                  parse_report.miscounted_fingers)
    outfile.write('    %d issues with distance over all iterations. \n' %
                  parse_report.over_distance)
    outfile.write('\n\n')

    outfile.write('Worst frequencies:\n')

    for iteration, data_dict in enumerate(parse_report.data_dict_list):
        print_iteration_summary(data_dict, iteration, outfile)

    outfile.close()


# For each iteration, generate a subplot
def show_graph(parse_report):
    for iteration, data_dict in enumerate(parse_report.data_dict_list):
        sorted_by_freq = sorted(parse_report.data_dict_list[iteration].items())
        frequencies = [i[0] for i in sorted_by_freq]
        values = [weighting_function(i[1]) for i in sorted_by_freq]

        plt.subplot(parse_report.num_iterations, 1, iteration)
        plt.plot(frequencies, values)

        plt.xlabel('Frequency (Hz)')
        plt.ylabel('Number of problems')
        plt.legend(("Iteration %d" % iteration,))

    plt.title('Graphic Summary of Extended Noise Test')
    plt.show()


def main():
    # Error checking
    if len(sys.argv) != 2:
        print 'Usage: python noise_summary.py report.html'
        return

    input_file = sys.argv[1]
    if '.html' not in input_file:
        print 'File must be an html firmware report.'
        print 'An example report name is:'
        print 'touch_firmware_report-swanky-fw_2.0-noise-20140826_173022.html'
        return

    # Create filepaths
    directory = os.path.dirname(input_file)
    output_file = '%s_summary.txt' % \
                  os.path.splitext(os.path.basename(input_file))[0]
    output_path = os.path.join(directory, output_file)

    try:
        html_file = open(input_file)
    except:
        print '%s could not be found.' % input_file
        return

    # Parse the report
    html = html_file.read()
    c = CountIterations()
    c.feed(html)
    p = ParseReport(c.num_iterations)
    p.feed(html)
    html_file.close()
    p.close()

    # Display the result
    print_summary(p, output_path)
    print 'The summary has been saved to %s' % output_path
    show_graph(p)


if __name__ == '__main__':
    main()
