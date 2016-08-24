#!/bin/bash

set -e

usage() {

cat <<EOF
    $0
    --sdk <SDK file path>
    --system <system sdk file path>
    --support <support library file path>
EOF
  exit 2

}

banner() {
  echo "**************************************************"
  echo "Updating $1                                     "
  echo "**************************************************"
}

update_sdk() {
  if [ -f "$SDK" ]
  then
    banner "SDK"
    cd $ROOT_DIR/current
    rm -f android.jar uiautomator.jar framework.aidl
    unzip -j $SDK */android.jar */uiautomator.jar */framework.aidl
  fi
}

update_system_sdk() {
  if [ -f "$SYSTEM_SDK" ]
  then
    banner "system SDK"
    cp -f $SYSTEM_SDK $ROOT_DIR/system_current/android.jar
  fi
}

update_support_lib() {
  if [ -f "$SUPPORT" ]
  then
    banner "support library"
    rm -rf $ROOT_DIR/current/support/
    cd $ROOT_DIR/current
    unzip $SUPPORT >/dev/null

    # Remove duplicates
    rm -f support/v7/appcompat/libs/android-support-v4.jar
    rm -f support/multidex/instrumentation/libs/android-support-multidex.jar

    # Remove samples
    rm -rf support/samples

    # Remove source files
    find support -name "*.java" \
      -o -name "*.aidl" \
      -o -name AndroidManifest.xml \
    | xargs rm

    # Other misc files we don't need
    find support -name "*.gradle" \
      -o -name ".classpath" \
      -o -name ".project" \
      -o -name "project.properties" \
      -o -name "source.properties" \
      -o -name ".readme" \
      -o -name "README.txt" \
      -o -name "package.html" \
      -o -name "NOTICE.txt" \
    | xargs rm

    # Now we can remove empty dirs
    find . -type d -empty -delete
  fi
}

main() {
  while [ "$#" -gt 0 ]
  do
    case "$1" in
      --help|-h)
        usage
        ;;
      --sdk)
        export SDK="$2"
        shift; shift
        ;;
      --system)
        export SYSTEM_SDK="$2"
        shift; shift
        ;;
      --support)
        export SUPPORT="$2"
        shift; shift
        ;;
      -*)
        usage
        ;;
      *)
        break
        ;;
    esac
  done

  ROOT_DIR=$(realpath $(dirname $0))

  update_sdk
  update_system_sdk
  update_support_lib
}

main $*
