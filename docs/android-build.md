# Android build

QtNote currently has an experimental Qt Quick mobile target. An Android kit
selects it automatically through `QTNOTE_BUILD_MOBILE=ON`; desktop builds keep
using the existing Widgets application.

## Qt Creator setup

1. Install one matching Qt 6 version for both Desktop and Android. Include the
   `arm64-v8a` Android architecture and Qt Quick Controls.
2. In **Preferences > SDKs > Android**, configure a 64-bit JDK and let Qt
   Creator install the SDK, NDK and build tools required by that Qt version.
3. Open the repository's root `CMakeLists.txt` and select the generated Android
   `arm64-v8a` kit.
4. Configure the project. `QTNOTE_BUILD_MOBILE` is enabled automatically when
   the Android toolchain sets `ANDROID`; the build target is `qtnote_mobile`.
5. Select an emulator or a device with USB debugging enabled, then Build and
   Run. Qt Creator invokes `androiddeployqt` and packages the target as an APK.

To exercise the same QML shell with a desktop kit before deploying it, add
`-DQTNOTE_BUILD_MOBILE=ON` to the CMake configuration and build
`qtnote_mobile`.

## Current boundary

The shared editor controller and its migration plan are documented in
[Note editor architecture](note-editor-architecture.md).

The mobile target currently contains navigation for notes, plugins, application
settings and per-plugin settings. Application settings are persisted through
`QSettings`. The note and plugin models are intentionally empty until the
desktop core is separated from these Widgets-only APIs:

- `NoteStorage::settingsWidget()` returns `QWidget *`;
- `PluginOptionsInterface::optionsDialog()` returns `QDialog *`;
- `QtNote::Main` requires tray, desktop integration, dialogs and global
  shortcuts during startup;
- desktop plugins are discovered as host shared libraries and several depend
  on desktop-only libraries.

The next core change should introduce UI-neutral settings schemas/controllers
for storages and plugins. Desktop can render those schemas with Widgets while
mobile renders them with QML. Plugins that need custom controls can expose a
mobile QML component in addition to the schema. After that, the existing note
and plugin models can be adapted to QML role names and connected to the mobile
application object.
