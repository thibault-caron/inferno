# The 9 Circles of Hell — INFERNO (Reframed)
> Monitoring & Remote Diagnostics Edition

---

## Ante-hell — Rules

> *"Abandon all hope, ye who enter here."*

Before beginning, a few rules to follow:

- All classes must respect the Coplien canonical form
- Do not use a pointer when a reference can do the job
- Variables must be `const` unless there is a justified reason not to
- The use of global variables is forbidden
- Attributes must be private, with getters/setters in place
- Use smart pointers
- In order to access the different stages of your project, tags must be set on your repo

---

## Circle 1 — Limbo

> *"Where are confined, without other torment than a dull languor, all those who have not known the blessing of C++"*

After crossing the gates of hell, ready to begin the journey, the first task is to forge and understand communication tools. For this, two console programs capable of communicating over the network must be developed: an **agent** and a **server**.

These programs must be built around an `LPTF_Socket` class, encapsulating the different syscalls needed for network exchanges.
No network syscall must appear outside the `LPTF_Socket` class.
The programs must use neither threads nor fork.

The circle is considered cleared when the server is capable of exchanging simultaneously with multiple agents.

---

## Circle 2 — Lust

> *"The fate of damned souls in this circle is to be swept away by relentless winds."*

The second circle is a mutual design circle. With another group (or more), design a **binary communication protocol**. This protocol must be evolutive and take into account the needs of the entire journey through the 9 circles.

As a binary protocol, it must be composed of one or more combined classes/structs. An `LPTF_Packet` class must allow storing and extracting information in preparation for a network transfer.

This protocol must be named and written in the style of an RFC (see RFC 959 or 1149).

---

## Circle 3 — Gluttony

> *"Cerberus harasses and bites them. A relentless rain of hail and snow falls upon the sinful shades."*

Things get serious. The agent and server programs evolve into **curious and data-hungry monitoring software**.

The agent must now be capable, on server command, of:

- Returning information about the host machine: hostname, current user, OS type and version, architecture
- **Starting or stopping a performance metrics stream**, capturing system resource usage continuously and sending it back — even when no other activity is occurring (see Metrics Streaming below)
- Returning the list of currently running processes, including PID, name, and resource usage
- Executing a diagnostic system command on the host (e.g. `df`, `netstat`, `top -bn1`, `tasklist`) and returning the result

The prototypes of these different request types and their responses must be described by the protocol designed in the Lust circle.

### Metrics Streaming

The metrics stream is the technical centrepiece of this circle. When the server sends `START_METRICS`, the agent begins sampling system resources at a regular interval and streaming the results back via `DATA` messages — continuously, in the background, until `STOP_METRICS` is received.

Metrics to collect per sample:

- **CPU**: overall usage percentage + per-core breakdown
- **Memory**: total, used, available (physical + swap)
- **Disk I/O**: read and write throughput per device
- **Network I/O**: bytes sent and received per interface

Each sample is a self-contained payload, chunked if necessary using the protocol's chunking mechanism.

---

## Circle 4 — Greed

> *"Half filled with the avaricious, half with the prodigal. They push enormous weights with the effort of their chests."*

Out of ambition more than strict functional necessity, the server program must be given a new face: a **GUI built with Qt**.

The GUI must display a list of connected agents. For each, it must be possible to:

- Execute the requests defined in Circle 3
- Receive and display the responses
- View the IP address of each agent
- Disconnect an agent from the server
- Display the live metrics stream in a readable, updating view (graph or table)

---

## Circle 5 — Anger

> *"In the muddy waters of the Styx, the wrathful fight one another."*

Reacting impulsively, on the impulse of anger, is rarely wise. The same applies to the handling of information.

The server must offer the ability to display responses arriving from agents (command results, metrics streams) **directly in the GUI and/or record them in a SQL database**. For this, use PostgreSQL.

All database calls and query construction must be done through an `LPTF_Database` class.

On launch, the server GUI loads existing data from the database and presents it to the user, with a clear visual distinction between **online** and **offline** agents.

---

## Circle 6 — Heresy

> *"The abode of unbelievers, plunged into burning tombs."*

Once the satisfaction of having access to rich system information has passed, why not make it truly useful?

Create a widget in the server GUI that allows launching an **analysis of the data collected from the different agents** in order to extract:

- **CPU anomalies**: sustained spikes, unusual per-core imbalances
- **Memory pressure events**: usage approaching capacity, swap activity patterns
- **Suspicious processes**: processes with unusually high resource consumption or unexpected names (cross-referenced against the baseline process list)
- **Network anomalies**: unexpected bandwidth spikes, unusual traffic patterns on monitored interfaces

The different analysis methods must be implemented in an `LPTF_Analysis` class, encapsulating the calls to the different tools at your disposal.

---

## Circle 7 — Violence

> *"The first ring punishes Windows users, boiled in a river of boiling blood. The second holds Linux users transformed into dry shrubs. The third is a burning wasteland where a rain of flame falls on Mac users."*

What is physical pain compared to the happiness of the mind?

Your agent currently runs only on your host OS. In order to make it powerful and universal, it is now time to make it **cross-platform — executable on both Windows and Linux**.

Each of your classes that encapsulates system functions must inherit from an interface. One class per OS will be necessary. During compilation, preprocessor directives must allow the source code to be translated into a language understandable by the machine reading it.

> **Note**: this circle is where Metrics Streaming gets its full depth — `procfs` on Linux (`/proc/stat`, `/proc/meminfo`, `/proc/net/dev`) versus PDH/WMI on Windows. Same interface, two implementations.

---

## Circle 8 — Fraud

> *"Called Malebolge, punishing the fraudulent, divided into ten circular ditches."*

In order for your agent to be as **robust and production-ready** as possible:

- Run the agent as a **background daemon** (no visible console window in normal operation)
- Integrate its compiled code into a **service wrapper** — on Linux a `systemd` unit, on Windows a Windows Service
- **Install the agent in the appropriate system location** for background services on the target OS (e.g. `/usr/local/bin/` on Linux, `%ProgramFiles%` on Windows)
- **Bonus**: register the agent to **start automatically at system boot** via the OS service manager (systemd enable / Windows Service auto-start)
- Implement **reconnection resilience**: if the connection to the server is lost, the agent must attempt to reconnect at regular intervals until the server is available again

> This is exactly how real-world monitoring agents work — Datadog Agent, Prometheus Node Exporter, and similar tools follow this exact pattern.

---

## Circle 9 — Treachery

> *"Here are punished those who divide men. Their limbs, cut and divided, hang more or less mutilated according to the gravity of the divisions they caused on Earth."*

Once the work is done, all that remains is to spread it across the abyss and start again.

**Deploy your agent to a fleet of test machines** (virtual machines, teammates' machines with consent, lab environments) and validate that:

- The server correctly handles multiple simultaneous agents
- Metrics streams from different agents are correctly distinguished and stored
- The GUI remains usable under load
- The reconnection mechanism works correctly after a simulated server outage

Document your deployment procedure and results.