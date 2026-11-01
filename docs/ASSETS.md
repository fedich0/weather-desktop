# Графические ресурсы

Иконка `src/AppIcon.png` создана встроенным image_gen для Weather Desktop. `src/AppIcon.icns` содержит размеры для macOS, полученные конвертацией этого изображения. Иконка включена в сборочный скрипт и указана в Info.plist.

Промпт обновления встроенным image_gen: «Edit the Weather Desktop icon into a joyful, warm premium icon. Preserve the centered rounded square, transparent outer margins and small-size readability. Sky-blue to periwinkle and peach sunrise gradient, warm golden apricot sun behind a creamy white cloud, short rounded rays and two pale blue snow dots. Subtle smile, refined minimal soft dimensional forms, no text, no scenery, no mockup.»

Остальные ресурсы `src/Art` подготовлены процедурно для проекта. Системные шрифты и Apple frameworks не включены в архив, используются на Mac через системные API.
