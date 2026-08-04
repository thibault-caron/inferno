# notes windows agent

## wine

### cleanup

```bash
cd ..
rm -rf build-win && mkdir build-win && cd build-win
```

### from agent/build-win

```bash
cmake -S .. -B . \
            -DCMAKE_TOOLCHAIN_FILE="$(realpath ../../toolchain-mingw.cmake)" \
            -DOPENSSL_ROOT_DIR=/usr/x86_64-w64-mingw32
```

```bash
make
```

## avec 

```bash
SERVER_HOST=localhost wine \[path to project]/inferno/agent/build-win/bin/agent.exe
```

```bash
WINEDEBUG=+err SERVER_HOST=localhost wine \[path to project]/inferno/agent/build-win/bin/agent.exe 2>&1 | grep --line-buffered -E "err:|FAIL"
```

## sur windows

```bash
set SERVER_HOST=localhost && agent\build-win\bin\agent.exe
```

ou powershell

```bash
$env:SERVER_HOST="localhost"; .\bin\agent.exe
```
