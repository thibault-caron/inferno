FROM debian:bookworm-20260406-slim AS builder

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y --no-install-recommends \
    # Compiler & build
    g++ \
    cmake \
    make \
    ninja-build \
    # Google Test (system package, avoids FetchContent needing internet in container)
    libgtest-dev \
    libgmock-dev \
    # Debug & analysis
    gdb \
    valgrind \
    strace \
    file \
    # Quality of life
    git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

CMD ["bash"]

# ── Stage 2 : runtime (uncomment when you have a binary to ship) ───────────
# FROM debian:bookworm-20260406-slim AS runtime
# WORKDIR /app
# COPY --from=builder /app/build/agent . 
# or # COPY --from=builder /app/build/server .
# EXPOSE 4444
# CMD ["./agent"] or CMD ["./server"]