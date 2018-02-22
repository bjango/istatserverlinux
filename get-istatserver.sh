#!/bin/sh
set -e

# This script is meant for quick & easy install via:
#   $ curl -fsSL https://download.bjango.com/istatserverlinux.sh -o istatserverlinux.sh
#   $ sh istatserverlinux.sh
#
# NOTE: Make sure to verify the contents of the script
#       you downloaded matches the contents of istatserverlinux.sh
#       located at https://github.com/bjango/istatserverlinux
#       before executing.

command_exists() {
  command -v "$@" > /dev/null 2>&1
}

get_distribution() {
  lsb_dist=""

  if [ -r /etc/os-release ]; then
    lsb_dist="$(. /etc/os-release && echo "$ID")"
  fi

  # Returning an empty string here should be alright since the
  # case statements don't act unless you provide an actual value
  echo "$lsb_dist"
}

we_did_it() {
  if command_exists istatserver && [ -e /usr/local/etc/istatserver/istatserver.conf ]; then
    (
      set -x
      $sh_c 'istatserver -v'
    ) || true
  fi

  echo
  echo "Perfetto, you got yourself a brand new iStat Server."
  echo
  echo "You can now run it as a daemon using the following command:"
  echo "    sudo /usr/local/bin/istatserver -d"
  echo
  grep -w /usr/local/etc/istatserver/istatserver.conf -e server_code
  echo "When connecting this iStat Server instance to an iStat View,"
  echo "you can use this code or the one you set in the config file."
  echo
  echo "Make sure to take a look at the documentation at:"
  echo "https://bjango.com/help/istat3/istatserverlinux/"
  echo
  echo "... and yeah, you can run it at boot. Learn how at:"
  echo "https://github.com/bjango/istatserverlinux#starting-istat-server-at-boot"
  echo
}

istat_pls() {
  echo "# Executing iStat Server for Linux install script"

  user="$(id -un 2>/dev/null || true)"

  sh_c='sh -c'
  if [ "$user" != 'root' ]; then
    if command_exists sudo; then
      sh_c='sudo -E sh -c'
    else
      if command_exists su; then
        sh_c='su -c'
      else
        echo "Error: this installer needs the ability to run commands as root."
        echo "We are unable to find either 'sudo' or 'su' available to make this happen."
        exit 1
      fi
    fi
  fi

  # Some latform detection
  lsb_dist=$( get_distribution )
  lsb_dist="$(echo "$lsb_dist" | tr '[:upper:]' '[:lower:]')"

  # Run setup for each distribution accordingly
  case "$lsb_dist" in
    ubuntu|debian|raspbian)
      $sh_c "apt-get update -qq > /dev/null"
      $sh_c "apt-get install -y -qq g++ autoconf autogen libxml2-dev libssl-dev libsqlite3-dev libsensors4-dev libavahi-common-dev libavahi-client-dev > /dev/null"
      $sh_c "curl -fsSL https://download.bjango.com/istatserverlinux -o istatserverlinux.tar.gz"
      $sh_c "tar -zxf istatserverlinux.tar.gz"
      $sh_c "mv istatserver-* istatserverlinux"
      $sh_c "cd istatserverlinux && ./autogen > /dev/null"
      $sh_c "cd istatserverlinux && ./configure > /dev/null"
      $sh_c "cd istatserverlinux && make > /dev/null"
      $sh_c "cd istatserverlinux && make install > /dev/null"
      we_did_it
      exit 0
      ;;
    centos|fedora)
      $sh_c "echo 'meh, still to do'"
      exit 0
      ;;
  esac
  exit 1
}

istat_pls
