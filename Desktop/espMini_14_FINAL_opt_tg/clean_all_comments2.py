import os
import re

def clean_js_css_content(code):
    """Удаляет комментарии из JS или CSS, сохраняя строки и ссылки"""
    cleaned = ""
    i = 0
    in_string = False
    string_char = ''
    escape_next = False
    in_template_literal = False  # Для поддержки шаблонных строк в JS

    while i < len(code):
        char = code[i]
        next_char = code[i+1] if i+1 < len(code) else ''

        if escape_next:
            cleaned += char
            escape_next = False
            i += 1
            continue

        if char == '\\' and not escape_next:
            escape_next = True
            cleaned += char
            i += 1
            continue

        # Обработка шаблонных строк в JS (обратные кавычки)
        if char == '`' and not in_string:
            in_template_literal = not in_template_literal
            cleaned += char
            i += 1
            continue

        if (char == '"' or char == "'") and not in_string and not in_template_literal:
            in_string = True
            string_char = char
            cleaned += char
            i += 1
            continue
        elif char == string_char and in_string and not escape_next:
            in_string = False
            cleaned += char
            i += 1
            continue

        # Удаляем /* */ (вне строк и шаблонных литералов)
        if not in_string and not in_template_literal and char == '/' and next_char == '*':
            j = i + 2
            while j < len(code):
                if code[j] == '*' and j+1 < len(code) and code[j+1] == '/':
                    i = j + 1
                    break
                j += 1
            else:
                # Многострочный комментарий до конца
                i = j
            i += 1  # пропускаем после */
            continue

        # Удаляем // (вне строк и шаблонных литералов)
        if not in_string and not in_template_literal and char == '/' and next_char == '/':
            # Пропускаем до конца строки
            while i < len(code) and code[i] != '\n':
                i += 1
            continue

        cleaned += char
        i += 1

    return cleaned

def final_formatting(content):
    """
    Применяет финальное форматирование:
    1. Удаляет пробелы в конце строк.
    2. Сокращает множественные пустые строки (3 и более переносов) до одной пустой строки (2 переноса).
    """
    lines = content.splitlines()
    cleaned_lines = [re.sub(r'[ \t]+$', '', line) for line in lines]
    content = '\n'.join(cleaned_lines)
    content = re.sub(r'\n{3,}', '\n\n', content)
    return content.strip() + '\n'

def clean_html_file(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            content = file.read()
    except Exception as e:
        print(f"❌ Пропускаем (чтение): {file_path} — {e}")
        return

    # --- Шаг 1: Защита специальных блоков от изменений ---
    placeholders = {}
    placeholder_pattern = "___PROTECTED_{}___"

    def save_protected(match):
        block_id = len(placeholders)
        placeholders[block_id] = match.group(0)
        return placeholder_pattern.format(block_id)

    # 1a. Сначала защищаем условные комментарии IE, т.к. они содержат '-->'
    conditional_comments_pattern = r'(<!--\[if[^>]*>[\s\S]*?<!\[endif\]-->)'
    content = re.sub(conditional_comments_pattern, save_protected, content, flags=re.IGNORECASE)

    # 1b. Затем защищаем содержимое тегов <script>, <style>, <pre> и т.д.
    # Это нужно сделать ПОСЛЕ защиты условных комментариев.
    protected_tags_pattern = r'(<(?:pre|textarea|code|script|style|noscript)[^>]*>[\s\S]*?</(?:pre|textarea|code|script|style|noscript)>)'
    content = re.sub(protected_tags_pattern, save_protected, content, flags=re.IGNORECASE)

    # --- Шаг 2: Удаление всех оставшихся HTML комментариев ---
    # Теперь, когда специальные блоки защищены, можно безопасно удалять обычные комментарии.
    content = re.sub(r'<!--.*?-->', '', content, flags=re.DOTALL)

    # --- Шаг 3: Восстановление защищенных блоков ---
    for bid, block in placeholders.items():
        content = content.replace(placeholder_pattern.format(bid), block)

    # --- Шаг 4: Очистка содержимого внутри восстановленных <script> и <style> ---
    def clean_tag_content(match):
        tag_open = match.group(1)
        inner_content = match.group(2)
        tag_close = match.group(3)

        if 'script' in tag_open.lower() or 'style' in tag_open.lower():
            inner_content = clean_js_css_content(inner_content)

        return f"{tag_open}{inner_content}{tag_close}"

    # Применяем очистку к <script> и <style> тегам
    content = re.sub(
        r'(<script[^>]*>)([\s\S]*?)(</script>)',
        clean_tag_content,
        content,
        flags=re.IGNORECASE | re.DOTALL
    )
    content = re.sub(
        r'(<style[^>]*>)([\s\S]*?)(</style>)',
        clean_tag_content,
        content,
        flags=re.IGNORECASE | re.DOTALL
    )

    # --- Шаг 5: Финальное форматирование ---
    content = final_formatting(content)

    # --- Шаг 6: Сохранение результата ---
    try:
        with open(file_path, 'w', encoding='utf-8') as file:
            file.write(content)
        print(f"🧹 HTML очищен: {file_path}")
    except Exception as e:
        print(f"❌ Ошибка (запись): {file_path} — {e}")

def clean_cpp_file(file_path):
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            content = file.read()
    except Exception as e:
        print(f"❌ Пропускаем (чтение): {file_path} — {e}")
        return

    cleaned = clean_js_css_content(content)
    cleaned_content = final_formatting(cleaned)

    try:
        with open(file_path, 'w', encoding='utf-8') as file:
            file.write(cleaned_content)
        print(f"🧹 C++ очищен: {file_path}")
    except Exception as e:
        print(f"❌ Ошибка (запись): {file_path} — {e}")

def clean_generic_text_file(file_path, ext):
    try:
        with open(file_path, 'r', encoding='utf-8') as file:
            content = file.read()
    except Exception as e:
        print(f"❌ Пропускаем (чтение): {file_path} — {e}")
        return

    if ext in ['.js', '.css']:
        cleaned = clean_js_css_content(content)
    else:
        cleaned = content

    cleaned_content = final_formatting(cleaned)

    try:
        with open(file_path, 'w', encoding='utf-8') as file:
            file.write(cleaned_content)
        print(f"🧹 {ext.upper().strip('.')} очищен: {file_path}")
    except Exception as e:
        print(f"❌ Ошибка (запись): {file_path} — {e}")

def process_folder(folder_path):
    for root, dirs, files in os.walk(folder_path):
        for filename in files:
            file_path = os.path.join(root, filename)
            ext = os.path.splitext(filename.lower())[1]

            if ext == '.html':
                clean_html_file(file_path)
            elif ext in ('.h', '.hpp', '.cpp', '.cc'):
                clean_cpp_file(file_path)
            elif ext in ('.js', '.css'):
                clean_generic_text_file(file_path, ext)

# === УКАЖИТЕ ПАПКУ ===
target_folder = os.getcwd()

if __name__ == "__main__":
    if os.path.exists(target_folder):
        process_folder(target_folder)
        print("✅ Очистка завершена: комментарии удалены, пустые строки сокращены до одной.")
    else:
        print(f"❌ Папка не найдена: {target_folder}")
