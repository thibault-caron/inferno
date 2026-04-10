#!/bin/bash
set -e

# Absolute path to the directory containing this script.
# All paths derive from here — works regardless of where the script is called from.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ==================== COLORS ====================
BLUE='\033[0;34m'
GREEN='\033[0;32m'
RED='\033[0;31m'
YELLOW='\033[1;33m'
RESET='\033[0m'

title()   { echo -e "\n${BLUE}========================================${RESET}"; echo -e "${BLUE}  $1${RESET}"; echo -e "${BLUE}========================================${RESET}"; }
success() { echo -e "${GREEN}✅ $1${RESET}"; }
failure() { echo -e "${RED}❌ $1${RESET}"; }
info()    { echo -e "${YELLOW}➜  $1${RESET}"; }

# ==================== PATH HELPERS ====================

# Source directory for each target (where CMakeLists.txt lives)
source_dir() {
    case $1 in
        agent)  echo "$SCRIPT_DIR/agent" ;;
        server) echo "$SCRIPT_DIR/server" ;;
        shared) echo "$SCRIPT_DIR/shared" ;;
        all)    echo "$SCRIPT_DIR" ;;
    esac
}

# Build output directory for each target
build_dir() {
    case $1 in
        agent)  echo "$SCRIPT_DIR/agent/build" ;;
        server) echo "$SCRIPT_DIR/server/build" ;;
        shared) echo "$SCRIPT_DIR/shared/build" ;;
        all)    echo "$SCRIPT_DIR/build" ;;
    esac
}

# CMake target name (what --target expects)
cmake_target() {
    case $1 in
        agent)  echo "agent" ;;
        server) echo "server" ;;
        shared) echo "shared_lib" ;;
        all)    echo "" ;;
    esac
}

# ctest filter name
test_target() {
    case $1 in
        agent)  echo "agent_tests" ;;
        server) echo "server_tests" ;;
        shared) echo "shared_tests" ;;
        all)    echo "" ;;
    esac
}

# ==================== USAGE ====================
usage() {
    echo -e "
${BLUE}INFERNO build script${RESET}

Usage: ./inferno.sh <command> [target]

${YELLOW}Commands:${RESET}
  build   [target]    Build one or all targets
  test    [target]    Build then run tests
  run     <target>    Build then run executable (agent or server)
  clean   [target]    Delete build directory
  rebuild [target]    Clean then build

${YELLOW}Targets:${RESET}
  all                 Everything (default when no target given)
  agent               Agent only
  server              Server only
  shared              Shared library only

${YELLOW}Examples:${RESET}
  ./inferno.sh build
  ./inferno.sh build agent
  ./inferno.sh test
  ./inferno.sh test server
  ./inferno.sh run agent
  ./inferno.sh clean
  ./inferno.sh clean agent
  ./inferno.sh rebuild agent
"
    exit 1
}

# ==================== COMMANDS ====================
cmd_build() {
    local target="${1:-all}"
    local src build

    # Always build shared first if building a component
    if [ "$target" != "shared" ] && [ "$target" != "all" ]; then
        local shared_build
        shared_build=$(build_dir "shared")
        if [ ! -f "$shared_build/build.ninja" ]; then
            info "Configuring shared..."
            cmake -S "$(source_dir shared)" -B "$shared_build" -G Ninja
        fi
        cmake --build "$shared_build" -j"$(nproc)"
    fi

    src=$(source_dir "$target")
    build=$(build_dir "$target")

    if [ ! -f "$build/build.ninja" ]; then
        info "Configuring $target..."
        cmake -S "$src" -B "$build" -G Ninja
    fi

    title "Building: $target"
    cmake --build "$build" -j"$(nproc)"   # ← no --target, builds everything including tests

    success "Build complete!"
}

cmd_test() {
    local target="${1:-all}"

    cmd_build "$target"

    # Always test shared — it's a dependency of everything
    if [ "$target" != "shared" ] && [ "$target" != "all" ]; then
        title "Testing: shared"
        ctest --test-dir "$(build_dir shared)" --output-on-failure
    fi

    title "Testing: $target"
    local test_t
    test_t=$(test_target "$target")

    if [ -z "$test_t" ]; then
        ctest --test-dir "$(build_dir "$target")" --output-on-failure
    else
        ctest --test-dir "$(build_dir "$target")" -R "$test_t" --output-on-failure
    fi

    success "All tests passed!"
}

cmd_run() {
    local target="${1:-}"

    case "$target" in
        server|agent)
            cmd_test "$target"   # ← was cmd_build, now tests run first
            local bin
            bin="$(build_dir "$target")/bin/$target"
            title "Running: $target"
            "$bin"
            ;;
        *)
            failure "'run' requires a specific target: agent or server"
            exit 1
            ;;
    esac
}

cmd_clean() {
    local target="${1:-all}"
    local build
    build=$(build_dir "$target")

    title "Cleaning: $target"
    rm -rf "$build"
    success "Cleaned $build"
}

cmd_rebuild() {
    cmd_clean "${1:-all}"
    cmd_build "${1:-all}"
}

# ==================== ENTRY POINT ====================
COMMAND="${1:-}"
TARGET="${2:-all}"

case "$COMMAND" in
    build)   cmd_build   "$TARGET" ;;
    test)    cmd_test    "$TARGET" ;;
    run)     cmd_run     "$TARGET" ;;
    clean)   cmd_clean   "$TARGET" ;;
    rebuild) cmd_rebuild "$TARGET" ;;
    *)       usage ;;
esac