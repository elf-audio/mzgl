# 05_Plugin - one mzgl plugin, every format

A stereo gain with a single automatable parameter, built as a **VST3**, an
**AUv2** and an **AUv3** (with its container app) from one `Plugin` and one
`PluginEditor`, plus a script that turns the built bundles into an installer
`.pkg`. Copy this folder to start your own plug-in.

```
src/GainPlugin.h      the ::Plugin - parameters, process(), state (de)serialisation
src/GainEditor.h      the PluginEditor (an mzgl App) - draws the fader, talks to the host
src/entryPoints.cpp   instantiatePlugin() / instantiatePluginEditor() / isPlugin()
src/GainVST3.cpp      VST3 glue: MzglVST3SingleComponent subclass + class factory (~40 lines)
src/GainAUv2.cpp      AUv2 glue: MzglAUv2Effect subclass + AUSDK_COMPONENT_ENTRY (~25 lines)
src/version.h         strings for the VST3 factory
CMakeLists.txt        the four targets, via cmake/plugin/mzgl-plugin.cmake
scripts/package.sh    pkgbuild/productbuild installer (optionally signed + notarized)
```

The AUv3 needs no glue file at all: `mzgl_add_auv3_plugin()` generates the
principal class (a subclass of mzgl's `AudioUnitViewController`), its nib, both
Info.plists and the container app.

| Target | Product | Generator | Wrapper |
|---|---|---|---|
| `05_PluginVST3` | `mzgl Gain.vst3` | any | `lib/mzgl/plugin/vst3/MzglVST3SingleComponent` |
| `05_PluginAUv2` | `mzgl Gain.component` | any (macOS) | `lib/mzgl/plugin/auv2/MzglAUv2Effect` (Apple AudioUnitSDK) |
| `05_PluginAUv3` | `05_PluginAUv3.appex` | Xcode only | mzgl `AudioUnitViewController` / `MZGLEffectAU` |
| `05_PluginAUv3App` | `mzgl Gain.app` | Xcode only | `lib/mzgl/plugin/auv3/MzglAUv3ContainerApp.mm` |

The AUv2 and AUv3 publish the same AudioComponent (`aufx` / `Gain` / `Mzgl`,
"mzgl: Gain"). Install one or the other; hosts list both otherwise.

## Build

From the `examples` folder:

```bash
# VST3 + AUv2 (Ninja or any generator)
cmake -Bbuild -GNinja
cmake --build build --target 05_PluginVST3 05_PluginAUv2
#  -> examples/bin/mzgl Gain.vst3, examples/bin/mzgl Gain.component

# AUv3 + container app (Xcode generator)
cmake -Bbuild-xcode -GXcode                       # add -DMZGL_DEVELOPMENT_TEAM=XXXXXXXXXX to sign properly
cmake --build build-xcode --target 05_PluginAUv3App --config Release
#  -> examples/build-xcode/05_Plugin/Release/mzgl Gain.app  (appex in Contents/PlugIns)
```

The first configure fetches the Steinberg VST3 SDK and Apple's AudioUnitSDK
with FetchContent (a few minutes). The VST3/AUv2 bundles are ad-hoc signed by
the build and complete (PkgInfo, Info.plist), so they load straight from
`examples/bin`; `cmake --install build` copies them to `~/Library/Audio/Plug-Ins`.

### Try it

```bash
# AUv2
cp -R "bin/mzgl Gain.component" ~/Library/Audio/Plug-Ins/Components/
auval -v aufx Gain Mzgl

# VST3
cp -R "bin/mzgl Gain.vst3" ~/Library/Audio/Plug-Ins/VST3/
# then open it in any VST3 host

# AUv3: run the container app once - it registers the appex with pluginkit;
# hosts then list "mzgl: Gain". (Unregister with: pluginkit -r -i com.elf-audio.mzgl.Gain.AUv3)
open "build-xcode/05_Plugin/Release/mzgl Gain.app"
auval -v aufx Gain Mzgl      # validates the AUv3 too (the first run right after
                             # registering can fail on channel setup while the
                             # extension process spins up - just run it again)
```

Without `MZGL_DEVELOPMENT_TEAM` the appex/app are ad-hoc signed ("Sign to Run
Locally") which is enough for local hosts on this machine but not distributable.

## Package

```bash
05_Plugin/scripts/package.sh                                        # unsigned pkg with whatever was built
05_Plugin/scripts/package.sh --sign "Developer ID Application: You (TEAMID)" \
                             --notarize my-notary-profile           # signed, notarized, stapled
#  -> examples/bin/mzgl-gain-0.1.0.pkg
```

Each built format becomes a selectable choice in the installer: VST3 to
`/Library/Audio/Plug-Ins/VST3`, AUv2 to `/Library/Audio/Plug-Ins/Components`,
the AUv3 container app to `/Applications`. `--notarize` takes a keychain profile
made with `xcrun notarytool store-credentials`.

## Making your own

1. Copy `05_Plugin`, rename the targets/strings in `CMakeLists.txt` and
   `src/version.h`, generate a new `FUID` in `GainVST3.cpp` (`uuidgen`) and pick
   your own `AU_SUBTYPE`/`AU_MANUFACTURER` four-character codes.
2. Replace `GainPlugin` with your DSP: register parameters in the constructor
   (their index is the host parameter ID in all three formats - only ever append),
   implement `process()`, `serialize()`/`deserialize()`.
3. Replace `GainEditor` with your UI. Push UI changes to the host with
   `effect.updateHostParameter(index, value)`; read parameters each frame to
   pick up host automation. Bracket drags with
   `beginIgnoringAutomation()`/`endIgnoringAutomation()`.
4. If your UI needs mzgl `data/` assets (fonts, SVGs), pass `DATA_DIR <dir>`
   (and optionally `DATA_SUBDIRS <sub>...`) to the `mzgl_add_*_plugin` calls;
   it is copied into every bundle's `Contents/Resources/data` and `dataPath()`
   resolves there at runtime.
5. Instruments: `setIsInstrument(true)` in the plugin, `cfg.audioInput = false;
   cfg.midiInput = true` in the VST3 config, `AU_TYPE aumu` for the AUs. For the
   AUv2 subclass `mzglau::MzglAUv2Instrument` instead of `MzglAUv2Effect` and
   declare it with `AUSDK_COMPONENT_ENTRY(ausdk::AUMusicDeviceFactory, ...)`; it
   has no audio input and one stereo output bus per `getNumOutputBusses()`.
   One `.component` can publish several AudioComponents (an instrument and a
   MIDI effect around the same plugin, say): give each its own subclass +
   entry macro, supply your own `INFO_PLIST` listing them and pass the extra
   entry points as `EXTRA_FACTORY_FUNCTIONS` (see koala's `plugin/koala`).
