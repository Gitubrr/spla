# User Guide

Руководство по установке, проверке и настройке библиотеки spla.

## Содержание

1. [Установка](#установка)
2. [Проверка работоспособности](#проверка-работоспособности)
4. [Конфигурационный файл](#конфигурационный-файл)
5. [Настройка](#настройка)
6. [Профили](#профили)
7. [Наследование профилей (`extends`)](#наследование-профилей)

## Установка

Инструкции по установке см. в [README](https://github.com/SparseLinearAlgebra/spla#installation).

## Проверка работоспособности

После установки выполните команду, которая подтверждает, что библиотека готова к использованию.

```bash
spla --softcheck
```
Команда выполняет следующую последовательность действий:

1. Читает конфигурационный файл, предоставляющийся библиотекой.
2. Инициализирует OpenCL, выбирает платформу и устройство.
4. Запускает простое тестовое задание.
5. Сообщает статус.

Пример успешного вывода:
```bash
$ spla --softcheck
[spla:check] Starting sanity check...
[spla:check] Loading factory config: /usr/share/spla/spla_conf.json
[spla:check] Initializing OpenCL...
[spla:check] Selected platform: NVIDIA CUDA
[spla:check] Selected device: NVIDIA GeForce RTX 3080
[spla:check] Running test kernel: 2 + 3 = ...
[spla:check] Result: 5 (expected 5)
[spla:check] SUCCESS - spla works correctly
```

## Конфигурационный файл
Конфигурационный файл, предоставляющийся библиотекой, имеет следующую структуру:

```json
{
    "execution": {
        "target": "auto",
        "on_unavailable": "fallback",
        "platform": 0,
        "device": 0
    },

    "queues": 1,
    "profiling": false,
    "allocator": "general",
    "allocator_size": 0,
    "verbosity": 2,
    "profile": null,
    "profiles": {}
}
```
| Параметр | Тип | Значения в конфигурационном файле по умолчанию | Назначение | Допустимые значения |
| :--- | :--- | :--- | :--- | :--- |
| execution.target | string | "auto" | На каком устройстве работать | `gpu` - только GPU<br>`cpu` - только CPU<br>`auto` - GPU если доступен, иначе CPU |
| execution.on_unavailable | string | "fallback" | Что делать, если GPU недоступен | `fallback` - переключиться на CPU<br>`fail` - вернуть ошибку |
| execution.platform | int | 0 | Индекс OpenCL платформы | ≥ 0 |
| execution.device | int | 0 | Индекс GPU в платформе | ≥ 0 |
| queues | int | 1 | Число командных очередей | ≥ 1 |
| profiling | bool | false | Профилирование очередей | true / false |
| allocator | string | "general" | Тип аллокатора | general, linear |
| allocator_size | int | 0 | Размер линейного аллокатора (байт) | ≥ 0 (игнорируется для `general`) |
| verbosity | int | 2 | Уровень логирования | `0` - нет вывода<br>`1` - только ошибки<br>`2` - ошибки, предупреждения<br>`3` - ошибки, предупреждения, дополнительная информация |
| profile | string / null | null | Выбранный профиль | имя профиля из profiles или `null` |
| profiles | object | {} | Словарь профилей | { "name": { ... override ... } } |
| profiles.< name >.extends | array of string | [] | Список профилей-родителей | имена профилей из profiles |

## Настройка

Если параметры по умолчанию вас не устраивают, вы можете настроить библиотеку. Способы конфигурирования перечислены в порядке убывания приоритета:

1. Аргументы командной строки.
2. Переменные окружения.
3. Пользовательский конфигурационный файл.
4. Системный конфигурационный файл.
5. Конфигурационный файл, поставляющийся библиотекой.

### Расположение конфигурационных файлов

Конфигурационный файл с параметрами по умолчанию:
>Linux	`/usr/share/spla/spla_conf.json` \
macOS	`/usr/local/share/spla/spla_conf.json` \
Windows	`%ProgramData%\spla\spla_conf.json`

Системный конфигурационный файл:
>Linux:	`/etc/spla/spla_conf.json` \
macOS:	`/Library/Application Support/spla/spla_conf.json` \
Windows:	`%ProgramData%\spla\spla_conf.json`

Пользовательский конфигурационный файл:
>Linux:	`~/.config/spla/spla_conf.json` \
macOS:	`~/Library/Application Support/spla/spla_conf.json` \
Windows:	`%APPDATA%\spla\spla_conf.json`


## Профили

Профили позволяют хранить несколько наборов настроек в одном конфигурационном файле и переключаться между ними при запуске. Это удобно, когда одну и ту же программу нужно запускать в разных режимах - например, на разных GPU или с разным уровнем логирования.

- В конфигурационном файле описываются общие настройки, которые берутся в качестве базовых параметров.
- Описывается секция `profiles` - именованные наборы параметров.
- При запуске указывается имя профиля.
- Библиотека переопределяет базовые параметры параметрами из выбранного профиля.

### Пример
```json
{
    "execution": {
        "device": 0
    },
    "verbosity": 2,
    "queues": 1,

    "profiles": {
        "gpu0": { "execution": { "device": 0 } },
        "gpu1": { "execution": { "device": 1 } },
        "gpu2": { "execution": { "device": 2 } },
        "debug": { "verbosity": 3 },
        "bench": { "verbosity": 0 }
    }
}
```
### Запуск
```bash
SPLA_PROFILE=gpu1 ./program
SPLA_PROFILE=debug ./program
SPLA_PROFILE=bench ./program
```
Или через CLI:
```bash
./program --spla-profile=gpu1
```

### Итоговый конфигурационный файл
```json
{
    "execution": {
        "device": 1
    },

    "queues": 1,
    "verbosity": 2,
}
```

## Наследование профилей

Иногда профили пересекаются по смыслу - например, **debug_gpu1** должен включать и режим отладки, и выбор GPU 1. Чтобы не дублировать параметры, профиль может наследовать другие профили через поле `extends`.

- Если профиль содержит поле `extends`, библиотека сначала применяет указанные в нём профили по порядку.
- Затем к базовым параметрам применяются параметры каждого родительского профиля по очереди: последний в списке переопределяет предыдущие.
- После этого применяются параметры самого выбранного профиля, которые переопределяют всё, что было задано раньше.

### Пример

``` json
{
    "execution": { "device": 0 },
    "verbosity": 2,

    "profiles": {
        "gpu0": { "execution": { "device": 0 } },
        "gpu1": { "execution": { "device": 1 } },
        "gpu2": { "execution": { "device": 2 } },

        "debug": { "verbosity": 3, "profiling": true },
        "bench": { "verbosity": 0 },

        "debug_gpu1": {
            "extends": ["debug", "gpu1"]
        },
        "bench_gpu2": {
            "extends": ["bench", "gpu2"]
        }
    }
}
```
### Запуск
```bash
SPLA_PROFILE=debug_gpu1 ./program
SPLA_PROFILE=bench_gpu2 ./program
```
Или через CLI:
```bash
./program --spla-profile=debug_gpu1 
```

### Итоговый конфигурационный файл
```json
{
    "execution": { "device": 1 },
    "verbosity": 3,
    "profiling": true
}
```

---
---

# Распространенные сценарии использования библиотеки
  1. Быстрый запуск без настройки.
  4. Многократный запуск на разных устройствах с разными параметрами.

## Быстрый запуск без настройки

Если вы хотите получить работоспособное приложение без настройки - ничего настраивать не нужно. Библиотека использует параметры по умолчанию из [конфигурационного файла](#конфигурационный-файл).

Этот сценарий работает одинаково на Linux, macOS и Windows. Если GPU недоступен, библиотека автоматически переключится на CPU, сообщив об этом.

Перед первым запуском рекомендуется [выполнить](#проверка-работоспособности) `spla --softcheck`, чтобы убедиться, что всё необходимое для работы spla установлено.

## Многократный запуск на разных устройствах с разными параметрами

Вы регулярно запускаете одну и ту же программу на разных GPU и в разных режимах - например, на GPU 0 и 1, в режиме отладки и в режиме измерения производительности. Вместо того чтобы держать несколько конфигурационных файлов, вы описываете всё в одном.

Подробнее в главе про [профили](#профили) и [наследование профилей (`extends`)](#наследование-профилей).

```json
{
    "execution": { "device": 0 },
    "verbosity": 2,

    "profiles": {
        "gpu0":  { "execution": { "device": 0 } },
        "gpu1":  { "execution": { "device": 1 } },
        "gpu2":  { "execution": { "device": 2 } },

        "debug": { "verbosity": 3, "profiling": true },
        "bench": { "verbosity": 0 },

        "debug_gpu1": { "extends": ["debug", "gpu1"] },
        "bench_gpu2": { "extends": ["bench", "gpu2"] }
    }
}
```