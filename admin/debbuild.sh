#!/bin/sh

sudo apt-get install libkf5globalaccel-dev libkf5windowsystem-dev libkf5notifications-dev

export QT_SELECT=5
exec gbp buildpackage --git-ignore-branch --git-ignore-new -us -uc -j5
