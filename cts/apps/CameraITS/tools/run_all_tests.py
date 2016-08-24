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

import os
import os.path
import tempfile
import subprocess
import time
import sys
import textwrap
import its.device

def main():
    """Run all the automated tests, saving intermediate files, and producing
    a summary/report of the results.

    Script should be run from the top-level CameraITS directory.
    """

    SKIP_RET_CODE = 101

    # Not yet mandated tests
    NOT_YET_MANDATED = {
        "scene0":[
            "test_jitter"
        ],
        "scene1":[
            "test_ae_precapture_trigger",
            "test_crop_region_raw",
            "test_ev_compensation_advanced",
            "test_ev_compensation_basic",
            "test_yuv_plus_jpeg"
        ],
        "scene2":[],
        "scene3":[],
        "scene4":[],
        "scene5":[],
        "sensor_fusion":[]
    }

    # Get all the scene0 and scene1 tests, which can be run using the same
    # physical setup.
    scenes = ["scene0", "scene1", "scene2", "scene3", "scene4", "scene5"]

    scene_req = {
        "scene0" : None,
        "scene1" : "A grey card covering at least the middle 30% of the scene",
        "scene2" : "A picture containing human faces",
        "scene3" : "A chart containing sharp edges like ISO 12233",
        "scene4" : "A specific test page of a circle covering at least the "
                   "middle 50% of the scene. See CameraITS.pdf section 2.3.4 "
                   "for more details",
        "scene5" : "Capture images with a diffuser attached to the camera. See "
                   "CameraITS.pdf section 2.3.4 for more details",
        "sensor_fusion" : "Rotating checkboard pattern. See "
                          "sensor_fusion/SensorFusion.pdf for detailed "
                          "instructions. Note that this test will be skipped "
                          "on devices not supporting REALTIME camera timestamp."
                          "If that is the case, no scene setup is required and "
                          "you can just answer Y when being asked if the scene "
                          "is okay"
    }
    scene_extra_args = {
        "scene5" : ["doAF=False"]
    }
    tests = []
    for d in scenes:
        tests += [(d,s[:-3],os.path.join("tests", d, s))
                  for s in os.listdir(os.path.join("tests",d))
                  if s[-3:] == ".py"]
    tests.sort()

    # Make output directories to hold the generated files.
    topdir = tempfile.mkdtemp()
    print "Saving output files to:", topdir, "\n"

    device_id = its.device.get_device_id()
    device_id_arg = "device=" + device_id
    print "Testing device " + device_id

    camera_ids = []
    for s in sys.argv[1:]:
        if s[:7] == "camera=" and len(s) > 7:
            camera_ids.append(s[7:])

    # user doesn't specify camera id, run through all cameras
    if not camera_ids:
        camera_ids_path = os.path.join(topdir, "camera_ids.txt")
        out_arg = "out=" + camera_ids_path
        cmd = ['python',
               os.path.join(os.getcwd(),"tools/get_camera_ids.py"), out_arg,
               device_id_arg]
        retcode = subprocess.call(cmd,cwd=topdir)
        assert(retcode == 0)
        with open(camera_ids_path, "r") as f:
            for line in f:
                camera_ids.append(line.replace('\n', ''))

    print "Running ITS on the following cameras:", camera_ids

    for camera_id in camera_ids:
        # Loop capturing images until user confirm test scene is correct
        camera_id_arg = "camera=" + camera_id
        print "Preparing to run ITS on camera", camera_id

        os.mkdir(os.path.join(topdir, camera_id))
        for d in scenes:
            os.mkdir(os.path.join(topdir, camera_id, d))

        print "Start running ITS on camera: ", camera_id
        # Run each test, capturing stdout and stderr.
        summary = "ITS test result summary for camera " + camera_id + "\n"
        numpass = 0
        numskip = 0
        numnotmandatedfail = 0
        numfail = 0

        prev_scene = ""
        for (scene,testname,testpath) in tests:
            if scene != prev_scene and scene_req[scene] != None:
                out_path = os.path.join(topdir, camera_id, scene+".jpg")
                out_arg = "out=" + out_path
                scene_arg = "scene=" + scene_req[scene]
                extra_args = scene_extra_args.get(scene, [])
                cmd = ['python',
                        os.path.join(os.getcwd(),"tools/validate_scene.py"),
                        camera_id_arg, out_arg, scene_arg, device_id_arg] + \
                        extra_args
                retcode = subprocess.call(cmd,cwd=topdir)
                assert(retcode == 0)
                print "Start running tests for", scene
            prev_scene = scene
            cmd = ['python', os.path.join(os.getcwd(),testpath)] + \
                  sys.argv[1:] + [camera_id_arg]
            outdir = os.path.join(topdir,camera_id,scene)
            outpath = os.path.join(outdir,testname+"_stdout.txt")
            errpath = os.path.join(outdir,testname+"_stderr.txt")
            t0 = time.time()
            with open(outpath,"w") as fout, open(errpath,"w") as ferr:
                retcode = subprocess.call(cmd,stderr=ferr,stdout=fout,cwd=outdir)
            t1 = time.time()

            if retcode == 0:
                retstr = "PASS "
                numpass += 1
            elif retcode == SKIP_RET_CODE:
                retstr = "SKIP "
                numskip += 1
            elif retcode != 0 and testname in NOT_YET_MANDATED[scene]:
                retstr = "FAIL*"
                numnotmandatedfail += 1
            else:
                retstr = "FAIL "
                numfail += 1

            msg = "%s %s/%s [%.1fs]" % (retstr, scene, testname, t1-t0)
            print msg
            summary += msg + "\n"
            if retcode != 0 and retcode != SKIP_RET_CODE:
                # Dump the stderr if the test fails
                with open (errpath, "r") as error_file:
                    errors = error_file.read()
                    summary += errors + "\n"

        if numskip > 0:
            skipstr = ", %d test%s skipped" % (numskip, "s" if numskip > 1 else "")
        else:
            skipstr = ""

        test_result = "\n%d / %d tests passed (%.1f%%)%s" % (
                numpass + numnotmandatedfail, len(tests) - numskip,
                100.0 * float(numpass + numnotmandatedfail) / (len(tests) - numskip)
                    if len(tests) != numskip else 100.0,
                skipstr)
        print test_result
        summary += test_result + "\n"

        if numnotmandatedfail > 0:
            msg = "(*) tests are not yet mandated"
            print msg
            summary += msg + "\n"

        result = numfail == 0
        print "Reporting ITS result to CtsVerifier"
        summary_path = os.path.join(topdir, camera_id, "summary.txt")
        with open(summary_path, "w") as f:
            f.write(summary)
        its.device.report_result(device_id, camera_id, result, summary_path)

    print "ITS tests finished. Please go back to CtsVerifier and proceed"

if __name__ == '__main__':
    main()
