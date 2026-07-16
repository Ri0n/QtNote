# Debian package profiles

QtNote uses separate Debian source-package profiles because APT resolves
`Build-Depends` before `debian/rules` is run. Qt major versions therefore
cannot be selected reliably with alternative dependencies in one control
file.

Available profiles:

- `qt6`: Qt 6, KF6 and Plasma 6 dependencies.
- `qt6-noble`: Qt 6 dependencies available in Ubuntu 24.04 Noble and Linux
  Mint 22.x. It builds QXmpp 1.15.1 with OMEMO as a bundled ExternalProject.
  QXmpp is linked statically into the XMPP plugin, so no private QXmpp shared
  libraries are shipped. KF6 integration and the Plasma 6 plasmoid are not
  built.

Build binary and source packages with:

```sh
packaging/build-deb qt6-noble -us -uc
packaging/build-deb qt6 -us -uc
```

To create a source-only package for a PPA:

```sh
packaging/build-deb qt6-noble -S -sa
```

The selected Qt major is copied into `debian/build-profile.mk`, so it remains
part of the generated source package and is used later by the binary package
builder. The tracked `debian/control` and `debian/build-profile.mk` files are
restored when the command exits.

The Noble profile downloads the checksum-pinned official QXmpp release
tarball during a local build. For a network-isolated build, unpack the same
release beforehand and configure with:

```sh
-DQTNOTE_BUILD_BUNDLED_QXMPP=ON \
-DQTNOTE_QXMPP_SOURCE_DIR=/path/to/qxmpp-1.15.1
```

Set `-DQTNOTE_BUNDLED_QXMPP_STATIC=OFF` to build and package private shared
QXmpp libraries instead.

An arbitrary package builder can be run under a selected profile after `--`:

```sh
packaging/build-deb qt6-noble -- gbp buildpackage -us -uc
```
