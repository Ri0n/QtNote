pkgname=qtnote
pkgver=3.2.0
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
md5sums=('fccd107d2130cd77da9a21edfb76c927')

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
