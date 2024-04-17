QtNote
======
It's just very small Qt app which usually lives in your system tray and allows you to make notes quick way.

QtNote was written as a clone of Tomboy with use of Qt.
It's not so feature rich as Tomboy but light and fast and usually suits all common needs.
Moreover QtNote can work with Tomboy notes and it's not so hard add support of other apps.

Some features:
* Quick access to notes from tray menu
* Internal notes manager to handle multiple notes at once / search notes
* Support for Tomboy notes
* Create notes from selection by hotkey
* Spell checking
* Cross-platform (tested on Linux and Windows with gcc and Visual Studio)
* Other: configurable amount of notes in menu. configurable storage path, print note, save note dialog geometry


## Downloads

Check https://github.com/Ri0n/QtNote/releases page for the latest downloads.

Some older releases could be found at https://yadi.sk/d/HbnqnaTN6fwzN.

## Compilation
```bash
$ cmake -B build && cmake --build build -j12 --target install
```

If installation to the default (usually `/usr/local`) prefix is not needed then remove `--target install`.

If you are curious about available options try next command in the source directory

```bash
$ cmake -LA . | grep QTNOTE
```

* QTNOTE_DEVEL - development mode (it's rather about resources (e.g. plugins) search paths. )
* QTNOTE_PLUGIN_ - enable/disable plugins. defaults are automatic

## Build DEB/RPM

Check ./admin/{deb,rpm}build.sh scripts. You can start them w/o arguments.

## Internationalization

https://app.transifex.com/rion/qtnote
