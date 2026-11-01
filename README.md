<p align="center">
  <img src="src/AppIcon.png" width="144" alt="Weather Desktop">
</p>
<h1 align="center">Weather Desktop</h1>
<p align="center"><strong>A little weather. A happier desktop.</strong></p>
<p align="center">Snow and autumn leaves for your Mac.</p>
<p align="center">
  <a href="#installation">Installation</a> ·
  <a href="#features">Features</a> ·
  <a href="https://github.com/fedich0/weather-desktop/issues">Report a bug</a>
</p>

Soft snow settles on your windows. Autumn leaves drift across your desktop. Move a window and watch them tumble down — then make the atmosphere your own with wind, speed and glass controls.

**Preview release · 0.9.0 RC** — launch and basic functionality have been reported working by the project owner on a MacBook Air M4. Broader compatibility testing is ongoing.

## Features

- Two seasons. Textured snow banks and rotating autumn leaves.
- Weather that follows your windows. Particles collect along window edges and fall when the windows move.
- Your own atmosphere. Adjust speed, density, wind and accumulation, including whether particles collect at the bottom of the screen.
- Quick presets. Quiet, Breeze and Storm, plus a saved favorite profile.
- Live preview. See changes inside the settings panel before returning to your desktop.
- Glass controls. Choose frosted or Liquid Glass styling and adjust the button tint.
- Stay in control. Click-through effects, a menu bar control, pause and a button to shake particles off the windows.
- Eco mode. A lower frame rate and less frequent window updates.

The interface is currently in Russian. Native Liquid Glass is used where supported; older systems use AppKit visual effects.

## Installation

### Build on your Mac

1. Download the project using Code → Download ZIP and extract it.
2. Open Install.command. If prompted, finish installing Apple's Command Line Tools, then open it again.
3. After a successful build, Weather Desktop opens from ~/Applications/Weather Desktop.app.

Keep its icon in the Dock for future launches. Once installed, the app runs without Terminal or the source folder.

### Download a test build

Available builds can be found under Actions → Build and tests. Open a successful run and download a local-test-* artifact. GitHub sign-in is required.

When a build is available, its DMG contains Weather Desktop.app and an Applications shortcut. Drag the app into Applications to install it.

### First launch on macOS

This preview build is not notarized by Apple. If macOS blocks it and you trust the build downloaded from this repository:

1. Try opening **Weather Desktop** once.
2. Open **System Settings → Privacy & Security**.
3. Find the message about Weather Desktop and click **Open Anyway** (in Russian: **«Всё равно открыть»**).
4. Authenticate if prompted, then confirm **Open**.

This allows this specific application to open; you do not need to disable Gatekeeper or system-wide protection. [Apple’s instructions](https://support.apple.com/102445).

## Compatibility

The deployment target is macOS 11 or later, with universal packaging configured for Apple Silicon and Intel. Compatibility with the minimum version, fullscreen apps, Spaces and different display layouts is still being tested.

## Privacy & performance

Preferences stay on your Mac. The app uses window geometry to position effects; the reviewed source contains no network client, analytics or document-reading functionality. See [Security](SECURITY.md) for details.

Eco mode runs at 20 fps instead of 30. Animation pauses when the display sleeps or the user session becomes inactive. Particle counts are capped; real-world CPU and battery measurements are still pending.

## Uninstall

Quit Weather Desktop and move its app to the Trash. To also remove preferences, run:
```bash
defaults delete local.windowsnow.desktop
```
The preferences identifier is retained from the app's earlier name. If present, remove ~/Library/Saved Application State/local.windowsnow.desktop.savedState. The app does not install background services or login items.
<details>
<summary><strong>For developers</strong></summary>

Built with Objective-C++, C++17 and Apple's system frameworks.
```bash
bash scripts/test.sh          # core tests
SANITIZE=1 bash scripts/test.sh
bash scripts/check-macos.sh   # macOS build and UI smoke checks
bash scripts/package.sh       # universal ZIP, DMG and checksums
```

See the [verification record](docs/VERIFICATION.md) and [release guide](docs/RELEASE.md) for test coverage and distribution details.
</details>

## License

[MIT](LICENSE) · FediCh
