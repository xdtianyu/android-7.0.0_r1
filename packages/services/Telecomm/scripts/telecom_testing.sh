lite_test_telecom() {
  usage="
  Usage: lite_test_telecom [-c CLASSNAME] [-d] [-a | -i] [-e], where

  -c CLASSNAME          Run tests only for the specified class/method. CLASSNAME
                          should be of the form SomeClassTest or SomeClassTest#testMethod.
  -d                    Waits for a debugger to attach before starting to run tests.
  -i                    Rebuild and reinstall the test apk before running tests (mmm).
  -a                    Rebuild all dependencies and reinstall the test apk before/
                          running tests (mmma).
  -e                    Run code coverage. Coverage will be output into the coverage/
                          directory in the repo root.
  -h                    This help message.
  "

  OPTIND=1
  class=
  install=false
  installwdep=false
  debug=false
  coverage=false

  while getopts "c:hadie" opt; do
    case "$opt" in
      h)
        echo "$usage"
        return 0;;
      \?)
        echo "$usage"
        return 0;;
      c)
        class=$OPTARG;;
      d)
        debug=true;;
      i)
        install=true;;
      a)
        install=true
        installwdep=true;;
      e)
        coverage=true;;
    esac
  done

  T=$(gettop)

  if [ $install = true ] ; then
    olddir=$(pwd)
    cd $T
    if [ $coverage = true ] ; then
      emma_opt="EMMA_INSTRUMENT_STATIC=true"
    else
      emma_opt="EMMA_INSTRUMENT_STATIC=false"
    fi
    # Build and exit script early if build fails
    if [ $installwdep = true ] ; then
      mmma -j40 "packages/services/Telecomm/tests" ${emma_opt}
    else
      mmm "packages/services/Telecomm/tests" ${emma_opt}
    fi
    if [ $? -ne 0 ] ; then
      echo "Make failed! try using -a instead of -i if building with coverage"
      return
    fi

    adb install -r -t "out/target/product/$TARGET_PRODUCT/data/app/TelecomUnitTests/TelecomUnitTests.apk"
    if [ $? -ne 0 ] ; then
      cd "$olddir"
      return $?
    fi
    cd "$olddir"
  fi

  e_options=""
  if [ -n "$class" ] ; then
    e_options="${e_options} -e class com.android.server.telecom.tests.${class}"
  fi
  if [ $debug = true ] ; then
    e_options="${e_options} -e debug 'true'"
  fi
  if [ $coverage = true ] ; then
    e_options="${e_options} -e coverage 'true'"
  fi
  adb shell am instrument ${e_options} -w com.android.server.telecom.tests/android.test.InstrumentationTestRunner

  if [ $coverage = true ] ; then
    adb root
    adb wait-for-device
    adb pull /data/user/0/com.android.server.telecom.tests/files/coverage.ec /tmp/
    if [ ! -d "$T/coverage" ] ; then
      mkdir -p "$T/coverage"
    fi
    java -jar "$T/prebuilts/sdk/tools/jack-jacoco-reporter.jar" \
      --report-dir "$T/coverage/" \
      --metadata-file "$T/out/target/common/obj/APPS/TelecomUnitTests_intermediates/coverage.em" \
      --coverage-file "/tmp/coverage.ec" \
      --source-dir "$T/packages/services/Telecomm/src/"
  fi
}
