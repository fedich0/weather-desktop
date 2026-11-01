#!/bin/bash
cd "$(dirname "$0")" || exit 1
if /bin/bash scripts/install.sh; then
    echo "Установка завершена. Это окно можно закрыть."
else
    echo "Установка не завершена. Скопируйте сообщение об ошибке выше."
    if [[ -t 0 ]]; then read -r -p "Нажмите Enter, чтобы закрыть окно…" unused; fi
    exit 1
fi
