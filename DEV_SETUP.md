
# Dev Setup

> You need **Docker Desktop** and nothing else.  
> No compiler, no CMake, no GTest — all of that lives inside the containers.

---

## First time setup

**1. Clone the repo**

```bash
git clone <repo-url>
cd INFERNO
```

**2. Create your `.env` file**

```bash
cp .env.template .env
```

Open `.env` and fill in the values. Ask a teammate if you don't know what to put.

**3. Build the Docker images** (only needed once, or after a Dockerfile change)

```bash
docker compose build
```

**4. Start everything**

```bash
docker compose up
```

This will automatically:

- Build the shared library
- Run all tests (agent + server + shared)
- If tests pass → run the server and agent binaries
- If any test fails → stop and print the error

---

## Daily workflow

### Start the environment

```bash
docker compose up
```

### Stop the environment

```bash
docker compose down
```

### Rebuild after a Dockerfile change

```bash
docker compose build
docker compose up
```

---

## Running commands manually inside a container

Sometimes you want to build or test something specific without restarting everything.

**Open a shell inside the server container:**

```bash
docker exec -it inferno-server bash
```

**Open a shell inside the agent container:**

```bash
docker exec -it inferno-agent bash
```

Once inside, you have access to `inferno.sh`:

```bash
# Build only
./inferno.sh build server
./inferno.sh build agent
./inferno.sh build shared

# Test only (also builds first)
./inferno.sh test server
./inferno.sh test agent
./inferno.sh test shared

# Build + test + run
./inferno.sh run server
./inferno.sh run agent

# Wipe build cache for a component (forces full rebuild)
./inferno.sh clean server
./inferno.sh clean agent
./inferno.sh clean shared

# Clean + build in one step
./inferno.sh rebuild server
```

---

## If something is broken

**Tests fail on startup:**  
Read the output — it will tell you which test failed and why. Fix the code, save the file (the bind mount makes it live inside the container), then run `docker compose up` again. You don't need to rebuild the image.

**"ninja: no work to do" on startup:**  
That's normal — it means nothing changed since the last build. The containers will proceed to run tests and start.

**Port already in use:**  
Change `SERVER_PORT` in your `.env` to something free (e.g. `8889`) and restart.

**Something is very broken and I want to start fresh:**

```bash
docker compose down
rm -rf agent/build server/build shared/build
docker compose up
```

This wipes all build caches and starts from scratch. Images are kept — you don't need to re-run `docker compose build`.

---

## Project structure (quick reference)

```text
INFERNO/
├── agent/          → agent source code (runs as daemon on the target host)
│   ├── include/    → .hpp headers
│   ├── src/        → .cpp sources
│   └── tests/      → GTest unit tests
├── server/         → server source code (always runs in Docker)
│   ├── include/
│   ├── src/
│   └── tests/
├── shared/         → protocol, socket interface, shared utilities
│   ├── include/
│   ├── src/
│   └── tests/
├── dashboard/      → Qt client (separate setup, see dashboard/README.md)
├── docker-compose.yml
├── inferno.sh      → build/test/run script
└── .env.template   → copy this to .env and fill in values
```

---

## Adding new source files

When you create a new `.cpp` file, you must register it in the relevant `CMakeLists.txt`.

Example — you created `server/src/command_dispatcher.cpp`:

```cmake
# in server/CMakeLists.txt
add_library(server_lib STATIC
    src/placeholder.cpp
    src/command_dispatcher.cpp   ← add this line
)
```

Same rule applies for `agent/` and `shared/`. You don't need to touch the `CMakeLists.txt` for header-only files (`.hpp`).

After editing a `CMakeLists.txt`, the next build will automatically reconfigure. Nothing special to do.

---

## Windows-specific notes

Docker Desktop on Windows runs containers through WSL2 — everything works the same as Linux inside the container. The only difference is how you open a terminal:

- Use **PowerShell**, **Git Bash**, or **Windows Terminal**
- `docker compose up` / `docker compose down` work identically
- `docker exec -it inferno-server bash` works identically
- File edits in VS Code on Windows are reflected live inside the container (bind mount handles this)

> ⚠️ If you use WSL2 directly: clone the repo inside the WSL2 filesystem (`/home/yourname/...`), not inside `/mnt/c/...`. Bind mounts from Windows paths have known performance issues.
