# Multi-os research

ce projet est un RAT (remote access tool) avec keylogger, donc 90% des vraies difficultés viennent justement des différences OS.

Si tu n’anticipes pas ça dès maintenant, tu vas tout casser au cercle 7.

Je vais te faire une cartographie propre des zones où tu DOIS séparer par OS, avec :

- pourquoi
- quelles libs / syscalls changent
- à quoi ressemblent les appels

## 🧠 Vue globale : où ça diverge vraiment selon l’OS

### 🔴 1. Réseau (Cercle 1 & 2) → ⚠️ DIFFÉRENT mais maîtrisable

Pourquoi

Windows ≠ Linux au niveau des sockets (Winsock vs POSIX)

#### Différences

Windows → Winsock
Linux → BSD sockets (POSIX)

#### Exemple

Linux

```c++
int sock = socket(AF_INET, SOCK_STREAM, 0);
connect(sock, ...);
send(sock, buffer, size, 0);
Windows
WSAStartup(MAKEWORD(2,2), &wsaData);
SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
connect(sock, ...);
send(sock, buffer, size, 0);
⚠️ Impact architecture

👉 Tu DOIS faire :

```text
ISocket
  ├── LinuxSocket
  └── WindowsSocket
  ```

👉 + une classe wrapper type LPTF_Socket (comme demandé)

### 🟠 2. Exécution de commandes (Cercle 3) → ⚠️ TRÈS DIFFÉRENT

Windows → cmd.exe / PowerShell
Linux → /bin/sh, bash

Exemple

Linux

```c++
system("ls");
```

Windows

```c++
system("dir");
```

Ou plus propre :

Linux

```c++
popen("ls", "r");
```

Windows

```c++
_popen("dir", "r");
```

⚠️ Impact

👉 Tu dois abstraire :

```text
ICommandExecutor
  ├── LinuxCommandExecutor
  └── WindowsCommandExecutor
  ```

### 🔴 3. Keylogger (Cercle 3) → 💀 GROS MORCEAU

C’est 100% OS dépendant, aucun standard C++

#### 🪟 Windows

API : Win32

```c++
SetWindowsHookEx(WH_KEYBOARD_LL, callback, NULL, 0);
```

- fonctionne en global
- assez “simple”

#### 🐧 Linux

👉 beaucoup plus compliqué :

options :

- /dev/input/event* (root requis)
- X11 (XRecord, XGrabKey)
- Wayland → quasi bloqué 😬

Exemple (simplifié X11) :

```c++
Display* display = XOpenDisplay(NULL);
XNextEvent(display, &event);
```

#### ⚠️ Impact architecture (TRÈS IMPORTANT)

👉 Interface obligatoire :

```text
IKeylogger
  ├── WindowsKeylogger
  └── LinuxKeylogger
```text

👉 Et surtout :

- pas les mêmes droits
- pas les mêmes comportements
- parfois impossible selon config (Wayland)

### 🟠 4. Infos système (OS, user, hostname)

Différences

Linux

```c++
gethostname(buffer, size);
getlogin();
uname(&info);
```

Windows

```c++
GetComputerName(buffer, &size);
GetUserName(buffer, &size);
```

👉 Interface :

```text
ISystemInfo
  ├── LinuxSystemInfo
  └── WindowsSystemInfo
  ```

### 🟠 5. Process list

Linux
// /proc filesystem
Windows
CreateToolhelp32Snapshot(...)
Process32First / Process32Next

👉 Interface :

```cmd
IProcessManager
```

### 🔴 6. File system & persistence (Cercle 8)

Différences énormes

```Linux
/home/user/.config
.bashrc
systemd / cron
```

```Windows
Registry
Startup folder
AppData
```

👉 Interface :

```text
IPersistence
```

### 🟡 7. Threads / async

👉 std::thread est cross-platform

Mais si tu touches :

- Windows → CreateThread
- Linux → pthread

👉 reste en C++ standard → évite problème

### 🟢 8. Qt (GUI)

Qt gère tout le cross-platform:

AUCUN code spécifique OS

### 🟢 9. PostgreSQL

safe:
libpq = cross-platform

## 🧱 Structure que tu DOIS viser

Franchement, si tu veux éviter de souffrir :

```text
/common
    Packet
    Protocol
    Interfaces/
        ISocket
        IKeylogger
        ICommandExecutor
        ISystemInfo
        IProcessManager

/windows
    WindowsSocket
    WindowsKeylogger
    WindowsCommandExecutor
    ...

/linux
    LinuxSocket
    LinuxKeylogger
    LinuxCommandExecutor
    ...

/agent
/server
/gui (Qt)
```

## ⚠️ Les 3 pièges majeurs de ton projet

1. Le keylogger
    - Linux ≠ Windows ≠ Wayland
2. Le protocole mal anticipé
    - si mal conçu → refactor complet cercle 3+
3. Le cross-platform tardif

## 🎯 Ce que tu dois faire CETTE SEMAINE

Tu es sur la bonne approche. Priorités :

1. Définir interfaces
    - ISocket
    - ICommandExecutor
    - IKeylogger
    - etc

2. Lister TOUS les cas d’usage réseau

→ pour ton protocole

1. Étudier :
Winsock vs POSIX
Keylogger Windows vs Linux

## 💬 Conclusion claire

👉 Les parties vraiment OS dépendantes :

- sockets
- keylogger (le pire)
- commandes système
- process
- filesystem / persistence

👉 Les parties safe :

- protocole
- sérialisation
- Qt
- PostgreSQL
