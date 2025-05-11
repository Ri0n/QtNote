#!/bin/sh

sudo apt-get install qt6-base-dev qt6-base-dev-tools qt6-tools-dev qt6-declarative-dev \
  extra-cmake-modules libplasma-dev libkf6globalaccel-dev libkf6windowsystem-dev \
  libkf6notifications-dev git-buildpackage debhelper pkgconf dh-cmake dh-sequence-cmake \
  libhunspell-dev

$(dirname $0)/debrelease

export QT_SELECT=6
exec gbp buildpackage --git-ignore-branch --git-ignore-new -us -uc -j10
