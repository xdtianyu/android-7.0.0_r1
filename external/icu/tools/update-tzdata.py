#!/usr/bin/python

"""Updates the timezone data held in bionic and ICU."""

import ftplib
import glob
import httplib
import os
import re
import shutil
import subprocess
import sys
import tarfile

import i18nutil
import updateicudata

regions = ['africa', 'antarctica', 'asia', 'australasia',
           'etcetera', 'europe', 'northamerica', 'southamerica',
           # These two deliberately come last so they override what came
           # before (and each other).
           'backward', 'backzone' ]

# Find the bionic directory.
android_build_top = i18nutil.GetAndroidRootOrDie()
bionic_dir = os.path.realpath('%s/bionic' % android_build_top)
bionic_libc_zoneinfo_dir = '%s/libc/zoneinfo' % bionic_dir
i18nutil.CheckDirExists(bionic_libc_zoneinfo_dir, 'bionic/libc/zoneinfo')
tools_dir = '%s/external/icu/tools' % android_build_top
i18nutil.CheckDirExists(tools_dir, 'external/icu/tools')

def GetCurrentTzDataVersion():
  return open('%s/tzdata' % bionic_libc_zoneinfo_dir).read().split('\x00', 1)[0]


def WriteSetupFile():
  """Writes the list of zones that ZoneCompactor should process."""
  links = []
  zones = []
  for region in regions:
    for line in open('extracted/%s' % region):
      fields = line.split()
      if fields:
        if fields[0] == 'Link':
          links.append('%s %s %s' % (fields[0], fields[1], fields[2]))
          zones.append(fields[2])
        elif fields[0] == 'Zone':
          zones.append(fields[1])
  zones.sort()

  setup = open('setup', 'w')
  for link in sorted(set(links)):
    setup.write('%s\n' % link)
  for zone in sorted(set(zones)):
    setup.write('%s\n' % zone)
  setup.close()


def FtpRetrieveFile(ftp, filename):
  ftp.retrbinary('RETR %s' % filename, open(filename, 'wb').write)


def FtpRetrieveFileAndSignature(ftp, data_filename):
  """Downloads and repackages the given data from the given FTP server."""
  print 'Downloading data...'
  FtpRetrieveFile(ftp, data_filename)

  print 'Downloading signature...'
  signature_filename = '%s.asc' % data_filename
  FtpRetrieveFile(ftp, signature_filename)


def HttpRetrieveFile(http, path, output_filename):
  http.request("GET", path)
  f = open(output_filename, 'wb')
  f.write(http.getresponse().read())
  f.close()


def HttpRetrieveFileAndSignature(http, data_filename):
  """Downloads and repackages the given data from the given HTTP server."""
  path = "/time-zones/repository/releases/%s" % data_filename

  print 'Downloading data...'
  HttpRetrieveFile(http, path, data_filename)

  print 'Downloading signature...'
  signature_filename = '%s.asc' % data_filename
  HttpRetrievefile(http, "%s.asc" % path, signature_filename)

def BuildIcuToolsAndData(data_filename):
  icu_build_dir = '%s/icu' % os.getcwd()

  updateicudata.PrepareIcuBuild(icu_build_dir)
  updateicudata.MakeTzDataFiles(icu_build_dir, data_filename)
  updateicudata.MakeAndCopyIcuDataFiles(icu_build_dir)

def CheckSignature(data_filename):
  signature_filename = '%s.asc' % data_filename
  print 'Verifying signature...'
  # If this fails for you, you probably need to import Paul Eggert's public key:
  # gpg --recv-keys ED97E90E62AA7E34
  subprocess.check_call(['gpg', '--trusted-key=ED97E90E62AA7E34', '--verify',
                         signature_filename, data_filename])


def BuildBionicToolsAndData(data_filename):
  new_version = re.search('(tzdata.+)\\.tar\\.gz', data_filename).group(1)

  print 'Extracting...'
  os.mkdir('extracted')
  tar = tarfile.open(data_filename, 'r')
  tar.extractall('extracted')

  print 'Calling zic(1)...'
  os.mkdir('data')
  zic_inputs = [ 'extracted/%s' % x for x in regions ]
  zic_cmd = ['zic', '-d', 'data' ]
  zic_cmd.extend(zic_inputs)
  subprocess.check_call(zic_cmd)

  WriteSetupFile()

  print 'Calling ZoneCompactor to update bionic to %s...' % new_version
  subprocess.check_call(['javac', '-d', '.',
                         '%s/ZoneCompactor.java' % tools_dir])
  subprocess.check_call(['java', 'ZoneCompactor',
                         'setup', 'data', 'extracted/zone.tab',
                         bionic_libc_zoneinfo_dir, new_version])


# Run with no arguments from any directory, with no special setup required.
# See http://www.iana.org/time-zones/ for more about the source of this data.
def main():
  print 'Found bionic in %s ...' % bionic_dir
  print 'Found icu in %s ...' % updateicudata.icuDir()

  print 'Looking for new tzdata...'

  tzdata_filenames = []

  # The FTP server lets you download intermediate releases, and also lets you
  # download the signatures for verification, so it's your best choice.
  use_ftp = True

  if use_ftp:
    ftp = ftplib.FTP('ftp.iana.org')
    ftp.login()
    ftp.cwd('tz/releases')
    for filename in ftp.nlst():
      if filename.startswith('tzdata20') and filename.endswith('.tar.gz'):
        tzdata_filenames.append(filename)
    tzdata_filenames.sort()
  else:
    http = httplib.HTTPConnection('www.iana.org')
    http.request("GET", "/time-zones")
    index_lines = http.getresponse().read().split('\n')
    for line in index_lines:
      m = re.compile('.*href="/time-zones/repository/releases/(tzdata20\d\d\c\.tar\.gz)".*').match(line)
      if m:
        tzdata_filenames.append(m.group(1))

  # If you're several releases behind, we'll walk you through the upgrades
  # one by one.
  current_version = GetCurrentTzDataVersion()
  current_filename = '%s.tar.gz' % current_version
  for filename in tzdata_filenames:
    if filename > current_filename:
      print 'Found new tzdata: %s' % filename
      i18nutil.SwitchToNewTemporaryDirectory()
      if use_ftp:
        FtpRetrieveFileAndSignature(ftp, filename)
      else:
        HttpRetrieveFileAndSignature(http, filename)

      CheckSignature(filename)
      BuildIcuToolsAndData(filename)
      BuildBionicToolsAndData(filename)
      print 'Look in %s and %s for new data files' % (bionic_dir, updateicudata.icuDir())
      sys.exit(0)

  print 'You already have the latest tzdata in bionic (%s)!' % current_version
  sys.exit(0)


if __name__ == '__main__':
  main()
