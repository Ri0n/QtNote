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

Android and desktop share `NotesModel`, `NotesSearchModel`, `RecentNotesModel`,
`NotesWorkspaceController`, `PluginListModel`, `StoragePriorityModel`,
`NoteEditor`, the structured QML editor, toolbar and settings controllers.
Android opens in the flat Recent view; desktop defaults to the storage-grouped
tree.

PTF is registered through the same core startup function on both platforms.
Android plugin discovery uses the explicit bundled factory registry documented
in [Android bundled plugin loading](mobile-plugin-loading.md). Gemini, OpenAI
Whisper and Nextcloud are currently in the allow-list. Plugin and storage
settings use the common QML/controller contract documented in
[Common settings API](settings-api-plan.md).

The remaining Android work is device hardening: IME and predictive-input tests,
background/process-death recovery, rotation and selection restoration, runtime
permissions and storage access, bundled crypto/native-library verification,
arm64 device builds, release signing and package metadata.
