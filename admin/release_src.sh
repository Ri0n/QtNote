#!/bin/bash

set -e

#git clean -xfd .
#git sudmofule foreach git clean -xfd .


gittag2version() {
  # remove hash and sum to last numbers. e.g. v1.0.5-35-gdfsdfggw => v1.0.40
  # (so please avoid using anything but 0 in the patch part of version)
  local git_version=$(echo "$1" | sed 's/-g[a-z0-9]\{7,\}//')
  gv_lp=${git_version##*.}
  case "$gv_lp" in
    *-*) build_v=$(expr ${gv_lp%-*} + ${gv_lp#*-}) ;;
    *) build_v="$gv_lp" ;;
  esac
  echo "${git_version%.*}.$build_v"
}

version=$(gittag2version ${1:-$(git describe --tags 2>/dev/null)})
base_name=qtnote-$version
git archive --prefix ${base_name}/ -o ../${base_name}.tar HEAD
git submodule --quiet foreach 'cd $toplevel; tar --transform "s|^|'$base_name/'|" -rf ../'"$base_name"'.tar $sm_path'


tar -vf ../${base_name}.tar --delete ${base_name}/PKGBUILD # it has invalid hashsum
echo "$version" > version
tar --transform "s|^|$base_name/|" -rf ../${base_name}.tar version
rm version
# TODO patch other files having version

xz -fz9 ../${base_name}.tar
echo "result: ../${base_name}.tar.xz"
