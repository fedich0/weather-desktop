#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."
bash scripts/build.sh --universal
if [[ -L dist ]]; then echo "dist должен быть обычной папкой." >&2; exit 1; fi
mkdir -p dist
version="$(/usr/libexec/PlistBuddy -c 'Print :CFBundleShortVersionString' 'build/Weather Desktop.app/Contents/Info.plist')"
archive="dist/WeatherDesktop-$version-local-test.zip"
/usr/bin/ditto -c -k --sequesterRsrc --keepParent "build/Weather Desktop.app" "$archive"
(cd dist && /usr/bin/shasum -a 256 "$(basename "$archive")") > "$archive.sha256"
echo "$archive"
# A disk image for drag-and-drop installation; requires a successful native build.
dmg_stage="$(mktemp -d)"
trap 'rm -rf -- "$dmg_stage"' EXIT
/usr/bin/ditto "build/Weather Desktop.app" "$dmg_stage/Weather Desktop.app"
ln -s /Applications "$dmg_stage/Applications"
/usr/bin/hdiutil create -volname 'Weather Desktop' -srcfolder "$dmg_stage" \
    -ov -format UDZO "dist/WeatherDesktop-$version-local-test.dmg"
(cd dist && /usr/bin/shasum -a 256 "WeatherDesktop-$version-local-test.dmg") > "dist/WeatherDesktop-$version-local-test.dmg.sha256"
echo "Сборки подписаны локально. Публичный релиз: docs/RELEASE.md."
