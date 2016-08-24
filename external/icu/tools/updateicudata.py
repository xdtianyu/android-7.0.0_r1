#!/usr/bin/python

"""Regenerates ICU data files."""

import glob
import os
import shutil
import subprocess
import sys

import i18nutil

# Find the icu directory.
android_build_top = i18nutil.GetAndroidRootOrDie()
icu_dir = os.path.realpath('%s/external/icu' % android_build_top)
icu4c_dir = os.path.realpath('%s/icu4c/source' % icu_dir)
icu4j_dir = os.path.realpath('%s/icu4j' % icu_dir)
i18nutil.CheckDirExists(icu4c_dir, 'external/icu/icu4c/source')
i18nutil.CheckDirExists(icu4j_dir, 'external/icu/icu4j')

def PrepareIcuBuild(icu_build_dir):
  # Keep track of the original cwd so we can go back to it at the end.
  original_working_dir = os.getcwd()

  # Create a directory to run 'make' from.
  os.mkdir(icu_build_dir)
  os.chdir(icu_build_dir)

  # Build the ICU tools.
  print 'Configuring ICU tools...'
  subprocess.check_call(['%s/runConfigureICU' % icu4c_dir, 'Linux'])

  os.chdir(original_working_dir)

def icuDir():
  return icu_dir

def MakeTzDataFiles(icu_build_dir, data_filename):
  # Keep track of the original cwd so we can go back to it at the end.
  original_working_dir = os.getcwd()

  # Fix missing files.
  os.chdir('%s/tools/tzcode' % icu_build_dir)

  # The tz2icu tool only picks up icuregions and icuzones in they are in the CWD
  for icu_data_file in [ 'icuregions', 'icuzones']:
    icu_data_file_source = '%s/tools/tzcode/%s' % (icu4c_dir, icu_data_file)
    icu_data_file_symlink = './%s' % icu_data_file
    os.symlink(icu_data_file_source, icu_data_file_symlink)

  shutil.copyfile('%s/%s' % (original_working_dir, data_filename),
                  data_filename)

  print 'Making ICU tz data files...'
  # The Makefile assumes the existence of the bin directory.
  os.mkdir('%s/bin' % icu_build_dir)
  subprocess.check_call(['make'])

  # Copy the source file to its ultimate destination.
  icu_txt_data_dir = '%s/data/misc' % icu4c_dir
  print 'Copying zoneinfo64.txt to %s ...' % icu_txt_data_dir
  shutil.copy('zoneinfo64.txt', icu_txt_data_dir)

  os.chdir(original_working_dir)


def MakeAndCopyIcuDataFiles(icu_build_dir):
  # Keep track of the original cwd so we can go back to it at the end.
  original_working_dir = os.getcwd()

  # Regenerate the .dat file.
  os.chdir(icu_build_dir)
  subprocess.check_call(['make', 'INCLUDE_UNI_CORE_DATA=1', '-j32'])

  # Copy the .dat file to its ultimate destination.
  icu_dat_data_dir = '%s/stubdata' % icu4c_dir
  datfiles = glob.glob('data/out/tmp/icudt??l.dat')
  if len(datfiles) != 1:
    print 'ERROR: Unexpectedly found %d .dat files (%s). Halting.' % (len(datfiles), datfiles)
    sys.exit(1)
  datfile = datfiles[0]
  print 'Copying %s to %s ...' % (datfile, icu_dat_data_dir)
  shutil.copy(datfile, icu_dat_data_dir)

  # Generate the ICU4J .jar files
  os.chdir('%s/data' % icu_build_dir)
  subprocess.check_call(['make', 'icu4j-data'])

  # Copy the ICU4J .jar files to their ultimate destination.
  icu_jar_data_dir = '%s/main/shared/data' % icu4j_dir
  jarfiles = glob.glob('out/icu4j/*.jar')
  if len(jarfiles) != 2:
    print 'ERROR: Unexpectedly found %d .jar files (%s). Halting.' % (len(jarfiles), jarfiles)
    sys.exit(1)
  for jarfile in jarfiles:
    print 'Copying %s to %s ...' % (jarfile, icu_jar_data_dir)
    shutil.copy(jarfile, icu_jar_data_dir)

  # Switch back to the original working cwd.
  os.chdir(original_working_dir)

# Run with no arguments from any directory, with no special setup required.
def main():
  i18nutil.SwitchToNewTemporaryDirectory()
  icu_build_dir = '%s/icu' % os.getcwd()

  print 'Found icu in %s ...' % icu_dir

  PrepareIcuBuild(icu_build_dir)

  MakeAndCopyIcuDataFiles(icu_build_dir)

  print 'Look in %s for new data files' % icu_dir
  sys.exit(0)

if __name__ == '__main__':
  main()
