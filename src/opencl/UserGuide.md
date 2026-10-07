# User Guide

Руководство по установке, проверке и настройке библиотеки spla.

## Содержание

1. [Установка](#установка)
2. [Проверка работоспособности](#проверка-работоспособности)
3. [Конфигурационный файл](#конфигурационный-файл)
4. [Настройка](#настройка)
5. [Конфигурации](#конфигурации)
6. [Наследование конфигураций (`extends`)](#наследование-конфигураций)

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
[spla:check] Initializing OpenCL...
[spla:check] Selected platform: NVIDIA CUDA
[spla:check] Selected device: NVIDIA GeForce RTX 3080
[spla:check] Running test kernel: 2 + 3 = ...
[spla:check] Result: 5 (expected 5)
[spla:check] SUCCESS - spla works correctly
```

## Конфигурационный файл
Конфигурационные файлы состоят из набора конфигураций, каждая конфигурация - это именованный набор параметров библиотеки, все параметры библиотеки описываются внутри конфигураций.

### Параметры конфигурации
| Параметр | Тип | Описание | Допустимые значения |
| :--- | :--- | :--- | :--- |
| extends | array of string | Список конфигураций-родителей | имена конфигураций (со всех уровней) |
| backend | string | На каком устройстве работать | `"gpu"` - только GPU<br>`"cpu"` - только CPU<br>`"any"` - любое доступное устройство<br>`"by_index"` - выбрать по индексам ниже |
| platform_index | int | Индекс OpenCL платформы<br> Используются только при backend: `"by_index"`<br> При других значениях backend - игнорируются | ≥ 0 |
| device_index | int | Индекс устройства внутри платформы<br> Используются только при backend: `"by_index"`<br> При других значениях backend - игнорируются | ≥ 0 |
| if_gpu_unavailable | string | Что делать, если GPU недоступен<br> Не применяется при backend: `"cpu"` | `"use_cpu"` - переключиться на CPU<br> `"abort"` - вернуть ошибку<br> |
| queues_count | int | Число командных очередей | ≥ 1 |
| profiling | bool | Профилирование очередей | true, false |
| allocator_type | string | Тип аллокатора | `"general"`, `"linear"` |
| linear_alloc_size | int | Размер линейного аллокатора (байт)<br> Используются только при allocator_type: `"linear"` | ≥ 0 |
| verbosity | int | Уровень логирования | `0` - нет вывода,<br>`1` - только ошибки,<br>`2` - ошибки, предупреждения,<br>`3` - ошибки, предупреждения, дополнительная информация |

Файл конфигурации, предоставляющийся библиотекой, имеет следующую структуру:
```json
{
    "default": {
        "backend": "gpu",
        "if_gpu_unavailable": "use_cpu",

        "queues_count": 1,
        "profiling": false,
        "allocator_type": "general",

        "verbosity": 2
    }
}
```
## Настройка

Если параметры по умолчанию вас не устраивают, вы можете настроить библиотеку. Способы конфигурирования перечислены в порядке убывания приоритета:

1. Аргументы командной строки.
2. Переменные окружения.
3. Файл, путь к которому пользователь может задать явно.
- > При необходимости пользователь может явно указать файл с конфигурациями, который не находится по пути пользовательского и системного конфигурационного файла.
4. Пользовательский конфигурационный файл.
5. Системный конфигурационный файл.
6. Конфигурационный файл, поставляющийся библиотекой.

### Расположение конфигурационных файлов

**Конфигурационный файл с параметрами по умолчанию:**

>Файл конфигурации с параметрами по умолчанию поставляется вместе с пакетом pyspla и располагается внутри директории установленного Python-пакета. 
Расположение зависит от способа установки пакета (глобально, --user, в виртуальном окружении).

**Системный конфигурационный файл:**
>Linux:	`/etc/spla/spla_conf.json` \
macOS:	`/Library/Application Support/spla/spla_conf.json` \
Windows:	`%ProgramData%\spla\spla_conf.json`

**Пользовательский конфигурационный файл:**
>Linux:	`~/.config/spla/spla_conf.json` \
macOS:	`~/Library/Application Support/spla/spla_conf.json` \
Windows:	`%APPDATA%\spla\spla_conf.json`

**Пользовательский конфигурационный файл, задавающийся явно:**

>На усмотрение пользователя.


## Конфигурации

Конфигурации позволяют хранить несколько наборов настроек в одном конфигурационном файле и переключаться между ними при запуске. Это удобно, когда одну и ту же программу нужно запускать в разных режимах.

Если пользователь не укажет конфигурацию, то будет взята конфигурация `"default"` из файла, который идет с библиотекой.

### Пример конфигурационного файла
```json
{
    "cpu":  { 
        "backend": "cpu", 
        "queues_count": 1,
        "profiling": false,
        "allocator_type": "general",
        "verbosity": 2
        },

    "gpu0":  { 
        "backend": "gpu", 
        "if_gpu_unavailable": "use_cpu",
        "queues_count": 2,
        "profiling": true,
        "allocator_type": "linear",
        "linear_alloc_size": 8,
        "verbosity": 0
        },

    "gpu1":  { 
        "backend": "by_index", 
        "platform_index": 0, 
        "device_index": 2,
        "if_gpu_unavailable": "abort",
        "queues_count": 1,
        "profiling": false,
        "allocator_type": "general",
        "verbosity": 3
        }
}
```
### Запуск
```bash
SPLA_CONFIG=cpu ./program
SPLA_CONFIG=gpu0 ./program
SPLA_CONFIG=gpu1 ./program
```
Или через CLI:
```bash
./program --spla-config=gpu1
```
## Наследование конфигураций

Иногда конфигурации пересекаются по смыслу - например, `debug_gpu1` должна включать и `режим отладки`, и `выбор GPU 1`. Чтобы не дублировать параметры, конфигурация может наследовать другие конфигурации через поле `extends`.

Если конфигурация содержит поле `extends`, библиотека сначала применяет указанные в нём конфигурации по порядку. Параметры каждой родительской конфигурации применяются по очереди: последняя в списке переопределяет предыдущие.

После этого применяются параметры самой выбранной конфигурации, которые переопределяют всё, что было задано раньше.
Если конфигурация не содержит `extends`, она полностью изолирована: применяются только её собственные параметры.

### Примеры конфигурационных файлов

``` json
{
    "gpu0": { 
        "backend": "by_index", 
        "platform_index": 0, 
        "device_index": 0,
        "if_gpu_unavailable": "abort",
        "queues_count": 1,
        "profiling": false
        },

    "debug": { 
        "profiling": true, // будет переопределен конфигурацией "gpu0"
        "allocator_type": "general",
        "verbosity": 3
        },

    "debug_gpu0": {
        "extends": ["debug", "gpu0"]
        }
}
```
### Итоговый набор параметров

| Параметр | Значение | Источник |
| :--- | :--- | :--- |
| backend | `by_index` | конфигурация `gpu0` |
| platform_index | `0` | конфигурация `gpu0` |
| device_index | `0` | конфигурация `gpu0` |
| if_gpu_unavailable | `"abort"` | конфигурация `gpu0` |
| queues_count | `1` | конфигурация `gpu0` |
| profiling | `false` | конфигурация `gpu0` |
| allocator_type | `general` | конфигурация `debug` |
| verbosity | `3` | конфигурация `debug` |
### Запуск
```bash
SPLA_CONFIG=debug_gpu0 ./program
```
Или через CLI:
```bash
./program --spla-config=debug_gpu0
```
---
```json
{    
    "gpu0": { 
        "backend": "by_index", 
        "platform_index": 0, 
        "device_index": 0,
        "if_gpu_unavailable": "abort",
        "queues_count": 1,
        "profiling": false,
        "allocator_type": "general",
        "verbosity": 3
        },

    "gpu0_profiling_true": { 
        "extends": ["gpu0"],
        "profiling": true // переопределит "profiling": false конфигурации "gpu0"
        }
}
```
### Запуск
```bash
SPLA_CONFIG=gpu0_profiling_true ./program
```
Или через CLI:
```bash
./program --spla-config=gpu0_profiling_true
```
Конфигурационные файлы состоят из списка именованных конфигураций. Конфигурации из разных файлов (по умолчанию, системного, пользовательского, пользовательского с явно указанным путем) сливаются по именам: одноимённые конфигурации объединяются по параметрам, а уникальные добавляются. Итоговый набор используется для разрешения `extends` и выбора активной конфигурации.

Имена в `extends` берутся из всех конфигураций, определённых в конфигурационгом файле по умолчанию, системном, пользовательском и пользовательском с явно указанным путем файлах конфигурации. Это позволяет наследовать конфигурации, определённые на другом уровне. Например, пользовательская конфигурация может наследовать `"default"` из файла по умолчанию, не дублируя его параметры.

### Пример конфигурационного файла
Конфигурационный файл по умолчанию:
``` json
{
    "default": {
        "backend": "gpu", // переопределится на "by_index"
        "if_gpu_unavailable": "use_cpu", // переопределится на "abort"

        "queues_count": 1,
        "profiling": false, // переопределится на true
        "allocator_type": "general",

        "verbosity": 2
        }
}
```
Конфигурационный файл, созданный пользователем:
```json
{
    "gpu0": { 
        "extends": ["default"],
        "backend": "by_index", // переопределяет "backend": "gpu"
        "platform_index": 0, 
        "device_index": 0,
        "if_gpu_unavailable": "abort", // переопределяет "if_gpu_unavailable": "use_cpu"
        "queues_count": 1,
        "profiling": true
        }
}
```

### Итоговый набор параметров

| Параметр | Значение | Источник |
| :--- | :--- | :--- |
| backend | `by_index` | конфигурация `gpu0` |
| platform_index | `0` | конфигурация `gpu0` |
| device_index | `0` | конфигурация `gpu0` |
| if_gpu_unavailable | `"abort"` | конфигурация `gpu0` |
| queues_count | `1` | конфигурация `gpu0` |
| profiling | `true` | конфигурация `gpu0` |
| allocator_type | `general` | конфигурация по умолчанию |
| verbosity | `2` | конфигурация по умолчанию |

---
---

# Распространенные сценарии использования библиотеки
  1. Быстрый запуск без настройки.
  2. Выбор между CPU и GPU без указания индексов.
  3. Многократный запуск на разных устройствах с разными параметрами.
  4. Запуск с конфигурациями из произвольного файла.

## Быстрый запуск без настройки

Если вы хотите получить работоспособное приложение без настройки - ничего настраивать не нужно. Библиотека использует параметры по умолчанию из [конфигурационного файла](#конфигурационный-файл). Этот сценарий работает одинаково на Linux, macOS и Windows. Если GPU недоступен, библиотека автоматически переключится на CPU, сообщив об этом. \
Перед первым запуском рекомендуется [выполнить](#проверка-работоспособности) `spla --softcheck`, чтобы убедиться, что всё необходимое для работы spla установлено.

## Выбор между CPU и GPU без указания индексов

Если вы хотите управлять типом устройства, но не указывать конкретные индексы OpenCL-платформ и устройств.

### Управление через backend

Тип устройства задаётся параметром backend
```json
{
    "gpu_only": { "backend": "gpu" }, // "gpu" - использовать первое доступное GPU;
    "cpu_only": { "backend": "cpu" }, // "cpu" - использовать CPU;
    "any":      { "backend": "any" }  // "any" - использовать первое доступное устройство любого типа.
}
```

Индексы `platform_index` и `device_index` при этих значениях игнорируются.

### Поведение при отсутствии GPU

Если выбран backend: `"gpu"`, но GPU недоступен, поведение определяется параметром `if_gpu_unavailable`:

```json
{
    "gpu_or_cpu": {
        "backend": "gpu",
        "if_gpu_unavailable": "use_cpu"
    },
    "gpu_only": {
        "backend": "gpu",
        "if_gpu_unavailable": "abort"
    }
}
```
`"use_cpu"` - библиотека переключается на CPU и продолжает работу. \
`"abort"` - библиотека возвращает ошибку и не продолжает работу. \
`if_gpu_unavailable` применяется только при backend: `"by_index"`, backend: `"gpu"` и backend: `"any"`. При backend: `"cpu"` параметр игнорируется.

## Многократный запуск на разных устройствах с разными параметрами

Вы регулярно запускаете одну и ту же программу на разных GPU и в разных режимах - например, на GPU 0 и 1, в режиме отладки и в режиме измерения производительности. Вместо того чтобы держать несколько файлов конфигурации, вы описываете всё в одном.
```json
{
    "default": { "backend": "gpu", "verbosity": 2 },

    "gpu0": { 
        "extends": ["default"], 
        "backend": "by_index", 
        "platform_index": 0, 
        "device_index": 0 
        },
    "gpu1": { 
        "extends": ["default"], 
        "backend": "by_index", 
        "platform_index": 0, 
        "device_index": 1 
        },
    "gpu2": { 
        "extends": ["default"], 
        "backend": "by_index", 
        "platform_index": 0, 
        "device_index": 2 
        },

    "debug": { 
        "extends": ["default"], 
        "verbosity": 3, 
        "profiling": true 
        },
    "bench": { 
        "extends": ["default"], 
        "verbosity": 0 
        },

    "debug_gpu1": { "extends": ["debug", "gpu1"] },
    "bench_gpu2": { "extends": ["bench", "gpu2"] }
}
```
Запуск 
```bash 
SPLA_CONFIG=gpu1 ./program
SPLA_CONFIG=debug ./program
SPLA_CONFIG=debug_gpu1 ./program
```
Подробнее в главе про [конфигурации](#конфигурации) и [наследование конфигураций (`extends`)](#наследование-конфигураций).

## Запуск с конфигурациями из произвольного файла

Вам нужно запустить тесты в CI с отдельным файлом конфигурации.

В среде CI неудобно и нежелательно трогать системный и пользовательский файлы конфигурации, а конфигурация тестов должна лежать в репозитории. Поэтому конфигурации описывают в отдельном файле внутри репозитория и указывают путь к нему при запуске и имя конфигурации.

```
my_project/
├── .github/
│   └── workflows/
│       ├── ci_with_conf.yml
│       └── ci_spla_conf.json
├── src/
└── tests/
```

### ci_spla_conf.json
```json
{
    "test_cpu": {
        "backend": "cpu",
        "queues_count": 1,
        "profiling": false,
        "allocator_type": "general",
        "verbosity": 1
        },
    "test_gpu0": {
        "backend": "by_index",
        "platform_index": 0,
        "device_index": 0,
        "if_gpu_unavailable": "abort",
        "queues_count": 2,
        "profiling": false,
        "allocator_type": "general",
        "verbosity": 0
        },
    "test_gpu1": {
        "backend": "by_index",
        "platform_index": 0,
        "device_index": 1,
        "if_gpu_unavailable": "abort",
        "queues_count": 2,
        "profiling": false,
        "allocator_type": "linear",
        "linear_alloc_size": 8,
        "verbosity": 0
        }
}
```

### ci_with_conf.yml
```yml
name: CI

on: [push, pull_request]

jobs:
  test:
    name: Test (${{ matrix.config }})
    runs-on: [self-hosted]

    strategy:
      fail-fast: false
      matrix:
        config: [test_cpu, test_gpu0, test_gpu1]

    steps:
      - name: Checkout repository
        uses: actions/checkout@v4

      - name: Set up Python
        uses: actions/setup-python@v5
        with:
          python-version: "3.11"

      - name: Install dependencies
        run: |
          python -m pip install --upgrade pip
          pip install pyspla pytest

      - name: Run tests
        env:
          SPLA_CONFIG_FILE: ${{ github.workspace }}/.github/workflows/ci_spla_conf.json
          SPLA_CONFIG: ${{ matrix.config }}
        run: pytest tests/
```
Если пользователь не укажет конфигурацию, то будет взята конфигурация `"default"` из файла, который идет с библиотекой.