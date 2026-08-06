# LPTF — Development Certificates

## ⚠️ Warning

These certificates are **self-signed and committed intentionally** for development
convenience. They are not secret in this repository.

**NEVER use these in a production deployment.**  
For any real deployment, generate fresh certificates and never commit private keys.

---

## What is in this folder

| File | Used by | Purpose |
|---|---|---|
| `ca.crt` | server + agent | The Certificate Authority both sides trust |
| `ca.key` | nobody at runtime | Only needed to sign new certificates |
| `server.crt` | server | Proves the server's identity to agents |
| `server.key` | server | Server private key, kept secret at runtime |
| `ca.srl` | nobody | Serial tracker used by OpenSSL internally. Do not delete. |

The agent never has its own certificate — it only loads `ca.crt` to verify
that whoever it connects to is a server signed by this CA.
This prevents any rogue server from issuing commands to the agent.

---

## How to generate

Run these commands from the `certs/` directory.

### All platform
#### On windows
OpenSSL must be installed first. If it is not:
```powershell
winget install ShiningLight.OpenSSL
```

```bash
# Step 1 — Create the Certificate Authority
openssl genrsa -out ca.key 4096
openssl req -new -x509 -days 3650 -key ca.key -out ca.crt -subj "/CN=LPTF-Dev-CA"

# Step 2 — Create the server certificate, signed by the CA
openssl genrsa -out server.key 2048
openssl req -new -key server.key -out server.csr -subj "/CN=lptf-server"
openssl x509 -req -days 3650 -in server.csr -CA ca.crt -CAkey ca.key -CAcreateserial -out server.crt


```
### Linux / macOS
```bash
# Step 3 — Clean up
rm server.csr
```

### Windows (PowerShell)
```powershell
# Step 3 — Clean up
Remove-Item server.csr
```

---

## Why self-signed

A real CA (like Let's Encrypt) would require a public domain name and renewal.
For a local development and testing environment, a self-signed CA is standard
practice — it provides real TLS encryption and server authentication,
just without a third-party trust chain.