# CVE-2025-7771 – ThrottleStop.sys Privilege Escalation

**CVE-2025-7771** | **CVSS 8.7** | **Local Privilege Escalation**

## Overview

ThrottleStop.sys exposes unrestricted IOCTL interfaces allowing arbitrary physical memory access through `MmMapIoSpace`. This enables local privilege escalation by patching kernel memory and executing arbitrary code in ring-0 context.

**Affected Component**: ThrottleStop.sys (TechPowerUp)  
**Vulnerability Type**: Exposed IOCTL with Insufficient Access Control (CWE-782)  
**Impact**: Local Privilege Escalation  
**Driver Status**: Digitally signed, compatible with Core Isolation

## Technical Details

### Vulnerable IOCTLs

| IOCTL | Function |
|-------|----------|
| `0x80006498` | Read Physical Memory |
| `0x8000649C` | Write Physical Memory |

### Exploitation Method

1. Open handle to `\\Device\\ThrottleStop`
2. Use `DeviceIoControl` with vulnerable IOCTLs
3. Map arbitrary physical memory via `MmMapIoSpace`
4. Patch kernel structures or function pointers
5. Execute arbitrary code in kernel context

## Proof of Concept

This implementation demonstrates exploitation of the ThrottleStop.sys vulnerability through:

- Driver handle acquisition via `CreateFileW`
- Process enumeration and targeting
- Physical memory read/write operations using vulnerable IOCTLs
- Template-based memory operations for different data types
- Buffer operations for larger memory regions

## Compilation & Usage

### Build

```bash
msbuild throttlestop.sln /p:Configuration=Release /p:Platform=x64
```

### Driver Service Setup

```cmd
# Create service
sc create ThrottleStop binPath= "C:\path\to\ThrottleStop.sys" type= kernel

# Start service
sc start ThrottleStop
```

### Execution

```bash
.\usermode\usermode.exe
```

**Requirements**: Administrator privileges, Visual Studio 2019/2022, ThrottleStop.sys driver service

## Code Structure

```
throttlestop/
├── usermode/
│   ├── Entry.cpp              # Main application entry point
│   ├── Driver/
│   │   └── Driver.h           # MemoryDriver class implementation
│   └── usermode.vcxproj       # Visual Studio project file
├── ThrottleStop.sys           # Vulnerable driver binary
└── throttlestop.sln           # Solution file
```

### Implementation Details

- **`MemoryDriver` Class**: Driver communication and memory operations
- **IOCTL Handlers**: `0x80006498` (read), `0x8000649C` (write)
- **Process Targeting**: Automatic process enumeration and selection
- **Memory Operations**: Template-based read/write with buffer support

### Driver Certificates

**Digital Signatures:**
- TechPowerUp LLC (SHA1)
- Microsoft Windows Hardware Compatibility Publisher (SHA256)
- TechPowerUp LLC (SHA1)

**Security Implications:**
- Driver bypasses Driver Signature Enforcement (DSE)
- Compatible with Core Isolation enabled systems
- Trusted by Windows as legitimate hardware driver
- No unsigned driver warnings or blocks

## Mitigation

- Remove ThrottleStop.sys if not required
- Block unsigned/unverified drivers
- Enable Kernel-mode Code Integrity (KMCI)
- Monitor for access to `\\Device\\ThrottleStop`
- Detect DeviceIoControl calls with IOCTL codes `0x80006498` and `0x8000649C`

## References

- [INCIBE-CERT Advisory on CVE-2025-7771](https://www.incibe.es/en/incibe-cert/early-warning/vulnerabilities/cve-2025-7771)
- [Kaspersky Analysis of ThrottleStop.sys Exploitation](https://securelist.com/av-killer-exploiting-throttlestop-sys/117026/)
- [Microsoft Driver Security Best Practices](https://learn.microsoft.com/en-us/windows-hardware/drivers/driversecurity/driver-security-dev-best-practices)

## Analysis Images

### IOCTL Handler Analysis

**Read Physical Memory (0x80006498)**
![IDA Analysis - Read IOCTL](https://i.imgur.com/8QuzuJ1.png)

**Write Physical Memory (0x8000649C)**
![IDA Analysis - Write IOCTL](https://i.imgur.com/yYPmY7W.png)

These screenshots show the vulnerable IOCTL handlers in ThrottleStop.sys that allow unrestricted physical memory access through `MmMapIoSpace`.

## Disclaimer

This repository is provided for educational and defensive research purposes only. Any unauthorized use to compromise systems is illegal and prohibited. Use at your own risk in controlled environments only.
