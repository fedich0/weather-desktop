#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
if [[ "$(uname -s)" != Darwin ]]; then echo "Эта проверка запускается на Mac." >&2; exit 1; fi
SANITIZE=1 bash scripts/test.sh
bash scripts/build.sh
smoke_dir="$(mktemp -d)"
trap 'rm -rf -- "$smoke_dir"' EXIT
/usr/bin/ditto "build/Weather Desktop.app" "$smoke_dir/WeatherDesktopSmoke.app"
/usr/bin/xcrun --sdk macosx clang++ -std=c++17 -O1 -g -fobjc-arc -mmacosx-version-min=11.0 \
    -framework Cocoa -framework QuartzCore -framework CoreGraphics \
    tests/AppSmoke.mm -o "$smoke_dir/WeatherDesktopSmoke.app/Contents/MacOS/WeatherDesktop"
# Use a separate preference domain for smoke tests.
/usr/libexec/PlistBuddy -c 'Set :CFBundleIdentifier local.windowsnow.smoketest' "$smoke_dir/WeatherDesktopSmoke.app/Contents/Info.plist"
/usr/bin/codesign --force --sign - "$smoke_dir/WeatherDesktopSmoke.app"
"$smoke_dir/WeatherDesktopSmoke.app/Contents/MacOS/WeatherDesktop"
