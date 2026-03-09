# Flipper Zero Honda Firmware – C++ Edition

This firmware enables your Flipper Zero to capture and replay RF signals for certain Honda vehicles.
It demonstrates *CVE-2022-27254* – rolling-code-less RF key fob replay for remotes operating at
roughly 433 MHz, specifically the key fob with **FCC ID : KR5V2X**.

</br>

To view a demonstration of this, please watch "Security Like The '80s : How I Stole Your RF" @
[CarHackingVillage DEFCON30 Talks](https://www.carhackingvillage.com/talks)!

</br>

This firmware was originally built and designed by SkorP, the Sub-GHz architect for Flipper Zero.
This is for **educational purposes ONLY**.

---

## What's new in v2.0 (C++ Rewrite)

This release is a **full rewrite** of the Honda key fob SubGHz application from C to **C++17**,
aligning with the modern Flipper Zero SDK:

- **New `honda_keyfob_app/`** – standalone C++ Flipper Application Package (FAP) with a polished
  Lock / Unlock / About menu UI.
- **`FuriString` API** (`furi/core/string.h`) – the modern heap-allocated string type from the
  updated Flipper Zero SDK replaces the legacy `string_t` from M\*LIB. Inline C++ overloads for
  `flipper_format`, `path`, and `storage` functions allow gradual migration without breaking C code.
- **SubGHz application converted to C++17** – all `.c` sources in
  `applications/subghz/` (scenes, views, helpers, history, settings) have been renamed to `.cpp`
  and updated to use `FuriString*` and `std::vector<>` instead of M\*LIB containers.
- **`-std=c++17` build flag** already present in `site_scons/cc.scons` – no toolchain changes
  required.

---

## Standalone Honda Key Fob FAP (`honda_keyfob_app/`)

The new standalone application is the recommended way to use this project. It runs on top of the
**stock Flipper Zero firmware** – no custom firmware flashing required.

### Prerequisites

1. A Flipper Zero with standard firmware (0.92+ recommended).
2. SD card with the signal files copied to the correct location (see below).

### SD card setup

Copy the capture files to the SD card:

```
SD card/
└── subghz/
    └── honda/
        ├── Lock_honda.sub
        └── Unlock_honda.sub
```

### How to build and install

```bash
# From the firmware root
./fbt fap_honda_keyfob
# Flash via qFlipper or copy honda_keyfob.fap to SD card /apps/Sub-GHz/
```

---

## Legacy: Custom Firmware (original method)

The original approach required flashing a custom `.dfu` firmware with the Honda CC1101 presets
baked in. This is no longer necessary – use the standalone FAP above instead.

<details><summary>Flipper Zero custom CC1101 preset for Honda key fobs (FCC ID KR5V2X)</summary>

```
# Custom preset – Honda1 (primary, 433.657 MHz)
Custom_preset_name: Honda1
Custom_preset_module: CC1101
Custom_preset_data: 02 0D 0B 06 08 32 07 04 14 00 13 02 12 04 11 36 10 69 15 32 18 18 19 16 1D 91 1C 00 1B 07 20 FB 22 10 21 56 00 00 C0 00 00 00 00 00 00 00

# Custom preset – Honda2 (more sensitive / noisier, 434.177 MHz)
Custom_preset_name: Honda2
Custom_preset_module: CC1101
Custom_preset_data: 02 0D 0B 06 08 32 07 04 14 00 13 02 12 07 11 36 10 E9 15 32 18 18 19 16 1D 92 1C 40 1B 03 20 FB 22 10 21 56 00 00 C0 00 00 00 00 00 00 00
```

Place this file at `SD:/subghz/assets/setting_user` on the Flipper Zero SD card.

</details>

## Legacy: Manual RAW capture/replay

- SubGHz → Read RAW
- Modulation: **Honda1**
- Frequency: **433.65 MHz** (preferred) or **434.17 MHz**

> **Note:** Honda2 modulation is more sensitive, hence more noisy.

---

## Demonstration

https://user-images.githubusercontent.com/5160055/183486723-ce0aae23-5c37-4587-8930-6ef2ab17c6dc.mp4
