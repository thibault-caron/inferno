# notes windows agent

## wine

### cleanup

```bash
cd ..
rm -rf build-win && mkdir build-win && cd build-win
```

### from agent/build-win

```bash
cmake -DCMAKE_TOOLCHAIN_FILE="$PWD/../../toolchain-mingw.cmake" \
      -DOPENSSL_ROOT_DIR=/usr/x86_64-w64-mingw32 \
      ..

make
```

```bash
SERVER_HOST=localhost wine /mnt/Data/_Thibault/Documents/_Plateforme/!Dev_Log2/C++/Inferno/inferno/agent/build-win/bin/agent.exe
```

```bash
WINEDEBUG=+err SERVER_HOST=localhost wine /mnt/Data/_Thibault/Documents/_Plateforme/!Dev_Log2/C++/Inferno/inferno/agent/build-win/bin/agent.exe 2>&1 | grep --line-buffered -E "err:|FAIL"
```

## sur windows

```bash
set SERVER_HOST=localhost && agent\build-win\bin\agent.exe
```

```bash
ou powershell
set SERVER_HOST=localhost && agent\build-win\bin\agent.exe
```
