pkgname=qtnote
pkgver=3.0.5
pkgrel=1
pkgdesc="Note-taking application written with Qt in mind"
arch=('i686' 'x86_64')
url="http://ri0n.github.io/QtNote"
license=('GPL3')
depends=( 'qt5-base' 'qt5-tools' 'hunspell'
          'kglobalaccel' 'kwindowsystem'
          'knotifications')
makedepends=('tar')
conflicts=(qtnote-git)
source=(https://github.com/Ri0n/QtNote/archive/$pkgver.tar.gz)
md5sums=('4e119a4ee9bf9b4520f3a5b6a64ed1b9')

_srcdirname=QtNote

build() {
  cd "$srcdir"
  # BUILD HERE
  cd "$srcdir/$_srcdirname-$pkgver"
  cmake -DCMAKE_INSTALL_PREFIX="/usr" .
  make
}

package() {
  cd "$srcdir/$_srcdirname-$pkgver"
  make INSTALL_ROOT="$pkgdir/"  install
}
