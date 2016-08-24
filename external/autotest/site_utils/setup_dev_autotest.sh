#!/bin/bash
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
set -e

USAGE='Usage: setup_dev_autotest.sh [-pavnm]'
HELP="${USAGE}\n\n\
Install and configure software needed to run autotest locally.\n\
If you're just working on tests, you do not need to run this.\n\n\
Options:\n\
  -p Desired Autotest DB password. Must be non-empty.\n\
  -a Absolute path to autotest source tree.\n\
  -v Show info logging from build_externals.py and compile_gwt_clients.py \n\
  -n Non-interactive mode, doesn't ask for any user input.
     Requires -p and -a to be set.\n\
  -m Allow remote access for database."

function get_y_or_n_interactive {
    local ret
    while true; do
        read -p "$2" yn
        case $yn in
            [Yy]* ) ret="y"; break;;
            [Nn]* ) ret="n"; break;;
            * ) echo "Please enter y or n.";;
        esac
    done
    eval $1="'$ret'"
}

function get_y_or_n {
  local ret=$3
  if [ "${noninteractive}" = "FALSE" ]; then
    get_y_or_n_interactive sub "$2"
    ret=$sub
  fi
  eval $1="'$ret'"
}

AUTOTEST_DIR=
PASSWD=
verbose="FALSE"
noninteractive="FALSE"
remotedb="FALSE"
while getopts ":p:a:vnmh" opt; do
  case ${opt} in
    a)
      AUTOTEST_DIR=$OPTARG
      ;;
    p)
      PASSWD=$OPTARG
      ;;
    v)
      verbose="TRUE"
      ;;
    n)
      noninteractive="TRUE"
      ;;
    m)
      remotedb="TRUE"
      ;;
    h)
      echo -e "${HELP}" >&2
      exit 0
      ;;
    \?)
      echo "Invalid option: -$OPTARG" >&2
      echo -e "${HELP}" >&2
      exit 1
      ;;
    :)
      echo "Option -$OPTARG requires an argument." >&2
      echo -e "${HELP}" >&2
      exit 1
      ;;
  esac
done

if [[ $EUID -eq 0 ]]; then
  echo "Running with sudo / as root is not recommended"
  get_y_or_n verify "Continue as root? [y/N]: " "n"
  if [[ "${verify}" = 'n' ]]; then
    echo "Bailing!"
    exit 1
  fi
fi

if [ "${noninteractive}" = "TRUE" ]; then
  if [ -z "${AUTOTEST_DIR}" ]; then
    echo "-a must be specified in non-interactive mode." >&2
    exit 1
  fi
  if [ -z "${PASSWD}" ]; then
    echo "-p must be specified in non-interactive mode." >&2
    exit 1
  fi
fi


if [ -z "${PASSWD}" ]; then
  read -s -p "Autotest DB password: " PASSWD
  echo
  if [ -z "${PASSWD}" ]; then
    echo "Empty passwords not allowed." >&2
    exit 1
  fi
  read -s -p "Re-enter password: " PASSWD2
  echo
  if [ "${PASSWD}" != "${PASSWD2}" ]; then
    echo "Passwords don't match." >&2
    exit 1
  fi
fi

if [ -z "${AUTOTEST_DIR}" ]; then
  CANDIDATE=$(dirname "$(readlink -f "$0")" | egrep -o '(/[^/]+)*/files')
  read -p "Enter autotest dir [${CANDIDATE}]: " AUTOTEST_DIR
  if [ -z "${AUTOTEST_DIR}" ]; then
    AUTOTEST_DIR="${CANDIDATE}"
  fi
fi


# Sanity check AUTOTEST_DIR. If it's null, or doesn't exist on the filesystem
# then die.
if [ -z "${AUTOTEST_DIR}" ]; then
  echo "No AUTOTEST_DIR. Aborting script."
  exit 1
fi

if [ ! -d "${AUTOTEST_DIR}" ]; then
  echo "Directory " ${AUTOTEST_DIR} " does not exist. Aborting script."
  exit 1
fi


SHADOW_CONFIG_PATH="${AUTOTEST_DIR}/shadow_config.ini"
echo "Autotest supports local overrides of global configuration through a "
echo "'shadow' configuration file.  Setting one up for you now."
CLOBBER=0
if [ -f ${SHADOW_CONFIG_PATH} ]; then
  get_y_or_n clobber "Clobber existing shadow config? [Y/n]: " "n"
  if [[ "${clobber}" = 'n' ]]; then
    CLOBBER=1
    echo "Refusing to clobber existing shadow_config.ini."
  else
    echo "Clobbering existing shadow_config.ini."
  fi
fi

CROS_CHECKOUT=$(readlink -f ${AUTOTEST_DIR}/../../../..)

# Create clean shadow config if we're replacing it/creating a new one.
if [ $CLOBBER -eq 0 ]; then
  cat > "${SHADOW_CONFIG_PATH}" <<EOF
[AUTOTEST_WEB]
host: localhost
password: ${PASSWD}
readonly_host: localhost
readonly_user: chromeosqa-admin
readonly_password: ${PASSWD}

[SERVER]
hostname: localhost

[SCHEDULER]
drones: localhost

[CROS]
source_tree: ${CROS_CHECKOUT}
EOF
  echo -e "Done!\n"
fi

echo "Installing needed Ubuntu packages..."
PKG_LIST="mysql-server mysql-common libapache2-mod-wsgi python-mysqldb \
gnuplot apache2-mpm-prefork unzip python-imaging libpng12-dev libfreetype6-dev \
sqlite3 python-pysqlite2 git-core pbzip2 openjdk-6-jre openjdk-6-jdk \
python-crypto  python-dev subversion build-essential python-setuptools \
python-numpy python-scipy"

if ! sudo apt-get install -y ${PKG_LIST}; then
  echo "Could not install packages: $?"
  exit 1
fi
echo -e "Done!\n"

# Check if database exists, clobber existing database with user consent.
#
# Arguments: Name of the database
check_database()
{
  local db_name=$1
  echo "Setting up Database: $db_name in MySQL..."
  if mysql -u root -e ';' 2> /dev/null ; then
    PASSWD_STRING=
  elif mysql -u root -p"${PASSWD}" -e ';' 2> /dev/null ; then
    PASSWD_STRING="-p${PASSWD}"
  else
    PASSWD_STRING="-p"
  fi

  if ! mysqladmin -u root "${PASSWD_STRING}" ping ; then
    sudo service mysql start
  fi

  local clobberdb='y'
  local existing_database=$(mysql -u root "${PASSWD_STRING}" -e "SELECT \
  SCHEMA_NAME FROM INFORMATION_SCHEMA.SCHEMATA WHERE SCHEMA_NAME = '$db_name'")

  if [ -n "${existing_database}" ]; then
    get_y_or_n clobberdb "Clobber existing MySQL database? [Y/n]: " "n"
  fi

  local sql_priv="GRANT ALL PRIVILEGES ON $db_name.* TO \
  'chromeosqa-admin'@'localhost' IDENTIFIED BY '${PASSWD}';"

  if [ "${remotedb}" = "TRUE" ]; then
    sql_priv="${sql_priv} GRANT ALL PRIVILEGES ON $db_name.* TO \
    'chromeosqa-admin'@'%' IDENTIFIED BY '${PASSWD}';"
  fi

  local sql_command="drop database if exists $db_name; \
  create database $db_name; \
  ${sql_priv} FLUSH PRIVILEGES;"

  if [[ "${clobberdb}" = 'y' ]]; then
    mysql -u root "${PASSWD_STRING}" -e "${sql_command}"
  fi
  echo -e "Done!\n"
}

check_database 'chromeos_autotest_db'
check_database 'chromeos_lab_servers'

AT_DIR=/usr/local/autotest
echo -n "Bind-mounting your autotest dir at ${AT_DIR}..."
sudo mkdir -p "${AT_DIR}"
sudo mount --bind "${AUTOTEST_DIR}" "${AT_DIR}"
echo -e "Done!\n"

sudo chown -R "$(whoami)" "${AT_DIR}"

EXISTING_MOUNT=$(egrep "/.+[[:space:]]${AT_DIR}" /etc/fstab || /bin/true)
if [ -n "${EXISTING_MOUNT}" ]; then
  echo "${EXISTING_MOUNT}" | awk '{print $1 " already automounting at " $2}'
  echo "We won't update /etc/fstab, but you should have a line line this:"
  echo -e "${AUTOTEST_DIR}\t${AT_DIR}\tbind defaults,bind\t0\t0"
else
  echo -n "Adding aforementioned bind-mount to /etc/fstab..."
  # Is there a better way to elevate privs and do a redirect?
  sudo su -c \
    "echo -e '${AUTOTEST_DIR}\t${AT_DIR}\tbind defaults,bind\t0\t0' \
    >> /etc/fstab"
  echo -e "Done!\n"
fi

echo -n "Reticulating splines..."

if [ "${verbose}" = "TRUE" ]; then
  "${AT_DIR}"/utils/build_externals.py
  "${AT_DIR}"/utils/compile_gwt_clients.py -a
else
  "${AT_DIR}"/utils/build_externals.py &> /dev/null
  "${AT_DIR}"/utils/compile_gwt_clients.py -a &> /dev/null
fi

echo -e "Done!\n"

echo "Populating autotest mysql DB..."
"${AT_DIR}"/database/migrate.py sync -f
"${AT_DIR}"/frontend/manage.py syncdb --noinput
# You may have to run this twice.
"${AT_DIR}"/frontend/manage.py syncdb --noinput
"${AT_DIR}"/utils/test_importer.py
echo -e "Done!\n"

echo "Initializing chromeos_lab_servers mysql DB..."
"${AT_DIR}"/database/migrate.py sync -f -d AUTOTEST_SERVER_DB
echo -e "Done!\n"

echo "Configuring apache to run the autotest web interface..."
if [ ! -d /etc/apache2/run ]; then
  sudo mkdir /etc/apache2/run
fi
sudo ln -sf "${AT_DIR}"/apache/apache-conf \
  /etc/apache2/sites-available/autotest-server.conf
# Disable currently active default
sudo a2dissite 000-default default || true
# Enable autotest server
sudo a2ensite autotest-server.conf
# Enable rewrite module
sudo a2enmod rewrite
# Enable wsgi
sudo a2enmod wsgi
# Enable version
# built-in on trusty
sudo a2enmod version || true
# Enable headers
sudo a2enmod headers
# Enable cgid
sudo a2enmod cgid
# Setup permissions so that Apache web user can read the proper files.
chmod -R o+r "${AT_DIR}"
find "${AT_DIR}"/ -type d -print0 | xargs --null chmod o+x
chmod o+x "${AT_DIR}"/tko/*.cgi
# restart server
sudo /etc/init.d/apache2 restart

# Setup lxc and base container for server-side packaging support.
sudo apt-get install lxc -y
sudo python "${AT_DIR}"/site_utils/lxc.py -s

echo "Browse to http://localhost to see if Autotest is working."
echo "For further necessary set up steps, see https://sites.google.com/a/chromium.org/dev/chromium-os/testing/autotest-developer-faq/setup-autotest-server?pli=1"
