#!/bin/sh

sudo apt-get install libkf5globalaccel-dev libkf5windowsystem-dev libkf5notifications-dev git-buildpackage debhelper cdbs qtchooser pkg-config libqt5x11extras5-dev libhunspell-dev

export QT_SELECT=5
exec gbp buildpackage --git-ignore-branch --git-ignore-new -us -uc -j5
