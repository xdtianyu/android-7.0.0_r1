# Copyright 2014 The Android Open Source Project
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

import its.device
import its.caps
import its.objects
import its.image
import os.path
import pylab
import matplotlib
import matplotlib.pyplot

def main():
    """Verify that the DNG raw model parameters are correct.
    """
    NAME = os.path.basename(__file__).split(".")[0]

    NUM_STEPS = 4

    # Pass if the difference between expected and computed variances is small,
    # defined as being within an absolute variance delta of 0.0005, or within
    # 20% of the expected variance, whichever is larger; this is to allow the
    # test to pass in the presence of some randomness (since this test is
    # measuring noise of a small patch) and some imperfect scene conditions
    # (since ITS doesn't require a perfectly uniformly lit scene).
    DIFF_THRESH = 0.0005
    FRAC_THRESH = 0.2

    with its.device.ItsSession() as cam:

        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.raw(props) and
                             its.caps.raw16(props) and
                             its.caps.manual_sensor(props) and
                             its.caps.read_3a(props) and
                             its.caps.per_frame_control(props))

        white_level = float(props['android.sensor.info.whiteLevel'])
        black_levels = props['android.sensor.blackLevelPattern']
        cfa_idxs = its.image.get_canonical_cfa_order(props)
        black_levels = [black_levels[i] for i in cfa_idxs]

        # Expose for the scene with min sensitivity
        sens_min, sens_max = props['android.sensor.info.sensitivityRange']
        sens_step = (sens_max - sens_min) / NUM_STEPS
        s_ae,e_ae,_,_,_  = cam.do_3a(get_results=True)
        s_e_prod = s_ae * e_ae
        sensitivities = range(sens_min, sens_max, sens_step)

        var_expected = [[],[],[],[]]
        var_measured = [[],[],[],[]]
        for sens in sensitivities:

            # Capture a raw frame with the desired sensitivity.
            exp = int(s_e_prod / float(sens))
            req = its.objects.manual_capture_request(sens, exp)
            cap = cam.do_capture(req, cam.CAP_RAW)

            # Test each raw color channel (R, GR, GB, B):
            noise_profile = cap["metadata"]["android.sensor.noiseProfile"]
            assert((len(noise_profile)) == 4)
            for ch in range(4):
                # Get the noise model parameters for this channel of this shot.
                s,o = noise_profile[cfa_idxs[ch]]

                # Get a center tile of the raw channel, and compute the mean.
                # Use a very small patch to ensure gross uniformity (i.e. so
                # non-uniform lighting or vignetting doesn't affect the variance
                # calculation).
                plane = its.image.convert_capture_to_planes(cap, props)[ch]
                plane = (plane * white_level - black_levels[ch]) / (
                        white_level - black_levels[ch])
                tile = its.image.get_image_patch(plane, 0.49,0.49,0.02,0.02)
                mean = tile.mean()

                # Calculate the expected variance based on the model, and the
                # measured variance from the tile.
                var_measured[ch].append(
                        its.image.compute_image_variances(tile)[0])
                var_expected[ch].append(s * mean + o)

    for ch in range(4):
        pylab.plot(sensitivities, var_expected[ch], "rgkb"[ch],
                label=["R","GR","GB","B"][ch]+" expected")
        pylab.plot(sensitivities, var_measured[ch], "rgkb"[ch]+"--",
                label=["R", "GR", "GB", "B"][ch]+" measured")
    pylab.xlabel("Sensitivity")
    pylab.ylabel("Center patch variance")
    pylab.legend(loc=2)
    matplotlib.pyplot.savefig("%s_plot.png" % (NAME))

    # Pass/fail check.
    for ch in range(4):
        diffs = [var_measured[ch][i] - var_expected[ch][i]
                 for i in range(NUM_STEPS)]
        print "Diffs (%s):"%(["R","GR","GB","B"][ch]), diffs
        for i,diff in enumerate(diffs):
            thresh = max(DIFF_THRESH, FRAC_THRESH * var_expected[ch][i])
            assert(diff <= thresh)

if __name__ == '__main__':
    main()

