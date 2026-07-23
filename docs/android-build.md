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

Android and desktop now share `NotesModel`, `NotesSearchModel`,
`RecentNotesModel`, `NotesWorkspaceController`, `PluginListModel`, and
`StoragePriorityModel`. Android opens in the flat Recent view; the shared manager
also provides the storage-grouped tree.

PTF is registered through the same core startup function on both platforms.
Android plugin discovery uses the explicit bundled factory registry documented
in [Android bundled plugin loading](mobile-plugin-loading.md). The registry is
active, but its allow-list is empty until existing plugins are split from their
Widgets/Main/DBus dependencies.

Plugin and storage settings still need the UI-neutral contract described in
[Common settings API plan](settings-api-plan.md). Until that contract is
implemented, desktop QWidget settings remain the only complete settings UI.
