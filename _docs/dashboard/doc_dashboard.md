# Inferno Dashboard — documentation

## Role

Graphical interface of the project. It connects to the server, shows the
connected agents and lets me send them commands.

The dashboard never talks to the agents directly. Everything goes through the
server, which passes the messages both ways.

```
Dashboard  --COMMAND-->  Server  -->  Agent
                                       | runs it
Dashboard  <--RESPONSE--  Server  <----+
```

---

## Architecture

Two separate parts: the network and the display. They do not know each other
directly, they talk through Qt signals.

```
+-------------------- DISPLAY ---------------------+
|  MainWindow                                      |
|     +-- AgentItemWidget      one agent row       |
|     +-- MetricCardsWidget    the 4 cards         |
|     +-- LineChartWidget x4   the charts          |
|     +-- ProcessTableWidget   the process table   |
|     +-- SeriesHistory x4     the kept points     |
+------------------------+-------------------------+
                         | Qt signals
+------------------------+-------------------------+
|                     NETWORK                      |
|  ServerClient       connects, listens, sorts     |
|  DashboardSession   buffer + splits into frames  |
|  SocketFactory      TCP, or TLSSocketFactory     |
|  WindowsSocket / LinuxSocket                     |
+--------------------------------------------------+
```

`ServerClient` does not include any widget file. It sends a signal when it has
received something, and `MainWindow` decides what to do with it.

---

## The classes

| Class                | Role                                         |
| -------------------- | -------------------------------------------- |
| `MainWindow`         | Puts the widgets together, wires the buttons |
| `ServerClient`       | Talks to the server                          |
| `DashboardSession`   | Byte buffer, and splitting into frames       |
| `AgentItemWidget`    | One row of the agent list                    |
| `MetricCardsWidget`  | The 4 cards at the top                       |
| `LineChartWidget`    | One chart, drawn with QPainter               |
| `ProcessTableWidget` | The RUNNING PROCESSES table                  |
| `SeriesHistory`      | Keeps the last N points of M series          |
| `uiutils`            | Free function `makeLabel()`                  |
| `theme.h`            | The colours, with names                      |

---

## Main methods

### ServerClient

| Method                                   | Takes                               | Returns   |
| ---------------------------------------- | ----------------------------------- | --------- |
| `connectToServer(host, port)`            | `QString`, `quint16`                | `bool`    |
| `sendCommand(target, type, data)`        | `QString`, `CommandType`, `QString` | `void`    |
| `sendDisconnect(target)`                 | `QString`                           | `void`    |
| `sendRegister()` _(private)_             | —                                   | `void`    |
| `onReadyRead()` _(private)_              | —                                   | `void`    |
| `handleFrame(frame)` _(private)_         | `const Frame&`                      | `void`    |
| `handleData(payload)` _(private)_        | `const std::vector<uint8_t>&`       | `void`    |
| `resolveCertPath()` _(static, internal)_ | —                                   | `QString` |

**Signals sent to `MainWindow`:**

| Signal                                    | Sent when                          | Handled by              |
| ----------------------------------------- | ---------------------------------- | ----------------------- |
| `agentReceived(id, name, os, ip, online)` | an agent arrives                   | `addAgentItem()`        |
| `responseReceived(target, text)`          | a shell command has answered       | `showOutput()`          |
| `processListReceived(target, processes)`  | the process list arrives           | `setProcesses()`        |
| `metricsReceived(target, sample)`         | a measurement arrives              | `onMetricsReceived()`   |
| `osInfoReceived(target, info)`            | an `OS_INFO` answer arrives        | `onOsInfoReceived()`    |
| `agentDisconnected(target)`               | the server reports a disconnection | `onAgentDisconnected()` |

### MainWindow

| Method                                    | Takes                                                | Returns            |
| ----------------------------------------- | ---------------------------------------------------- | ------------------ |
| `addAgentItem(id, name, os, ip, online)`  | 4 `QString` + `bool`                                 | `void`             |
| `showOutput(text)`                        | `const QString&`                                     | `void`             |
| `buildContentArea()`                      | —                                                    | `void`             |
| `createChart(title, series, labels, ...)` | `QString`, `QVector<QVector<double>>`, `QStringList` | `LineChartWidget*` |
| `buildStatusBar()`                        | —                                                    | `void`             |
| `clearAgentView()`                        | —                                                    | `void`             |
| `updateAgentCounters()`                   | —                                                    | `void`             |
| `updateStatusBadge()`                     | —                                                    | `void`             |
| `closeEvent(event)` _(protected)_         | `QCloseEvent*`                                       | `void`             |

**Slots:**

| Slot                                | What it does                                     |
| ----------------------------------- | ------------------------------------------------ |
| `onMetricsReceived(target, sample)` | Fills the cards and adds a point to the 4 charts |
| `onOsInfoReceived(target, info)`    | Puts the OS version in the badge                 |
| `onAgentDisconnected(target)`       | Turns the dot red and recomputes the counters    |

**Members:** `m_client`, `m_processTable`, `m_metricCards`, the 4
`LineChartWidget*`, the 4 `SeriesHistory`, `m_onlineLabel`, `m_target` (the
agent shown), `m_streamingTarget` (the agent that streams), `m_osBadgeDetailed`.

### SeriesHistory

| Method                                  | Takes                                          | Returns                    |
| --------------------------------------- | ---------------------------------------------- | -------------------------- |
| `SeriesHistory(seriesCount, maxPoints)` | `int`, `int`                                   | —                          |
| `append(values)`                        | `const QVector<double>&`, one value per series | `void`                     |
| `series()`                              | —                                              | `QVector<QVector<double>>` |
| `clear()`                               | —                                              | `void`                     |

Members: `m_series` (the outer vector indexes the series, the inner one the
points) and `m_maxPoints`.

### ProcessTableWidget

| Method                          | Takes                             | Returns    |
| ------------------------------- | --------------------------------- | ---------- |
| `setProcesses(processes)`       | `const std::vector<ProcessInfo>&` | `void`     |
| `createSeparator()` _(private)_ | —                                 | `QWidget*` |
| `createBar(value)` _(private)_  | `int` 0-100                       | `QWidget*` |

### MetricCardsWidget

| Method                                                      | Takes                  | Returns    |
| ----------------------------------------------------------- | ---------------------- | ---------- |
| `updateFromSample(sample)`                                  | `const MetricsSample&` | `void`     |
| `updateMetric(key, value)`                                  | `QString`, `QString`   | `void`     |
| `updateSubtitle(key, value)`                                | `QString`, `QString`   | `void`     |
| `clear()`                                                   | —                      | `void`     |
| `createMetricCard(key, title, value, subtitle)` _(private)_ | 4 `QString`            | `QWidget*` |

The keys are `"cpu"`, `"memory"`, `"disk"`, `"network"`. Two tables link a key
to its label: `m_metricValues` for the big value, `m_metricSubtitles` for the
line below.

### AgentItemWidget

| Method                            | Takes                | Returns |
| --------------------------------- | -------------------- | ------- |
| `setAgent(name, details, online)` | 2 `QString` + `bool` | `void`  |
| `setOnline(online)`               | `bool`               | `void`  |

### LineChartWidget

| Method                                      | Takes                             | Returns |
| ------------------------------------------- | --------------------------------- | ------- |
| `setSeries(series)`                         | `const QVector<QVector<double>>&` | `void`  |
| `setLabels(labels)`                         | `const QStringList&`              | `void`  |
| `setYMax(max)`                              | `double`                          | `void`  |
| `setYUnit(unit)`                            | `const QString&`                  | `void`  |
| `setDashed(indices)` / `setFilled(indices)` | `const QVector<int>&`             | `void`  |

### uiutils

| Function                      | Takes                | Returns   |
| ----------------------------- | -------------------- | --------- |
| `makeLabel(text, objectName)` | `QString`, `QString` | `QLabel*` |

---

## Technical choices

**The widgets are kept as pointers.** Before, I wrote
`insertWidget(0, new ProcessTableWidget(this))`: the widget was shown but its
address was lost. Now I keep it in `m_processTable`, so I can call
`setProcesses()` when the data arrives. Same thing for the four charts and for
the online agents label.

**`setProcesses` clears the grid, then builds it again.** Otherwise the new
rows would pile up on the old ones.

**A separate struct for the display.** The protocol gives numbers, the table
wants text (`"18.3%"`, `"6.5 MB"`). That is why there is `ProcessRow`,
different from the `ProcessInfo` of the protocol — both had the same name at
the start, which made a collision.

**A `target → CommandType` table.** A `RESPONSE` frame does not say what it
contains, and the id is useless, because the dashboard sends `id = 0` and the
server assigns the real one. So I write down what I asked each agent, to know
how to read its answer.

This solution is not perfect: I only keep one type per agent. Because I send
`START_METRICS` then `RUNNING_PROCESSES` on selection, the acknowledgement of
the first one can be taken for a process list. So I put the parsing inside a
`try`: if it fails, I treat the answer as text, instead of letting the
exception kill the application.

**`makeLabel` is a free function**, not a method: it keeps no state. It was
copied in three classes, now there is only one.

**The dashboard accumulates the chart points.** The server sends one
measurement at a time, not a history. `SeriesHistory` keeps the last 20 points
of each series: on each measurement I add a point, I drop the oldest one, and I
give everything back to the chart. One instance per chart, and each one only
knows its own series.

**Formatting belongs to the display.** `ServerClient` passes the raw
`MetricsSample` and the separate fields of an agent. It is
`MetricCardsWidget::updateFromSample` that converts to `"1.2 GB"`, and
`MainWindow::addAgentItem` that builds the details line. The network layer
makes no display string.

**Each chart has its own scale.** The axis was fixed from 0 to 100 % in the
code, which flattened the memory in GB and the throughputs. `setYMax` and
`setYUnit` let me set 100 % for the CPU, 20 % for the memory, 50 KB/s for the
network and 10 MB/s for the disk. The values that go above are brought back to
the ceiling with `qBound`, otherwise they would be drawn outside the frame.

**The measurements are filtered by agent.** All the agents stream at the same
time, and the frames arrive mixed. So each signal carries the `target`, and the
slot starts by comparing it with `m_target`: without this, the cards would show
the values of several machines one after the other.

**Two variables for two states.** `m_target` says which agent is shown,
`m_streamingTarget` says which one streams. It is not the same thing: after a
second click, the agent stays shown but its stream is stopped. One single
variable could not carry both answers.

**The data of the agent is stored on its item.** MAC on `Qt::UserRole`, name on
`+1`, OS on `+2`, IP on `+3`, online state on `+4`. This is what lets me fill
the header on click and recompute the counters without asking the widgets.

**`addAgentItem` updates before it adds.** It first looks for an item with this
MAC. If it exists, it fixes the state and the dot, then it returns; if not, it
creates the row. Without this, an agent that reconnects would appear twice.

**The TLS certificate is looked for in several places.** The path depended on
the build tree: two levels above the executable land on the root with the Linux
script, but not with Qt Creator, which adds one folder per kit.
`resolveCertPath()` tries three paths and returns the first one that exists.

**The wrong network values are skipped, not capped.** The agent sometimes sends
throughputs close to 2^64. I skip the sample and I write a message, instead of
showing a value that looks right and would hide the problem.

---

## From the click to the display

Example with the **Process list** button:

1. `MainWindow` calls `sendCommand(m_target, RUNNING_PROCESSES, "")`
2. `ServerClient` builds and sends a `COMMAND` frame
3. the server passes it to the agent, which answers
4. the `QSocketNotifier` calls `onReadyRead()`, which takes out the complete
   frame
5. `handleFrame()` sees a `RESPONSE` and finds the type of the command that was
   sent
6. it parses and emits `processListReceived`
7. `MainWindow` calls `m_processTable->setProcesses()`

Example with a **measurement**:

1. the agent sends a `DATA` frame every second
2. `handleData()` parses the `DashboardData`, then the `MetricsSample`
3. it emits `metricsReceived(target, sample)`
4. `onMetricsReceived` ignores the measurement if it does not come from
   `m_target`
5. if it does, it fills the cards and adds a point to each of the 4 charts

---

## What the selection of an agent does

A click on a row does several things:

1. the header is filled with the name, the OS badge and the IP badge
2. if another agent was streaming, it receives `STOP_METRICS`
3. if the agent changes, `clearAgentView()` clears the charts, the table, the
   cards and the console
4. the new agent receives `START_METRICS` then `RUNNING_PROCESSES`
5. the badge of the header turns green

A second click on the same agent stops the stream without clearing anything:
the row stays selected, the badge turns red. The next click starts it again.

This is why the signal used is `itemClicked` and not `currentItemChanged`: this
last one is not triggered when you click again on the row that is already
selected.

---

## What is still fixed in the `.ui`

- `last sample: 0.3 s ago` — to connect to the `timestamp` of the last
  `MetricsSample` received
- `db: PostgreSQL connected` — no data of the protocol carries this
  information, the label will be removed
