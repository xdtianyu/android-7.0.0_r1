# Copyright 2013 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import its.image
import its.caps
import its.device
import its.objects
import its.target
import pylab
import numpy
import os.path
import matplotlib
import matplotlib.pyplot

def main():
    """Test that a constant exposure is seen as ISO and exposure time vary.

    Take a series of shots that have ISO and exposure time chosen to balance
    each other; result should be the same brightness, but over the sequence
    the images should get noisier.
    """
    NAME = os.path.basename(__file__).split(".")[0]

    THRESHOLD_MAX_OUTLIER_DIFF = 0.1
    THRESHOLD_MIN_LEVEL = 0.1
    THRESHOLD_MAX_LEVEL = 0.9
    THRESHOLD_MAX_LEVEL_DIFF = 0.03
    THRESHOLD_MAX_LEVEL_DIFF_WIDE_RANGE = 0.05
    THRESHOLD_ROUND_DOWN_GAIN = 0.1
    THRESHOLD_ROUND_DOWN_EXP = 0.05

    mults = []
    r_means = []
    g_means = []
    b_means = []
    threshold_max_level_diff = THRESHOLD_MAX_LEVEL_DIFF

    with its.device.ItsSession() as cam:
        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.compute_target_exposure(props) and
                             its.caps.per_frame_control(props))

        e,s = its.target.get_target_exposure_combos(cam)["minSensitivity"]
        s_e_product = s*e
        expt_range = props['android.sensor.info.exposureTimeRange']
        sens_range = props['android.sensor.info.sensitivityRange']

        m = 1.0
        while s*m < sens_range[1] and e/m > expt_range[0]:
            mults.append(m)
            s_test = round(s*m)
            e_test = s_e_product / s_test
            print "Testing s:", s_test, "e:", e_test
            req = its.objects.manual_capture_request(s_test, e_test, True, props)
            cap = cam.do_capture(req)
            s_res = cap["metadata"]["android.sensor.sensitivity"]
            e_res = cap["metadata"]["android.sensor.exposureTime"]
            assert(0 <= s_test - s_res < s_test * THRESHOLD_ROUND_DOWN_GAIN)
            assert(0 <= e_test - e_res < e_test * THRESHOLD_ROUND_DOWN_EXP)
            s_e_product_res = s_res * e_res
            request_result_ratio = s_e_product / s_e_product_res
            print "Capture result s:", s_test, "e:", e_test
            img = its.image.convert_capture_to_rgb_image(cap)
            its.image.write_image(img, "%s_mult=%3.2f.jpg" % (NAME, m))
            tile = its.image.get_image_patch(img, 0.45, 0.45, 0.1, 0.1)
            rgb_means = its.image.compute_image_means(tile)
            # Adjust for the difference between request and result
            r_means.append(rgb_means[0] * request_result_ratio)
            g_means.append(rgb_means[1] * request_result_ratio)
            b_means.append(rgb_means[2] * request_result_ratio)
            # Test 3 steps per 2x gain
            m = m * pow(2, 1.0 / 3)

        # Allow more threshold for devices with wider exposure range
        if m >= 64.0:
            threshold_max_level_diff = THRESHOLD_MAX_LEVEL_DIFF_WIDE_RANGE

    # Draw a plot.
    pylab.plot(mults, r_means, 'r.-')
    pylab.plot(mults, g_means, 'g.-')
    pylab.plot(mults, b_means, 'b.-')
    pylab.ylim([0,1])
    matplotlib.pyplot.savefig("%s_plot_means.png" % (NAME))

    # Check for linearity. Verify sample pixel mean values are close to each
    # other. Also ensure that the images aren't clamped to 0 or 1
    # (which would make them look like flat lines).
    for chan in xrange(3):
        values = [r_means, g_means, b_means][chan]
        m, b = numpy.polyfit(mults, values, 1).tolist()
        max_val = max(values)
        min_val = min(values)
        max_diff = max_val - min_val
        print "Channel %d line fit (y = mx+b): m = %f, b = %f" % (chan, m, b)
        print "Channel max %f min %f diff %f" % (max_val, min_val, max_diff)
        assert(max_diff < threshold_max_level_diff)
        assert(b > THRESHOLD_MIN_LEVEL and b < THRESHOLD_MAX_LEVEL)
        for v in values:
            assert(v > THRESHOLD_MIN_LEVEL and v < THRESHOLD_MAX_LEVEL)
            assert(abs(v - b) < THRESHOLD_MAX_OUTLIER_DIFF)

if __name__ == '__main__':
    main()
