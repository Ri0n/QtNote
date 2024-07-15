#!/bin/sh

sudo apt-get install libkf5globalaccel-dev libkf5windowsystem-dev libkf5notifications-dev git-buildpackage debhelper \
  pkgconf dh-cmake dh-sequence-cmake libqt5x11extras5-dev libhunspell-dev

$(dirname $0)/debrelease

export QT_SELECT=5
exec gbp buildpackage --git-ignore-branch --git-ignore-new -us -uc -j10
