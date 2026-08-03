## Troubleshooting

### Segmentation Fault on Windows

**Cause:** Mismatched toolchain (e.g., part ucrt64 + part mingw64)

**Fix:**
1. Verify GCC version: `gcc --version`
   - Should show: `GNU 13.1.0` from Qt
   - NOT from another MinGW/MSYS2 installation

2. Check OpenSSL location:
```powershell
   Test-Path "C:\msys64\mingw64\lib\libssl.dll.a"  # Must be True
```

3. Rebuild from scratch:
```powershell
   rm -r dashboard/build
   ./dashboard/windows-build.bat
```

### CMake Can't Find OpenSSL

**The script automatically searches for OpenSSL.** If it fails:

1. Ensure MSYS2 MinGW64 is installed:
```bash
   # Open MSYS2 MinGW64 terminal and run:
   pacman -S mingw-w64-x86_64-openssl
```

2. Verify files exist:
```bash
   ls /mingw64/lib/libssl.dll.a
   ls /mingw64/include/openssl/ssl.h
```

3. Manual override (if needed):
   Edit `dashboard/windows-build.bat`, find the OpenSSL detection section, and set:
```batch
   set "OPENSSL_PATH=C:\msys64\mingw64"
```

### Build Fails: "Command not found"

Make sure you're building from:
- **Qt Creator**, OR
- **PowerShell** (not Git Bash), OR  
- **Qt's MinGW environment terminal**

NOT from plain CMD or Git Bash (they won't have Qt's tools in PATH).