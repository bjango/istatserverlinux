#!/bin/sh
set -e

# This script is meant for quick & easy install via:
#   $ curl -fsSL https://files.bjango.com/istatserverlinux/istatserverlinux.sh -o istatserverlinux.sh
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
  else
    lsb_dist="$(uname)"
  fi

  echo "$lsb_dist"
}

get_distribution_like() {
  lsb_dist_like=""

  if [ -r /etc/os-release ]; then
    lsb_dist_like="$(. /etc/os-release && echo "$ID_LIKE")"
  fi

  echo "$lsb_dist_like"
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
  echo "The istatserver config file is located at"
  echo "    /usr/local/etc/istatserver/istatserver.conf"
  echo
  echo "iStat View will ask for a passcode the first time you connect."
  echo "You can edit this passcode in the istatserver config file."
  echo
  echo "Here is your current passcode"
  grep -w /usr/local/etc/istatserver/istatserver.conf -e server_code | grep -v "#" | sed -e "s/server_code//g" | sed -e 's/[ \t]//g'
  echo
  echo "Make sure to take a look at the documentation at:"
  echo "https://bjango.com/help/istat3/istatserverlinux/"
  echo
  echo "Learn how to run istatserver at boot:"
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

  # Some platform detection
  lsb_dist=$( get_distribution )
  lsb_dist="$(echo "$lsb_dist" | tr '[:upper:]' '[:lower:]')"
  
  lsb_dist_like=$( get_distribution_like )
  lsb_dist_like="$(echo "$lsb_dist_like" | tr '[:upper:]' '[:lower:]')"


# Check if OS is supported or find what OS it is like and try that instead
  case "$lsb_dist" in
    ubuntu|debian|raspbian|linuxmint|elementary|centos|fedora|freebsd|dragonfly|netbsd|solus|arch|opensuse|manjaro|slackware|sabayon|gentoo)
      ;;
    *)
	  case "$lsb_dist_like" in
	  	ubuntu|debian)
		  lsb_dist="ubuntu";
		  ;;
	  	fedora)
		  lsb_dist="fedora";
		  ;;
	  	freebsd)
		  lsb_dist="freebsd";
		  ;;
	  	arch)
		  lsb_dist="arch";
		  ;;
	  	opensuse)
		  lsb_dist="opensuse";
		  ;;
	  esac
  esac


  # Run setup for each distribution accordingly
  
  echo "Installing required packages"

  case "$lsb_dist" in
    ubuntu|debian|raspbian|linuxmint|elementary)
      $sh_c "apt-get update -qq > /dev/null"
      $sh_c "apt-get install -y -qq curl g++ autoconf autogen libxml2-dev libssl-dev libsqlite3-dev libsensors4-dev libavahi-common-dev libavahi-client-dev > /dev/null"
      ;;
    centos|fedora)
      if [ -r /bin/dnf ]; then
        $sh_c "dnf -q -y install curl autoconf automake gcc-c++ libxml2-devel openssl-devel sqlite-devel lm_sensors lm_sensors-devel avahi-devel  > /dev/null"
	  else
        $sh_c "yum -q -y install curl autoconf automake gcc-c++ libxml2-devel openssl-devel sqlite-devel lm_sensors lm_sensors-devel avahi-devel  > /dev/null"
	  fi
      ;;
    freebsd|dragonfly)
      $sh_c "env ASSUME_ALWAYS_YES=YES pkg install curl autoconf automake openssl sqlite  > /dev/null"
      ;;
    solus)
      $sh_c "eopkg install -y -c system.devel  > /dev/null"
      $sh_c "eopkg install -y  curl openssl-devel sqlite3-devel lm_sensors-devel  > /dev/null"
      ;;
    opensuse)
      $sh_c "zypper install -y  gcc-c++ libxml2-devel autoconf automake curl openssl-devel sqlite3-devel  > /dev/null"
      ;;
    #openbsd)
    #  $sh_c "pkg_add -I automake-1.9.6p12 autoconf-2.69p2 > /dev/null"
    #  ;;
    arch|manjaro)
      $sh_c "pacman -S --noconfirm automake autoconf openssl sqlite > /dev/null || :"
      ;;
    netbsd)
      $sh_c "pkg_add -I automake autoconf > /dev/null || :"
      ;;
    slackware)
      $sh_c "slackpkg -batch=on -default_answer=y install automake autoconf gcc-g++ curl lm_sensors > /dev/null || :"
      ;;
    sabayon|gentoo)
      $sh_c "emerge --ask=n automake autoconf gcc curl > /dev/null || :"
      ;;
    *)
	  echo "unsupported OS";
      exit 1
      ;;
  esac
  
  if [ -r ./istatserverlinux ]; then
	  $sh_c "rm -r ./istatserverlinux"
  fi

  echo "Downloading istatserver"
  $sh_c "curl -fsSL https://download.bjango.com/istatserverlinux -o istatserverlinux.tar.gz"

  echo "Extracting istatserver"
  $sh_c "tar -zxf istatserverlinux.tar.gz"
  $sh_c "mv istatserver-* istatserverlinux"

  echo "Building istatserver"
  $sh_c "cd istatserverlinux && ./autogen > /dev/null"
  $sh_c "cd istatserverlinux && ./configure > /dev/null"
  $sh_c "cd istatserverlinux && make > /dev/null"
  $sh_c "cd istatserverlinux && make install > /dev/null"

  echo "Cleaning up"
  $sh_c "rm -r ./istatserverlinux > /dev/null"
  $sh_c "rm ./istatserverlinux.tar.gz > /dev/null"
  we_did_it
  exit 0
}

istat_pls
