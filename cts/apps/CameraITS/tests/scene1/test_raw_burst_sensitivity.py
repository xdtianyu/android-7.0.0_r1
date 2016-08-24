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
    """Capture a set of raw images with increasing gains and measure the noise.

    Capture raw-only, in a burst.
    """
    NAME = os.path.basename(__file__).split(".")[0]

    # Each shot must be 1% noisier (by the variance metric) than the previous
    # one.
    VAR_THRESH = 1.01

    NUM_STEPS = 5

    with its.device.ItsSession() as cam:

        props = cam.get_camera_properties()
        its.caps.skip_unless(its.caps.raw16(props) and
                             its.caps.manual_sensor(props) and
                             its.caps.read_3a(props) and
                             its.caps.per_frame_control(props))

        # Expose for the scene with min sensitivity
        sens_min, sens_max = props['android.sensor.info.sensitivityRange']
        # Digital gains might not be visible on RAW data
        sens_max = props['android.sensor.maxAnalogSensitivity']
        sens_step = (sens_max - sens_min) / NUM_STEPS
        s_ae,e_ae,_,_,_  = cam.do_3a(get_results=True)
        s_e_prod = s_ae * e_ae

        reqs = []
        settings = []
        for s in range(sens_min, sens_max, sens_step):
            e = int(s_e_prod / float(s))
            req = its.objects.manual_capture_request(s, e)
            reqs.append(req)
            settings.append((s,e))

        caps = cam.do_capture(reqs, cam.CAP_RAW)

        variances = []
        for i,cap in enumerate(caps):
            (s,e) = settings[i]

            # Measure the variance. Each shot should be noisier than the
            # previous shot (as the gain is increasing).
            plane = its.image.convert_capture_to_planes(cap, props)[1]
            tile = its.image.get_image_patch(plane, 0.45,0.45,0.1,0.1)
            var = its.image.compute_image_variances(tile)[0]
            variances.append(var)

            img = its.image.convert_capture_to_rgb_image(cap, props=props)
            its.image.write_image(img, "%s_s=%05d_var=%f.jpg" % (NAME,s,var))
            print "s=%d, e=%d, var=%e"%(s,e,var)

        pylab.plot(range(len(variances)), variances)
        matplotlib.pyplot.savefig("%s_variances.png" % (NAME))

        # Test that each shot is noisier than the previous one.
        for i in range(len(variances) - 1):
            assert(variances[i] < variances[i+1] / VAR_THRESH)

if __name__ == '__main__':
    main()

