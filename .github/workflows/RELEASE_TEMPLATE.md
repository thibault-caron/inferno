# Inferno v${VERSION}

Released: ${RELEASE_DATE}

## Downloads

### Agent
- **Linux**: `inferno-agent-linux-v${VERSION}.AppImage`
- **Windows**: `inferno-agent-windows-v${VERSION}.zip`

### Dashboard
- **Linux**: `inferno-dashboard-linux-v${VERSION}.AppImage`
- **Windows**: `inferno-dashboard-windows-v${VERSION}.zip`

### Server
```bash
docker pull ghcr.io/your-org/inferno-server:${VERSION}
```
---

## Quick Start

### 1. Start the Server & Database

```bash
docker compose up
```

This starts:
- TimescaleDB (port 5432)
- Inferno Server (port 8888)

### 2. Run the Agent

**Linux:**
```bash
chmod +x inferno-agent-linux-v${VERSION}.AppImage
SERVER_HOST=localhost SERVER_PORT=8888 ./inferno-agent-linux-v${VERSION}.AppImage
```

**Windows:**
```powershell
cd inferno-agent-windows-v${VERSION}
set SERVER_HOST=localhost
set SERVER_PORT=8888
agent.exe
```

### 3. Environment Variables

**Required:**
- `SERVER_HOST` — Server address (default: `localhost`)
- `SERVER_PORT` — Server port (default: `8888`)

**Optional:**
- `TLS` — Enable TLS (default: `true`)
```bash
  TLS=true ./inferno-agent-linux-v${VERSION}.AppImage
```

**TLS Note:** If `TLS=true`, the agent looks for CA certificate at:
1. `$APPDIR/usr/share/inferno/ca.crt` (Linux AppImage, bundled)
2. `./certs/ca.crt` (development)

On Windows, ensure `ca.crt` is in the same folder as `agent.exe`.

### 4. Run the Dashboard (GUI)

```bash
./inferno-dashboard-linux-v${VERSION}.AppImage
```

or

```powershell
.\inferno-dashboard-windows-v${VERSION}\dashboard.exe
```

---

## What's New

See [CHANGELOG.md](../../CHANGELOG.md) for details.

---

## Documentation

- [Architecture & Design](../../_docs/)
- [Protocol Specification](../../_docs/project/lptf_binary_protocol.md)
- [Building from Source](../../README.md)

---

## Troubleshooting

**Agent won't connect (TLS enabled)?**
- Verify `ca.crt` is present (bundled in Linux AppImage, add manually on Windows)
- Check `TLS=true` env var is set
- Verify server is running with matching TLS config

**Server won't start?**
- Ensure Docker is running
- Check port 5432 (database) and 8888 (server) are available