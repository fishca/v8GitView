# Архитектура v8GitView

Документ описывает текущий каркас (0.1.0) и решения, заложенные под большие
репозитории 1С:Предприятие.

## Слои

```
┌─────────────────────────────────────────────────────────────┐
│ UI (GUI thread)                                             │
│  MainWindow ── CommitView (QTableView)                      │
│                    │                                        │
│                    ├── CommitModel (QAbstractTableModel)    │
│                    └── CommitDelegate (QStyledItemDelegate)  │
│                                                             │
│  CommitModel::needMore() ──► MainWindow::requestMore()      │
└──────────────────────────────┬──────────────────────────────┘
                               │ queued signals/slots
┌──────────────────────────────▼──────────────────────────────┐
│ HistoryLoader (QThread)                                     │
│  GitRepository (libgit2 RAII)                               │
│  git_revwalk (GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME)         │
│  GraphLayout                                                │
└─────────────────────────────────────────────────────────────┘
```

- `src/git/` — тонкая RAII-обёртка libgit2 и структура `CommitInfo`.
- `src/history/` — загрузчик, модель, lane-разметка и `HistoryRow`.
- `src/ui/` — палитра/шрифты, таблица, делегат отрисовки.
- `src/app/` — главное окно и связывание сигналов.
- `src/main.cpp` — инициализация libgit2, темы, регистрация метатипов.

## Потоки и владение

`HistoryLoader` создаётся без родителя и переносится в `QThread`:

```cpp
m_loader = new HistoryLoader;
m_loader->moveToThread(&m_loaderThread);
connect(&m_loaderThread, &QThread::finished,
        m_loader, &QObject::deleteLater);
```

Все запросы к загрузчику идут через `QMetaObject::invokeMethod(...,
Qt::QueuedConnection)`. Сигналы `opened`, `batch`, `failed` доставляются в GUI
поток через queued connection.

Отмена:

- `HistoryLoader::cancel()` вызывает `QAtomicInt::storeRelaxed(1)` и безопасен
  из GUI потока.
- Цикл `git_revwalk_next` проверяет флаг на каждом коммите.
- При открытии нового репозитория GUI сначала вызывает `cancel()`, затем
  ставит в очередь `openRepository`. Старые батчи, если успели уйти, придут
  раньше сигнала `opened`, а `openPath` уже очистил модель; `opened` ещё раз
  подтверждает новое состояние.
- При закрытии окна вызывается `cancel()`, затем `shutdown` через
  `BlockingQueuedConnection`, после чего поток завершается.

Один экземпляр `GitRepository` используется только в потоке загрузчика.

## Загрузка истории

1. `GitRepository::open()` вызывает `git_repository_open_ext`, поэтому можно
   передать путь к подкаталогу внутри рабочей копии.
2. `git_repository_is_empty` отсекает пустой репозиторий.
3. `GitRepository::refs()` один раз строит карту `oid -> [refs]`. Символические
   `HEAD` и `refs/remotes/origin/HEAD` пропускаются, чтобы не дублировать
   бейджи. Список сортируется и дедуплицируется.
4. `git_revwalk` сортируется как `GIT_SORT_TOPOLOGICAL | GIT_SORT_TIME`:
   родитель никогда не появляется раньше ребёнка, а внутри topo-порядка
   коммиты идут по времени.
5. `loadNextBatch()` читает до 1 000 коммитов, формирует `HistoryRow` и
   отправляет батч. Признак `hasMore` позволяет модели запросить следующую
   порцию.

Модель:

- `canFetchMore()` возвращает `m_hasMore`.
- `fetchMore()` ставит `m_fetchPending` и испускает `needMore()`.
- `appendBatch()` вставляет строки одним `beginInsertRows/endInsertRows`,
  сбрасывает `m_fetchPending` и при необходимости сразу просит ещё батч,
  пока не заполнен read-ahead (400 строк).
- `QTableView` сам вызывает `fetchMore()` при прокрутке к концу.

## Структуры данных

```cpp
struct CommitInfo {
    QString oid;          // 40 hex
    QString shortOid;     // 8 hex
    QStringList parents;
    QString author;
    QString email;
    qint64 authorTime;
    QString subject;
    QStringList refs;     // полные имена refs/heads/..., refs/tags/...
};

struct GraphEdge {
    int fromLane;         // lane в верхней координатной сетке
    int toLane;           // lane в нижней координатной сетке
    int color;
};

struct GraphRow {
    int lane;             // lane самого коммита
    int color;
    int laneCount;
    bool incoming;        // линия входит в строку сверху
    QVector<GraphEdge> edges;
};

struct HistoryRow {
    CommitInfo commit;
    GraphRow graph;
};
```

`HistoryRow` регистрируется как метатип для queued-сигналов; `GraphRow`
кладётся в `QVariant` через роль `CommitModel::GraphRole`.

## Алгоритм GraphLayout

Состояние — массив активных lane'ов:

```cpp
struct Lane { QString oid; int color; };
QVector<Lane> m_lanes;
```

`oid` — коммит, который ожидается в этом lane ниже. Инвариант: один `oid`
встречается не более чем в одном lane.

Для очередного коммита:

1. Найти его lane по `oid`. Если коммит не найден (новый head или корень без
   детей в загруженном диапазоне) — добавить lane в конец.
2. Запомнить `row.lane`, `row.color`, `row.incoming`.
3. Удалить lane коммита из состояния: он потреблён.
4. Для каждого родителя:
   - если родитель уже есть в активном lane — провести ребро в этот lane;
   - иначе первому родителю выделить позицию коммита (история продолжается
     вертикально), остальным — новый lane в конце (merge-ветки).
5. Для всех остальных верхних lane'ов найти их позицию после вставок и
   добавить ребро «проходит сквозь строку».
6. `laneCount = max(top.size(), bottom.size(), max индекс ребра + 1)`.

Сложность — `O(N * lanes)`. Для типичной 1С-истории число одновременно
активных lane'ов невелико (единицы-десятки), поэтому это дешевле, чем
постобработка `git log --graph`.

### Отрисовка графа

`CommitDelegate::paintGraph`:

- вертикальные переходы и merge-ветки рисуются кубическими кривыми
  (`QPainterPath::cubicTo`) с контрольными точками на середине высоты строки;
- `incoming` рисует вертикальный отрезок от верхней границы к точке коммита;
- точка коммита — залитый круг с полупрозрачным halo;
- цвета берутся из палитры `Theme::laneColor` (16 цветов по кругу);
- ширина графовой зоны зависит от максимального `laneCount`, который модель
  сообщает через `maxLanesChanged`.

## Отрисовка таблицы

- Три колонки: Commit (stretch), Author (fixed 190 px), Date (fixed 150 px).
- `verticalHeader` скрыт, строки фиксированной высоты 28 px.
- Делегат рисует только первую колонку: граф, короткий SHA, бейджи refs и
  сообщение. Автор и дата используют стандартный делегат.
- Фон строки рисуется вручную с учётом `QStyleOptionViewItem::Alternate`,
  выделения и hover.
- Текст обрезается `QFontMetrics::elidedText` по фактической ширине.
- Бейджи ограничены по ширине: если они не помещаются, рисуется `+N`.
- Шрифт SHA — моноширинный (`Cascadia Mono` с fallback на `Consolas`).

## Производительность

Ключевые решения:

1. **Нет виджета на коммит.** Только модель и делегат; Qt рисует видимые
   строки.
2. **Ленивая загрузка.** Первый батч — 1 000 коммитов; остальное по мере
   прокрутки.
3. **Плоские данные.** `QVector<HistoryRow>`; никаких объектов libgit2 после
   чтения.
4. **Один обход.** `git_revwalk` не перезапускается при догрузке; состояние
   lane'ов сохраняется между батчами.
5. **Без блобов.** Для истории не читаются деревья и содержимое файлов.
6. **Фоновый поток.** `git_revwalk_next` и `git_commit_lookup` не блокируют
   GUI.

Что стоит добавить для очень больших 1С-репозиториев:

- окно загруженных коммитов с выгрузкой старых строк (сейчас загруженные
  строки остаются в памяти);
- чтение `commit-graph` (libgit2 поддерживает его в revwalk, но не Bloom-
  фильтры для path-limited history);
- история файла через `git log --follow -- <path>` с Bloom-фильтрами;
- инкрементальное обновление при `FETCH_HEAD`/смене HEAD;
- виртуализация с `QAbstractItemModel::canFetchMore` уже есть, но для
  миллиона коммитов нужен ещё и верхний предел памяти.

## Лицензионные заметки

- Код проекта — MIT.
- libgit2 линкуется динамически; лицензия GPL-2.0 с linking exception
  совместима с MIT-проектом.
- Qt линкуется динамически по LGPL-3.0; при распространении бинарников нужно
  приложить тексты лицензий и обеспечить возможность замены Qt-библиотек.
- Код lazygit/GitAhead/SourceGit не копировался; используются только идеи
  компоновки и визуального оформления.
