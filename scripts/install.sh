#!/bin/bash
# One-time local installation. Subsequent launches use the native .app directly.
set -euo pipefail
project_dir="$(cd "$(dirname "$0")/.." && pwd)"
cd "$project_dir"
if [[ "$(uname -s)" != Darwin ]]; then
    echo "Установка Weather Desktop требует Mac." >&2
    exit 1
fi
if ! /usr/bin/xcrun --find clang++ >/dev/null 2>&1; then
    echo "Для первой сборки нужны инструменты Apple. Сейчас откроется их установка."
    /usr/bin/xcode-select --install || true
    echo "Дождитесь завершения установки Apple и снова откройте Install.command."
    exit 1
fi
bash scripts/build.sh
install_dir="$HOME/Applications"
target="$install_dir/Weather Desktop.app"
mkdir -p "$install_dir"
lock_dir="$install_dir/.WeatherDesktop.install-lock"
if ! mkdir "$lock_dir" 2>/dev/null; then
    echo "Другая установка уже выполняется. Если она прервалась, удалите пустую папку $lock_dir." >&2
    exit 1
fi
stage=""
backup=""
committed=0
cleanup() {
    result=$?
    trap - EXIT
    if [[ "$committed" == 0 && -n "$backup" && ! -e "$target" ]]; then
        mv "$backup/Weather Desktop.app" "$target" || true
    fi
    [[ -z "$stage" ]] || rm -rf -- "$stage"
    rmdir "$lock_dir" || true
    exit "$result"
}
trap cleanup EXIT
stage="$(mktemp -d "$install_dir/.WeatherDesktop-stage-XXXXXX")"
/usr/bin/ditto "build/Weather Desktop.app" "$stage/Weather Desktop.app"
/usr/bin/codesign --verify --strict "$stage/Weather Desktop.app"
if [[ -e "$target" || -L "$target" ]]; then
    if [[ -L "$target" || "$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$target/Contents/Info.plist" 2>/dev/null || true)" != local.windowsnow.desktop ]]; then
        echo "Место установки занято другим файлом: $target. Переместите его вручную." >&2
        exit 1
    fi
fi
# Quit only this app before replacing its executable; never terminate other apps.
if /usr/bin/pgrep -u "$(id -u)" -x 'WeatherDesktop|WindowSnow' >/dev/null; then
    /usr/bin/osascript -e 'tell application id "local.windowsnow.desktop" to quit' >/dev/null 2>&1 || true
    for attempt in {1..20}; do
        if ! /usr/bin/pgrep -u "$(id -u)" -x 'WeatherDesktop|WindowSnow' >/dev/null; then break; fi
        sleep .1
    done
    if /usr/bin/pgrep -u "$(id -u)" -x 'WeatherDesktop|WindowSnow' >/dev/null; then
        echo "Завершите Weather Desktop / WindowSnow и повторите установку." >&2
        exit 1
    fi
fi
if [[ -e "$target" ]]; then
    backup="$(mktemp -d "$project_dir/build/installed-previous-XXXXXX")"
    mv "$target" "$backup/Weather Desktop.app"
fi
mv "$stage/Weather Desktop.app" "$target"
committed=1
/usr/bin/open "$target"
echo ""
echo "Weather Desktop установлен: $target"
echo "Дальше открывайте приложение из Finder или закрепите его иконку в Dock."
echo "Terminal больше не нужен. После проверки можно удалить папку установщика."
if [[ -n "$backup" ]]; then echo "Предыдущая копия: $backup/Weather Desktop.app"; fi
