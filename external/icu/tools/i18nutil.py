import os
import sys
import tempfile

"""Shared functions for use in i18n scripts."""

def CheckDirExists(dir, dirname):
  if not os.path.isdir(dir):
    print "Couldn't find %s (%s)!" % (dirname, dir)
    sys.exit(1)

def GetAndroidRootOrDie():
  value = os.environ.get('ANDROID_BUILD_TOP')
  if not value:
    print "ANDROID_BUILD_TOP not defined: run envsetup.sh / lunch"
    sys.exit(1);
  CheckDirExists(value, '$ANDROID_BUILD_TOP')
  return value

def SwitchToNewTemporaryDirectory():
  tmp_dir = tempfile.mkdtemp('-i18n')
  os.chdir(tmp_dir)
  print 'Created temporary directory "%s"...' % tmp_dir
