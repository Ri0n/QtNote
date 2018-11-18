#!/bin/sh

cd $(dirname "$0")/..

if lsb_release -i | grep -i suse; then
  sudo zypper in gcc-c++ libqt5-qtbase-devel libqt5-linguist pkg-config hunspell-devel kwindowsystem-devel knotifications-devel kglobalaccel-devel
  pip install gbp
else
  sudo yum install gcc-c++ pkgconfig qt5-linguist qt5-qtbase-devel hunspell-devel kwindowsystem-devel knotifications-devel kglobalaccel-devel
fi

gbp buildpackage-rpm --git-ignore-branc
