# Реализация алгоритма хеширования ГОСТ Р 34.11-2012 (Стрибог)
Ветка: `optimized`

## Обзор
Данная ветка содержит **высокооптимизированную реализацию** алгоритма хеширования ГОСТ Р 34.11-2012 ("Стрибог") с использованием современных SIMD-инструкций (AVX/AVX2). Код ориентирован на максимальную производительность при обработке больших объемов данных.

*Другие ветки*:
* `master` — эталонная реализация на чистом C без оптимизаций.
* `debug` — версия с подробным отладочным выводом всех преобразований.

## Ключевые оптимизации
### 1. Векторизация AVX/AVX2

Используются 256-битные регистры (YMM) для параллельной обработки 64 байт за две операции:
```c
/* Загрузка 512 бит (64 байт) из памяти */
#define LOAD(P, xmm0, xmm1) { \
    const __m256i *__m256p = (const __m256i *) P; \
    xmm0 = MEM_READ_I256(&__m256p[0]); \
    xmm1 = MEM_READ_I256(&__m256p[1]); \
}
```

### 2. Параллельные операции XOR

Побитовое исключающее ИЛИ выполняется над 256-битными блоками:
```c
#define X256R(xmm0, xmm1, xmm2, xmm3) { \
    xmm0 = _mm256_xor_si256(xmm0, xmm2); \
    xmm1 = _mm256_xor_si256(xmm1, xmm3); \
}
```

### 3. Векторизованное преобразование LPS

Линейное преобразование оптимизировано через предвычисленные таблицы:
```c
tmm1 = _mm256_set_epi64x(
    L_MATRIX_PRECALC_BYTES[i][block[8 * i + 7]],
    L_MATRIX_PRECALC_BYTES[i][block[8 * i + 6]],
    L_MATRIX_PRECALC_BYTES[i][block[8 * i + 5]],
    L_MATRIX_PRECALC_BYTES[i][block[8 * i + 4]]
);
```

## Структура проекта
```console
.
├── Makefile                    # Основной Makefile для сборки
├── src/                        # Исходный код
│   ├── hash/                   # Реализация алгоритма Стрибог
│   │   ├── stribog.c           # Основная реализация (портал, диспетчеризация)
│   │   ├── stribog.h           # Заголовочный файл
│   │   ├── stribog_avx.h       # AVX-оптимизированное ядро (новое!)
│   │   └── stribog_const.h     # Константы алгоритма (S-блоки, матрица A)
│   ├── cli/                    # Парсинг аргументов командной строки
│   │   ├── cli.c
│   │   └── cli.h
│   ├── io/                     # Функции ввода-вывода
│   │   ├── io.c
│   │   └── io.h
│   └── main.c                  # Точка входа
├── tests/                      # Тесты и измерения
│   ├── unit/                   # Модульные тесты
│   │   └── test_gost_examples.c
│   ├── benchmark/              # Тесты производительности
│   │   ├── src/                # Исходники бенчмарков
│   │   └── Makefile
│   └── comparison/             # Сравнение с другими реализациями
│       ├── src/adapters/       # Адаптеры для разных реализаций
│       ├── other_impl_1/       # Первая сторонняя реализация
│       ├── other_impl_2/       # Вторая сторонняя реализация
│       ├── other_impl_3/       # Третья сторонняя реализация
│       ├── test_data/          # Тестовые данные
│       └── Makefile
├── bin/                        # Исполняемые файлы (после сборки)
└── obj/                        # Объектные файлы (после сборки)
```

## Система сборки
Проект использует иерархическую систему Makefile:

### Основной Makefile (корневой)
```bash
# Сборка
make

# Запуск тестов на примерах из ГОСТ
make test

# Запуск сравнения с другими реализациями
make compare

# Запуск тестов производительности
make benchmark
```

### Makefile для тестов производительности (`tests/benchmark/`)
```bash
cd tests/benchmark

# Сборка и быстрый тест
make quick

# Полный тест производительности
make full

# Тест с привязкой к ядру
make pro

# Очистка
make clean
```

### Makefile для сравнения реализаций (`tests/comparison/`)
```bash
cd tests/comparison

# Сборка всех библиотек и утилит
make all

# Запуск сравнения
make run_compare

# Сборка генератора тестовых данных
make random_generator
```

## Быстрый старт

### Установка и сборка
```bash
# Клонирование репозитория
git clone https://github.com/Helsing02/stribog-hash.git
cd stribog-hash

# Переключение на ветку optimized
git checkout optimized

# Инициализация подмодулей
git submodule update --init --recursive

# Сборка проекта
make

# Проверка сборки
./bin/stribog --help
```

## Использование

### Справка по использованию
```bash
./bin/stribog -h
```
Выводит:

```console
Usage: bin/stribog [OPTIONS]
Calculate Stribog hash (GOST R 34.11-2012) for input data.

Options:
  -s, --size SIZE      Hash size (256 or 512, default: 512)
  -i, --input FILE     Input file (default: stdin)
  -x, --hex-input      Read input as hex string (default: raw bytes)
  -o, --output FILE    Output file (default: stdout)
  -X, --hex-output     Output hash in hex format (default: raw bytes)
  -h, --help           Display this help and exit

Examples:
  bin/stribog -s 256 -i file.txt -o -X hash.txt
  cat file.txt | bin/stribog -s 512 > hash.txt
  echo -n "hello" | bin/stribog
```

### Примеры использования
```bash
# Хеширование строки из аргумента (256-битный хеш)
echo -n "Пример сообщения" | ./bin/stribog -s 256

# Хеширование файла с выводом в hex
./bin/stribog -i README.md -X

# Хеширование hex-строки
echo -n "616263" | ./bin/stribog -x  # Хеш от "abc"

# Хеширование данных из stdin с сохранением в файл
cat data.bin | ./bin/stribog -o hash.bin

# Полный пример: 256-битный хеш файла с hex-выводом в файл
./bin/stribog -s 256 -i document.pdf -X -o hash.txt
```

## Форматы данных

### Входные форматы
* Raw bytes (по умолчанию) — бинарные данные
* Hex string (с флагом -x) — шестнадцатеричное представление

### Выходные форматы
* Raw bytes (по умолчанию) — бинарный хеш
* Hex string (с флагом -X) — шестнадцатеричное представление хеша

## Тестирование

### Модульные тесты
```bash
make test
```
Проверяет корректность реализации на официальных тестовых векторах из стандарта ГОСТ.

### Сравнение с другими реализациями
```bash
make compare
```
Сравнивает результаты работы данной реализации с тремя другими открытыми реализациями алгоритма.

### Тесты производительности
```bash
make benchmark
```
Запускает комплексное тестирование производительности с разными размерами данных (от 1KB до 1MB).
