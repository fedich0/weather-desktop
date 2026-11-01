#!/bin/bash
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
cd "$project_dir"
if [[ "$(uname -s)" != Darwin ]]; then
    echo "Сборка приложения требует macOS и Command Line Tools Apple." >&2
    exit 1
fi
if ! /usr/bin/xcrun --find clang++ >/dev/null 2>&1; then
    echo "Установи инструменты командой xcode-select --install, затем повтори сборку." >&2
    exit 1
fi
if [[ -L build ]]; then
    echo "Папка build является символической ссылкой. Сборка остановлена." >&2
    exit 1
fi
mkdir -p build
if ! mkdir build/.build-lock 2>/dev/null; then
    echo "Другая сборка уже запущена. Если она прервалась, удали пустую build/.build-lock." >&2
    exit 1
fi
stage=""
cleanup() {
    [[ -z "$stage" ]] || rm -rf -- "$stage"
    rmdir build/.build-lock
}
trap cleanup EXIT
stage="$(mktemp -d "$project_dir/build/.stage-XXXXXX")"
app="$stage/Weather Desktop.app"
mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources"
cp src/Art/*.png src/Art/*.jpg src/AppIcon.icns "$app/Contents/Resources/"
cp src/Info.plist "$app/Contents/"
cp LICENSE "$app/Contents/Resources/LICENSE.txt"
architectures=(-arch "$(uname -m)")
case "${1:-}" in
    --universal) architectures=(-arch arm64 -arch x86_64) ;;
    "") ;;
    *) echo "Использование: scripts/build.sh [--universal]" >&2; exit 1 ;;
esac
/usr/bin/plutil -lint "$app/Contents/Info.plist"
/usr/bin/xcrun --sdk macosx clang++ -std=c++17 -O2 -Wall -Wextra -Wno-unused-parameter \
    -fobjc-arc -mmacosx-version-min=11.0 "${architectures[@]}" \
    -framework Cocoa -framework QuartzCore -framework CoreGraphics \
    src/main.mm -o "$app/Contents/MacOS/WeatherDesktop"
# Ad-hoc signing permits local testing. Developer ID distribution is a separate release step.
/usr/bin/codesign --force --sign - --options runtime --timestamp=none "$app"
/usr/bin/codesign --verify --strict --verbose=2 "$app"
previous="build/Weather Desktop.app"
backup=""
if [[ -e "$previous" || -L "$previous" ]]; then
    if [[ -L "$previous" || "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$previous/Contents/Info.plist" 2>/dev/null || true)" != local.windowsnow.desktop ]]; then
        echo "build/Weather Desktop.app занят другим файлом. Перемести его перед сборкой." >&2
        exit 1
    fi
    backup="$(mktemp -d "$project_dir/build/previous-XXXXXX")"
    mv "$previous" "$backup/Weather Desktop.app"
fi
if ! mv "$app" "$previous"; then
    [[ -z "$backup" ]] || mv "$backup/Weather Desktop.app" "$previous"
    exit 1
fi
echo "Готово: $project_dir/$previous"
if [[ -n "$backup" ]]; then echo "Предыдущая сборка: $backup/Weather Desktop.app"; fi
