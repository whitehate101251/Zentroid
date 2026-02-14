# Zentroid

> **Zentroid — Reimagining Android screen control — inspired by QtScrcpy and Scrcpy**


![license](https://img.shields.io/badge/license-Apache2.0-blue.svg)

Zentroid is a high-performance Android screen mirroring and control application built with C++, Qt, OpenGL, and FFmpeg. Connect your Android device via USB or WiFi and control it from your desktop — no root required.

Supports **GNU/Linux**, **Windows**, and **macOS**.

---

## ✨ Features at a Glance

### 🖥️ Screen Mirroring

- **Real-time display** of your Android device screen
- **Up to 120 FPS** (or any custom frame rate) — not locked to 60 FPS
- **Auto-detect monitor refresh rate** — automatically matches your display's native Hz (90, 120, 144, 165, etc.)
- **High resolution** — configurable up to native device resolution (640 / 720 / 1080 / 1280 / 1920 or original)
- **Ultra-low latency** — ~35–70 ms over USB
- **Instant startup** — first frame in about 1 second
- **Non-intrusive** — nothing is installed on your Android device
- **VSync disabled** rendering pipeline for buttery-smooth output
- **FPS overlay** — real-time FPS counter displayed on the video window

### 🎮 Visual Keymap Editor (Drag & Drop)

A built-in **Bluestacks-style visual keymap editor** — no manual JSON editing required:

- **Drag-and-drop** node placement on a live device screenshot canvas
- **6 node types**:
  - 🔵 **Click** — single touch
  - 🔵 **Click Twice** — double tap
  - 🔵 **Click Multi** — multi-touch sequences
  - 🔵 **Drag** — swipe / drag gestures
  - 🔵 **WASD Steer Wheel** — virtual joystick for movement
  - 🔵 **Gesture** — multi-finger gestures (pinch, swipe, rotate)
- **Properties panel** — live editing of position, key binding, comment, and all type-specific parameters
- **Key capture dialog** — press a key on your keyboard **or any mouse button** to assign it
- **Extra mouse button mapping** — map your mouse's side buttons (Mouse4 / Mouse5 / Back / Forward) to any in-game action directly from the editor — perfect for gaming mice macros
- **F1–F11 function keys** — fully mappable as keymap triggers and forwarded as native Android keycodes (F12 is reserved for overlay toggle)
- **Undo / Redo** — full history stack
- **Snap-to-grid** with configurable grid size
- **Alignment tools** — left, right, top, bottom, center-H, center-V, distribute-H, distribute-V
- **Copy / Paste / Duplicate** nodes
- **Rubber-band multi-select** on canvas
- **Zoom in / out / reset** and fit-to-view
- **Right-click context menu** on nodes
- **Global settings editing** — `mouseMoveMap` and `switchKey` directly from the editor

### 🗂️ Profile & Template System

- **Profile management** — load, save, and switch between keymap files via dropdown
- **Templates** — prebuilt layouts for common game genres (FPS, MOBA, etc.)
- **Add New Keymap** — create a new keymap directly from the dropdown
- **Auto-restore** — remembers your last-selected keymap on refresh

### 🎬 Macro Recording

- **Record click sequences** with precise timing
- **Relative position** recording — macros adapt to different resolutions
- **Delay support** between each recorded action

### 🖐️ Gesture Support

- **8 preset multi-finger gestures**:
  - Pinch In / Pinch Out
  - Two-Finger Swipe (Up / Down / Left / Right)
  - Rotate
  - Custom gesture paths
- **Configurable duration** and finger count

### 📐 Multi-Layer System

- **Named layers** with color coding
- **Visibility toggle** — show/hide layers
- **Active layer selection** — organize complex keymaps into logical groups

### 🔲 Real-Time Transparent Overlay

A transparent overlay that renders keymap nodes directly on top of the live video stream:

- **Play mode** — semi-transparent key indicators visible while you play
- **Edit mode** — drag nodes, add/delete via right-click, save with `Ctrl+S`
- **F12** to toggle between Play and Edit mode
- **Profile dropdown** — switch keymaps on-the-fly from the overlay toolbar
- **Hot-reload** — file watcher auto-reloads when keymap files change externally
- **Save confirmation** — green toast notification on successful save
- **Auto device sync** — saving from the overlay immediately reloads the keymap on the device
- **Switch-key hint** displayed in play mode

### 🖱️ Extra Mouse Button & Macro Support

- **Mouse side-button mapping** — assign Mouse4 (Back), Mouse5 (Forward), and any extra mouse button to in-game actions
- **Click directly in the key-capture dialog** with your mouse side button to bind it — no JSON editing needed
- **Friendly labels** — side buttons display as "Mouse4 (Back)" / "Mouse5 (Forward)" in the editor
- **Works with any gaming mouse** — Logitech, Razer, SteelSeries, etc. — any button that sends XButton1/XButton2 events
- **Combine with macros** — bind a mouse side button to a multi-click sequence, drag, or gesture for one-button combos

### 🎛️ Device Control

- **Full keyboard & mouse control** of your Android device
- **USB and WiFi connections** — both supported
- **Multiple simultaneous devices** — connect and control several phones at once
- **Group control** — send inputs to all connected devices simultaneously
- **Screenshot** — save to PNG
- **Screen recording** — MP4 or MKV format with configurable bit rate
- **Drag & drop APK install** — drop an APK onto the video window to install
- **Drag & drop file transfer** — send files directly to `/sdcard/`
- **Clipboard sync** — bidirectional clipboard between PC and device:
  - `Ctrl+C` — copy device clipboard to PC
  - `Ctrl+Shift+V` — paste PC clipboard to device
  - `Ctrl+V` — inject PC clipboard as text events
- **Audio forwarding** — sync device speaker to PC (Android 10+, via [sndcpy](https://github.com/rom1v/sndcpy))

### 🪟 Window & Display

- **Always on top** mode
- **Frameless window** mode
- **Full-screen** mode
- **Screen-off mirroring** — turn off the Android screen to save power while mirroring
- **Stay awake** — keep device awake while connected
- **Lock orientation** — 0° / 90° / 180° / 270°
- **Pixel-perfect 1:1** window resize
- **Dark theme** UI
- **System tray** minimization

### 🌍 Other

- **Simple Mode / Advanced Mode** toggle
- **IP history** — dropdown with up to 10 saved IPs and clear-history option
- **Port history** — dropdown with saved ports
- **Device nicknames** — assign friendly names to each device serial
- **Internationalization** — English, Chinese, Japanese
- **Custom codec** name and codec options support
- **Linux AppImage** support with environment variable fallback for paths

---

## ⌨️ Keyboard Shortcuts

| Action                                 | Shortcut (Windows)            | Shortcut (macOS)              |
|----------------------------------------|-------------------------------|-------------------------------|
| Switch fullscreen mode                 | `Ctrl+F`                      | `Cmd+F`                       |
| Resize window to 1:1 (pixel-perfect)   | `Ctrl+G`                      | `Cmd+G`                       |
| Resize window to remove black borders  | `Ctrl+W` / Double-click       | `Cmd+W` / Double-click        |
| HOME                                   | `Ctrl+H` / Middle-click       | `Ctrl+H` / Middle-click       |
| BACK                                   | `Ctrl+B` / Right-click        | `Cmd+B` / Right-click         |
| APP_SWITCH                             | `Ctrl+S`                      | `Cmd+S`                       |
| MENU                                   | `Ctrl+M`                      | `Ctrl+M`                      |
| VOLUME_UP                              | `Ctrl+Up`                     | `Cmd+Up`                      |
| VOLUME_DOWN                            | `Ctrl+Down`                   | `Cmd+Down`                    |
| POWER                                  | `Ctrl+P`                      | `Cmd+P`                       |
| Turn screen off (keep mirroring)       | `Ctrl+O`                      | `Cmd+O`                       |
| Expand notification panel              | `Ctrl+N`                      | `Cmd+N`                       |
| Collapse notification panel            | `Ctrl+Shift+N`                | `Cmd+Shift+N`                 |
| Copy device clipboard                  | `Ctrl+C`                      | `Cmd+C`                       |
| Cut device clipboard                   | `Ctrl+X`                      | `Cmd+X`                       |
| Paste computer clipboard               | `Ctrl+V`                      | `Cmd+V`                       |
| Inject clipboard text                  | `Ctrl+Shift+V`                | `Cmd+Shift+V`                 |
| Toggle keymap overlay (Play ↔ Edit)    | `F12`                         | `F12`                         |

---

## 📋 Requirements

- Android API ≥ 21 (Android 5.0)
- [ADB debugging](https://developer.android.com/studio/command-line/adb.html#Enabling) enabled on your device
- 
---

## 📦 Downloads

Prebuilt binaries are published on the GitHub Releases page.

- **Linux (AppImage)** — download the latest `Zentroid-x86_64.AppImage` from the [Releases](https://github.com/whitehate101251/Zentroid/releases/latest) page, then:
   ```bash
   chmod +x Zentroid-x86_64.AppImage
   ./Zentroid-x86_64.AppImage
   ```
- **Windows / macOS** — builds will be provided as installers/archives in future releases. Until then, you can build from source using the platform-specific instructions below.

## � How to Use

### Linux

1. **Build** (or download a release):
   ```bash
   git clone --recurse-submodules https://github.com/whitehate101251/Zentroid.git
   cd Zentroid
   ./ci/linux/build_for_linux.sh Release
   ```

2. **Run:**
   ```bash
   cd output/x64/Release
   ./Zentroid
   ```

3. **Connect your device:**
   - **USB:** Plug in your Android phone with USB debugging enabled → it appears automatically
   - **WiFi:** Enter the device IP and port (default `5555`) → click **Connect**

4. **Start mirroring:** Select your device from the list → click **Start**

5. **Use keymaps:**
   - Select a keymap from the dropdown (e.g. BGMI, FRAG) → click **Apply**
   - Press <kbd>~</kbd> to toggle game keymap mode
   - Press <kbd>F12</kbd> to open the overlay keymap editor

> **Tip:** To create an AppImage (portable, runs on any distro):
> ```bash
> ./ci/linux/package_appimage.sh Release
> chmod +x output/appimage/Zentroid-*.AppImage
> ./output/appimage/Zentroid-*.AppImage
> ```

### Windows

1. **Install prerequisites:**
   - [Qt 5.12.5+](https://www.qt.io/download) with MSVC 2019/2022 components
   - [Visual Studio 2022](https://visualstudio.microsoft.com/) (Community is free)
   - [CMake 3.16+](https://cmake.org/download/)

2. **Build:**
   ```cmd
   git clone --recurse-submodules https://github.com/whitehate101251/Zentroid.git
   cd Zentroid

   REM Set your Qt path
   set ENV_QT_PATH=C:\Qt\5.15.2

   REM Open "x64 Native Tools Command Prompt for VS 2022", then:
   ci\win\build_for_win.bat Release x64
   ```

3. **Run:**
   ```cmd
   cd output\x64\Release
   Zentroid.exe
   ```
   Or **simply double-click `Zentroid.exe` in File Explorer.**

4. **Connect your device:**
   - **USB:** Plug in your phone with USB debugging enabled
   - **WiFi:** Enter device IP → click **Connect**

5. **Start mirroring:** Select device → click **Start**

### macOS

1. **Install prerequisites:**
   ```bash
   brew install qt@5 cmake
   ```

2. **Build:**
   ```bash
   git clone --recurse-submodules https://github.com/whitehate101251/Zentroid.git
   cd Zentroid
   ./ci/mac/build_for_mac.sh
   ```

3. **Run:**
   ```bash
   cd output/x64/Release
   open Zentroid.app
   # Or from terminal:
   ./Zentroid.app/Contents/MacOS/Zentroid
   ```

4. **Connect & mirror:** Same as Linux — USB or WiFi, select device, click **Start**.

### Quick Start (All Platforms)

| Step | Action |
|------|--------|
| 1 | Enable **USB debugging** on your Android device (Settings → Developer Options) |
| 2 | Connect via **USB** or enter **IP:Port** for WiFi |
| 3 | Click **Start** to begin mirroring |
| 4 | Select a **keymap** from the dropdown and click **Apply** |
| 5 | Press <kbd>~</kbd> to toggle keymap mode on/off |
| 6 | Press <kbd>F12</kbd> to open the visual keymap overlay editor |
| 7 | Use your **mouse side buttons** (Mouse4/Mouse5) in the key capture dialog for extra bindings |

---

## �🔧 Build

### Prerequisites

1. **Qt development environment** (Qt 6.x recommended, Qt 5.12+ supported). Use the [official Qt installer](https://www.qt.io/download) or [aqt](https://github.com/miurahr/aqtinstall).
2. **CMake 3.16+**
3. **C++17 compatible compiler**

### Steps

```bash
git clone --recurse-submodules https://github.com/whitehate101251/Zentroid.git
cd Zentroid

# Linux
./ci/linux/build_for_linux.sh "Release"

# Windows — open CMakeLists.txt in Qt Creator, build Release

# macOS
./ci/mac/build_for_mac.sh
```

Compiled artifacts are located at `output/x64/Release`.

### Building scrcpy-server

1. Set up an Android development environment
2. Open the `server` project in Android Studio
3. Build the APK, rename it to `scrcpy-server`, and place it in `Zentroid/ZentroidCore/src/third_party/`

---

## 🔑 Key Mapping Scripts

Custom key mapping scripts are supported. See [Key Mapping Documentation](docs/KeyMapDes.md) for script writing rules.

Default scripts for several games are included in the `keymap/` directory.

**Usage:**
1. Write a script (or use the visual editor) and save it in the `keymap` directory
2. Click **Refresh Script** in the main UI
3. Select your script and click **Apply**
4. Press <kbd>~</kbd> (the `switchKey`) to toggle custom mapping mode

---

## 🙏 Acknowledgements & Credits

Zentroid is a fork of [**QtScrcpy**](https://github.com/barry-ran/QtScrcpy) by [**Rankun (barry-ran)**](https://github.com/barry-ran).

QtScrcpy is itself based on [**scrcpy**](https://github.com/Genymobile/scrcpy) by [**Genymobile**](https://github.com/Genymobile).

Massive thanks to both projects and their contributors — without their foundational work, Zentroid would not exist.

| | scrcpy | QtScrcpy | Zentroid |
|---|:---:|:---:|:---:|
| UI | SDL | Qt | Qt |
| Video decode | FFmpeg | FFmpeg | FFmpeg |
| Video render | SDL | OpenGL | OpenGL |
| Cross-platform | self-implemented | Qt | Qt |
| Language | C | C++ | C++ |
| Style | sync | async | async |
| Custom keymap | ❌ | ✅ JSON scripts | ✅ Visual drag-and-drop editor |
| FPS | default | 30–60 | Up to 120+ / custom / auto-detect |
| Keymap overlay | ❌ | ❌ | ✅ Real-time transparent overlay |
| Gesture support | ❌ | ❌ | ✅ 8 preset multi-finger gestures |
| Macro recording | ❌ | ❌ | ✅ Timed click sequences |
| Mouse side-button mapping | ❌ | ❌ | ✅ Mouse4/Mouse5 via GUI editor |
| Multi-layer keymaps | ❌ | ❌ | ✅ Named layers with visibility |
| Build system | Meson + Gradle | qmake or CMake | CMake |

---

## 📜 License

This project is licensed under the **Apache License 2.0**.

```
Copyright (C) 2025 Shreyansh Chauhan

Based on Zentroid, Copyright (C) 2025 Rankun
Based on scrcpy, Copyright (C) Genymobile

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
```

See [LICENSE](LICENSE) for the full license text.

---

## 👤 Author

**Shreyansh Chauhan** — [github.com/whitehate101251](https://github.com/whitehate101251)

---

### Special Thanks

Special shout-out to [**Rankun (barry-ran)**](https://github.com/barry-ran), original creator of [**QtScrcpy**](https://github.com/barry-ran/QtScrcpy), whose work inspired this project and made Zentroid possible.

---
