# Unbound HQ

Unbound HQ is an Aroma Wii U application for Call of Duty: Black Ops II and Call of Duty: Ghosts. This repository contains the v1.0.0 / r56 application source, runtime assets, tests, and development tools.

The app switches between games, detects regional game and official-update installations, installs verified Core packages and optional map packs to USB or internal storage, manages mods and Aroma Workshop plugins, and shows network activity and release news. ZIPs on SD can be used as installation sources.

HQ installs at sd:/wiiu/apps/Unbound-HQ/Unbound-HQ.wuhb. It migrates the earlier BO2-Unbound-HQ app name and carries saved settings and install records to the current folder.

## Build

The Dockerfile pins the Wii U toolchain and the libmocha and librpxloader artifacts used by HQ.

```sh
docker build -t unbound-hq-builder .
docker run --rm -v "$PWD:/project" unbound-hq-builder make -j6
```

The build produces Unbound-HQ.wuhb at the repository root. Copy it to wiiu/apps/Unbound-HQ/ on the SD card and open it through Aroma.

With devkitPro, wiiu-dev, wiiu-portlibs, libmocha, and librpxloader installed locally, run make directly. Python 3 is used by the asset preparation and publishing tools. The committed content and data directories supply the assets required by the normal build.

## Verify

On a Linux host with a C++20 compiler, SDL2 development libraries, and pkg-config, run the host regression suite.

```sh
make test
python3 tests/install_flow_source_tests.py
python3 tests/sd_package_flow_tests.py
python3 tests/map_publisher_tests.py
```

The optional download_animation_assets_tests.py takes the directory of original GIF files as its required argument and uses Pillow to compare them with the committed animation atlases.

Additional ZIP and curl integration checks are available through make test-zip and make test-curl with their required development libraries installed.

r56's Ghosts USB launch handoff was verified on a Wii U. Disc and regional discovery also have host regression coverage. An interrupted game-content install must finish through HQ's repair flow before the game is launched.

## Source layout

- source and include contain the application and shared model.
- content contains fonts, branding, audio, animations, and other runtime assets.
- data contains embedded binary resources, including the HTTPS trust bundle.
- tests contains host tests and platform substitutes.
- tools contains packaging, publishing, preview, and asset preparation utilities.
- server-example contains example release feeds and publishing documentation.
- docs/implementation-history.md preserves earlier development notes.

## Distribution

HQ binaries are published at https://github.com/tonytrawl/bo2-unbound/releases. The app reads release manifests and news from https://downloads.unc-bo2.fun/. Default endpoints and the app revision are defined in include/config.hpp.

Game content packages use SHA-256 and size records in the manifest. Upload a versioned package before publishing its manifest entry. The installer accepts Core payloads under update/code and update/content and writes the installed version marker after the package completes.

## License

Application source uses the existing MIT license in LICENSE. Bundled fonts, icons, and other third-party assets retain their accompanying license and attribution files.
