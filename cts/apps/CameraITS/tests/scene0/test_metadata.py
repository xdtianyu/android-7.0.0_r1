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

import its.image
import its.device
import its.objects
import its.target
import its.caps

def main():
    """Test the validity of some metadata entries.

    Looks at capture results and at the camera characteristics objects.
    """
    global md, props, failed

    with its.device.ItsSession() as cam:
        # Arbitrary capture request exposure values; image content is not
        # important for this test, only the metadata.
        props = cam.get_camera_properties()
        auto_req = its.objects.auto_capture_request()
        cap = cam.do_capture(auto_req)
        md = cap["metadata"]

    print "Hardware level"
    print "  Legacy:", its.caps.legacy(props)
    print "  Limited:", its.caps.limited(props)
    print "  Full or better:", its.caps.full_or_better(props)
    print "Capabilities"
    print "  Manual sensor:", its.caps.manual_sensor(props)
    print "  Manual post-proc:", its.caps.manual_post_proc(props)
    print "  Raw:", its.caps.raw(props)
    print "  Sensor fusion:", its.caps.sensor_fusion(props)

    # Test: hardware level should be a valid value.
    check('props.has_key("android.info.supportedHardwareLevel")')
    check('props["android.info.supportedHardwareLevel"] is not None')
    check('props["android.info.supportedHardwareLevel"] in [0,1,2,3]')
    full = getval('props["android.info.supportedHardwareLevel"]') == 1
    manual_sensor = its.caps.manual_sensor(props)

    # Test: rollingShutterSkew, and frameDuration tags must all be present,
    # and rollingShutterSkew must be greater than zero and smaller than all
    # of the possible frame durations.
    if manual_sensor:
        check('md.has_key("android.sensor.frameDuration")')
        check('md["android.sensor.frameDuration"] is not None')
    check('md.has_key("android.sensor.rollingShutterSkew")')
    check('md["android.sensor.rollingShutterSkew"] is not None')
    if manual_sensor:
        check('md["android.sensor.frameDuration"] > '
              'md["android.sensor.rollingShutterSkew"] > 0')

    # Test: timestampSource must be a valid value.
    check('props.has_key("android.sensor.info.timestampSource")')
    check('props["android.sensor.info.timestampSource"] is not None')
    check('props["android.sensor.info.timestampSource"] in [0,1]')

    # Test: croppingType must be a valid value, and for full devices, it
    # must be FREEFORM=1.
    check('props.has_key("android.scaler.croppingType")')
    check('props["android.scaler.croppingType"] is not None')
    check('props["android.scaler.croppingType"] in [0,1]')

    assert(not failed)

def getval(expr, default=None):
    try:
        return eval(expr)
    except:
        return default

failed = False
def check(expr):
    global md, props, failed
    try:
        if eval(expr):
            print "Passed>", expr
        else:
            print "Failed>>", expr
            failed = True
    except:
        print "Failed>>", expr
        failed = True

if __name__ == '__main__':
    main()

