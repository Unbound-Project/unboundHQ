# Unbound HQ

**Unbound HQ v1.5** is an Aroma Wii U app for **Call of Duty: Black Ops II** and **Call of Duty: Ghosts**. Switch games, install and update Unbound, browse Workshop content, manage mods and plugins, and check UNC activity from one app.

[Download the latest Wii U release](https://github.com/tonytrawl/bo2-unbound/releases/latest) · [UNC Online](https://unc-bo2.fun/) · [Package layout](docs/package-layout.txt)

This repository contains the Wii U source, build assets, host tests and development tools. The current application revision is **r66**.

## Install HQ

Download the SD ZIP from the latest release, extract its `wiiu` folder to the root of your SD card, merge folders, and launch **Unbound HQ** through Aroma.

The app belongs at `sd:/wiiu/apps/Unbound-HQ/Unbound-HQ.wuhb`. HQ migrates the earlier BO2 app name and preserves its settings and install records.

You need Aroma and a supported copy of the selected game with its official update installed:

| Game | Official update |
| --- | --- |
| Black Ops II — USA / Europe | v128 |
| Black Ops II — Japan | v96 or newer |
| Call of Duty: Ghosts | v112 |

HQ detects regional titles, language folders and game storage. Official Core and map-pack game files install to the selected game's **USB or internal NAND update**. An SD card can supply the ZIP instead of downloading it.

## What's in v1.5

- **One update confirmation:** installed Core and map-pack updates appear together with download sizes and estimated download/install times. Choose **Update all** or **Not now**. The initial install picker stays available for new installations and optional content.
- **Four Workshop tabs:** Plugins, Custom Maps, Mods and Custom Packages.
- **Local ZIP browsing:** choose a custom package from your SD folders, review its destinations and install it.
- **Support ZIP export:** save HQ diagnostics and Wii U crash logs together for bug reports.
- **Supply Run:** the optional installation minigame works during downloads and SD installs. Turn it off to see the centered progress bar and extra details.

Time estimates learn from successful downloads and installs. Initial estimates are labeled until HQ has measurements for your console.

## Workshop

| Tab | What it does |
| --- | --- |
| **Plugins** | Install, repair and manage Aroma plugins and their update preferences. |
| **Custom Maps** | Browse community maps from the selected game's custom-map catalog. |
| **Mods** | Install, update or reinstall mods from the selected game's mod catalog. |
| **Custom Packages** | Browse SD folders and install a ZIP you selected yourself. |

Custom Maps and Mods each have separate BO2 and Ghosts manifests, cached for offline browsing:

| Catalog | Black Ops II | Ghosts |
| --- | --- | --- |
| Custom Maps | [custom-maps.txt](https://downloads.unc-bo2.fun/custom-maps.txt) | [ghosts/custom-maps.txt](https://downloads.unc-bo2.fun/ghosts/custom-maps.txt) |
| Mods | [mods.txt](https://downloads.unc-bo2.fun/mods.txt) | [ghosts/mods.txt](https://downloads.unc-bo2.fun/ghosts/mods.txt) |

Entries use stable IDs, full ZIPs and optional patches. HQ verifies SHA-256 and exact size before installation, tracks installed versions and offers a full reinstall without downgrading newer content. Mods use `mod-` IDs; Custom Maps use `custom-` IDs.

## ZIP package layout

```text
release.zip
  update/
    code/...
    content/...
  sd/
    mods/example-mod/...
```

Either root can be omitted. Put `update` and `sd` directly inside the ZIP, without an extra enclosing folder.

| ZIP root | Destination |
| --- | --- |
| `update/` | The selected regional game's update on USB / NAND |
| `sd/` — Black Ops II | `sd:/unbound/content/` |
| `sd/` — Ghosts | `sd:/unbound-ghosts/content/` |

For example, `sd/mods/example/script.gsc` becomes `<game SD content>/mods/example/script.gsc`. Do not add another `content` folder beneath `sd`.

Custom Packages can browse from the SD root. Optional starting folders are `sd:/unbound/packages/` and `sd:/unbound-ghosts/packages/`. The original ZIP is retained. A local custom package does not claim an official release version; use **Install / Update** for official Core/map-pack packages and their version records.

ZIP **Store** and **Deflate** are supported. Limits are **4 GiB minus one byte** per archive, **2 GiB** per file and **5 GiB** expanded. Map packs keep their content-only console rules; Mods and Custom Packages can include mod-specific code. See [package-layout.txt](docs/package-layout.txt).

## Export a support report

Open **Support → Export ZIP** to save `sd:/wiiu/apps/Unbound-HQ/support-report.zip`.

The ZIP includes the current HQ diagnostics, available performance/install logs and native folder census, plus regular files copied from `/storage_slc/sys/logs/` on the Wii U. This includes the `.log` and `.dsc` records used to investigate DSI exceptions.

`export-summary.txt` lists included files and any unavailable or skipped logs. Original console logs stay in place. HQ also keeps the individual `diagnostics.txt` report. Export runs only when requested and saves locally for you to share. See [support-export.txt](docs/support-export.txt).

## Build

The Dockerfile specifies the devkitPro image version and pinned libmocha/librpxloader artifacts used by HQ.

```sh
git clone https://github.com/Unbound-Project/unboundHQ.git
cd unboundHQ
docker build -t unbound-hq-builder .
docker run --rm -v "${PWD}:/project" unbound-hq-builder make -j6
```

The build produces `Unbound-HQ.wuhb` at the repository root. Copy it to `wiiu/apps/Unbound-HQ/` on the SD card.

For a local toolchain, install devkitPro's `wiiu-dev` and `wiiu-portlibs`, libmocha and librpxloader, then run `make`. Python 3 prepares runtime content. The committed `content/` and `data/` directories provide the build assets.

## Verify

With the devkitPro build environment available, the host suites also require a native C++20 compiler, Python 3, pkg-config, SDL2/SDL2_image/SDL2_ttf, libzip and libcurl development libraries:

```sh
make test
make test-zip
make test-curl
python3 tests/install_flow_source_tests.py
python3 tests/sd_package_flow_tests.py
python3 tests/map_publisher_tests.py
```

The suites cover release planning, regional routing, ZIP integrity, interrupted installs, update selection, Workshop catalogs and support ZIPs. The support-export integration test verifies a 201-file crash-log fixture, unchanged source logs and retention of the previous ZIP after a failed export.

`tools/render_v1.cpp` renders the actual SDL UI with fixture data for layout and touch-control checks. Use a Wii U for final hardware testing.

## Publish content catalogs

HQ binaries stay on [GitHub Releases](https://github.com/tonytrawl/bo2-unbound/releases). Core/map-pack manifests, Workshop catalogs and news are hosted at `downloads.unc-bo2.fun`. Endpoints and revision are defined in [include/config.hpp](include/config.hpp).

[tools/publish_map_catalog.py](tools/publish_map_catalog.py) validates ZIP paths, CRCs, hashes and sizes and writes catalog entries. Default mode writes map-pack entries; `--custom` selects Custom Maps and `--mods` selects Mods. `--from-version` adds a patch to the current full release.

Templates: [Custom Maps](server-example/custom-maps.txt) · [Mods](server-example/mods.txt).

Upload the final versioned ZIP first, verify its public download, and publish the manifest last. Its size and SHA-256 must describe the complete ZIP users receive.

## Source layout

- `source/`, `include/` — application and shared model.
- `content/`, `data/` — runtime assets and embedded resources.
- `tests/` — host regression and integration tests.
- `tools/` — publishing, packaging, previews and asset preparation.
- `server-example/` — catalog templates and feed documentation.
- `docs/` — package/support-export details and implementation history.

## License

Application source is available under the [MIT license](LICENSE). Bundled fonts, icons and other third-party assets retain their accompanying licenses and attribution files.
