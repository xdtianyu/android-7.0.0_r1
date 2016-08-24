#!/usr/bin/python
import logging
import numpy
import unittest

import common
from autotest_lib.client.cros.audio import audio_analysis
from autotest_lib.client.cros.audio import audio_data

class SpectralAnalysisTest(unittest.TestCase):
    def setUp(self):
        """Uses the same seed to generate noise for each test."""
        numpy.random.seed(0)


    def testSpectralAnalysis(self):
        rate = 48000
        length_in_secs = 0.5
        freq_1 = 490.0
        freq_2 = 60.0
        coeff_1 = 1
        coeff_2 = 0.3
        samples = length_in_secs * rate
        noise = numpy.random.standard_normal(samples) * 0.005
        x = numpy.linspace(0.0, (samples - 1) * 1.0 / rate, samples)
        y = (coeff_1 * numpy.sin(freq_1 * 2.0 * numpy.pi * x) +
             coeff_2 * numpy.sin(freq_2 * 2.0 * numpy.pi * x)) + noise
        results = audio_analysis.spectral_analysis(y, rate)
        # Results should contains
        # [(490, 1*k), (60, 0.3*k), (0, 0.1*k)] where 490Hz is the dominant
        # frequency with coefficient 1, 60Hz is the second dominant frequency
        # with coefficient 0.3, 0Hz is from Gaussian noise with coefficient
        # around 0.1. The k constant is resulted from window function.
        logging.debug('Results: %s', results)
        self.assertTrue(abs(results[0][0]-freq_1) < 1)
        self.assertTrue(abs(results[1][0]-freq_2) < 1)
        self.assertTrue(
                abs(results[0][1] / results[1][1] - coeff_1 / coeff_2) < 0.01)


    def testSpectralAnalysisRealData(self):
        """This unittest checks the spectral analysis works on real data."""
        binary = open('client/cros/audio/test_data/1k_2k.raw', 'r').read()
        data = audio_data.AudioRawData(binary, 2, 'S32_LE')
        saturate_value = audio_data.get_maximum_value_from_sample_format(
                'S32_LE')
        golden_frequency = [1000, 2000]
        for channel in [0, 1]:
            normalized_signal = audio_analysis.normalize_signal(
                    data.channel_data[channel],saturate_value)
            spectral = audio_analysis.spectral_analysis(
                    normalized_signal, 48000, 0.02)
            logging.debug('channel %s: %s', channel, spectral)
            self.assertTrue(abs(spectral[0][0] - golden_frequency[channel]) < 5,
                            'Dominant frequency is not correct')


    def testNotMeaningfulData(self):
        """Checks that sepectral analysis rejects not meaningful data."""
        rate = 48000
        length_in_secs = 0.5
        samples = length_in_secs * rate
        noise_amplitude = audio_analysis.MEANINGFUL_RMS_THRESHOLD * 0.5
        noise = numpy.random.standard_normal(samples) * noise_amplitude
        with self.assertRaises(audio_analysis.RMSTooSmallError):
            results = audio_analysis.spectral_analysis(noise, rate)


    def testEmptyData(self):
        """Checks that sepectral analysis rejects empty data."""
        with self.assertRaises(audio_analysis.EmptyDataError):
            results = audio_analysis.spectral_analysis([], 100)


class NormalizeTest(unittest.TestCase):
    def testNormalize(self):
        y = [1, 2, 3, 4, 5]
        normalized_y = audio_analysis.normalize_signal(y, 10)
        expected = numpy.array([0.1, 0.2, 0.3, 0.4, 0.5])
        for i in xrange(len(y)):
            self.assertEqual(expected[i], normalized_y[i])


class AnomalyTest(unittest.TestCase):
    def setUp(self):
        """Creates a test signal of sine wave."""
        # Use the same seed for each test case.
        numpy.random.seed(0)

        self.block_size = 120
        self.rate = 48000
        self.freq = 440
        length_in_secs = 0.25
        self.samples = length_in_secs * self.rate
        x = numpy.linspace(
                0.0, (self.samples - 1) * 1.0 / self.rate, self.samples)
        self.y = numpy.sin(self.freq * 2.0 * numpy.pi * x)


    def add_noise(self):
        """Add noise to the test signal."""
        noise_amplitude = 0.3
        noise = numpy.random.standard_normal(len(self.y)) * noise_amplitude
        self.y = self.y + noise


    def insert_anomaly(self):
        """Inserts an anomaly to the test signal.

        The anomaly self.anomaly_samples should be created before calling this
        method.

        """
        self.anomaly_start_secs = 0.1
        self.y = numpy.insert(self.y, int(self.anomaly_start_secs * self.rate),
                              self.anomaly_samples)


    def generate_skip_anomaly(self):
        """Skips a section of test signal."""
        self.anomaly_start_secs = 0.1
        self.anomaly_duration_secs = 0.005
        anomaly_append_secs = self.anomaly_start_secs + self.anomaly_duration_secs
        anomaly_start_index = self.anomaly_start_secs * self.rate
        anomaly_append_index = anomaly_append_secs * self.rate
        self.y = numpy.append(self.y[:anomaly_start_index], self.y[anomaly_append_index:])


    def create_constant_anomaly(self, amplitude):
        """Creates an anomaly of constant samples.

        @param amplitude: The amplitude of the constant samples.

        """
        self.anomaly_duration_secs = 0.005
        self.anomaly_samples = (
                [amplitude] * int(self.anomaly_duration_secs * self.rate))


    def run_analysis(self):
        """Runs the anomaly detection."""
        self.results = audio_analysis.anomaly_detection(
                self.y, self.rate, self.freq, self.block_size)
        logging.debug('Results: %s', self.results)


    def check_no_anomaly(self):
        """Verifies that there is no anomaly in detection result."""
        self.run_analysis()
        self.assertFalse(self.results)


    def check_anomaly(self):
        """Verifies that there is anomaly in detection result.

        The detection result should contain anomaly time stamps that are
        close to where anomaly was inserted. There can be multiple anomalies
        since the detection depends on the block size.

        """
        self.run_analysis()
        self.assertTrue(self.results)
        # Anomaly can be detected as long as the detection window of block size
        # overlaps with anomaly.
        expected_detected_range_secs = (
                self.anomaly_start_secs - float(self.block_size) / self.rate,
                self.anomaly_start_secs + self.anomaly_duration_secs)
        for detected_secs in self.results:
            self.assertTrue(detected_secs <= expected_detected_range_secs[1])
            self.assertTrue(detected_secs >= expected_detected_range_secs[0] )


    def testGoodSignal(self):
        """Sine wave signal with no noise or anomaly."""
        self.check_no_anomaly()


    def testGoodSignalNoise(self):
        """Sine wave signal with noise."""
        self.add_noise()
        self.check_no_anomaly()


    def testZeroAnomaly(self):
        """Sine wave signal with no noise but with anomaly.

        This test case simulates underrun in digital data where there will be
        one block of samples with 0 amplitude.

        """
        self.create_constant_anomaly(0)
        self.insert_anomaly()
        self.check_anomaly()


    def testZeroAnomalyNoise(self):
        """Sine wave signal with noise and anomaly.

        This test case simulates underrun in analog data where there will be
        one block of samples with amplitudes close to 0.

        """
        self.create_constant_anomaly(0)
        self.insert_anomaly()
        self.add_noise()
        self.check_anomaly()


    def testLowConstantAnomaly(self):
        """Sine wave signal with low constant anomaly.

        The anomaly is one block of constant values.

        """
        self.create_constant_anomaly(0.05)
        self.insert_anomaly()
        self.check_anomaly()


    def testLowConstantAnomalyNoise(self):
        """Sine wave signal with low constant anomaly and noise.

        The anomaly is one block of constant values.

        """
        self.create_constant_anomaly(0.05)
        self.insert_anomaly()
        self.add_noise()
        self.check_anomaly()


    def testHighConstantAnomaly(self):
        """Sine wave signal with high constant anomaly.

        The anomaly is one block of constant values.

        """
        self.create_constant_anomaly(2)
        self.insert_anomaly()
        self.check_anomaly()


    def testHighConstantAnomalyNoise(self):
        """Sine wave signal with high constant anomaly and noise.

        The anomaly is one block of constant values.

        """
        self.create_constant_anomaly(2)
        self.insert_anomaly()
        self.add_noise()
        self.check_anomaly()


    def testSkippedAnomaly(self):
        """Sine wave signal with skipped anomaly.

        The anomaly simulates the symptom where a block is skipped.

        """
        self.generate_skip_anomaly()
        self.check_anomaly()


    def testSkippedAnomalyNoise(self):
        """Sine wave signal with skipped anomaly with noise.

        The anomaly simulates the symptom where a block is skipped.

        """
        self.generate_skip_anomaly()
        self.add_noise()
        self.check_anomaly()


    def testEmptyData(self):
        """Checks that anomaly detection rejects empty data."""
        self.y = []
        with self.assertRaises(audio_analysis.EmptyDataError):
            self.check_anomaly()


if __name__ == '__main__':
    logging.basicConfig(level=logging.DEBUG)
    unittest.main()
