# distro-dep-name

A tool to analyze C/C++ source code and generate distribution-specific dependency installation commands by querying package managers on remote VMs.

```
This project is in alpha stage and is currently not fully functional as of 2025-11-09.
```

## Features

- Parses C/C++ source files for `#include` directives
- Parses Makefiles for `-l` linker flags
- Queries package managers on remote VMs to find exact package names
- Supports multiple Linux distributions:
  - Debian
  - Ubuntu
  - Arch Linux
  - Alpine Linux
  - Gentoo
  - openSUSE
- Generates ready-to-run installation commands

## Building

```bash
make
```

## Configuration

The tool connects to VMs via SSH to query package managers. Configure VM hostnames using environment variables:

```bash
export DISTRO_VM_DEBIAN="user@debian-vm"
export DISTRO_VM_UBUNTU="user@ubuntu-vm"
export DISTRO_VM_ARCH="user@arch-vm"
export DISTRO_VM_ALPINE="user@alpine-vm"
export DISTRO_VM_GENTOO="user@gentoo-vm"
export DISTRO_VM_OPENSUSE="user@opensuse-vm"
```

**Important**: Environment variable names must be uppercase (e.g., `DISTRO_VM_DEBIAN`).

**Requirements:**
- SSH access to VMs with key-based authentication (no password)
- Package manager tools installed on each VM
- Proper SSH configuration in `~/.ssh/config` for timeouts and host verification (v0.0.3+)

## Usage

### Analyze a project and query all distros

```bash
./distro-dep-name /path/to/source
```

### Query specific distro(s)

```bash
./distro-dep-name -d debian /path/to/source
./distro-dep-name -d debian -d ubuntu /path/to/source
```

### List supported distros

```bash
./distro-dep-name -l
```

### Enable debug output (v0.0.3+)

```bash
./distro-dep-name -D /path/to/source
```

Shows verbose output including:
- Files being parsed
- Dependencies found
- SSH commands executed

### Help

```bash
./distro-dep-name -h
```

### Version

```bash
./distro-dep-name -V
```

## Example Output

```
Analyzing source code at: /path/to/project
Found 5 dependencies

Querying debian packages...
Querying ubuntu packages...
Querying arch packages...

## Dependency Installation Commands

### debian
apt install -y libssl-dev libcurl4-openssl-dev libsqlite3-dev

### ubuntu
apt install -y libssl-dev libcurl4-openssl-dev libsqlite3-dev

### arch
pacman -S --noconfirm openssl curl sqlite
```

## How It Works

1. **Source Parsing**: Recursively scans the source directory for C/C++ files and Makefiles
2. **Dependency Extraction**:
   - Extracts header files from `#include` statements
   - Extracts library names from `-l` flags in Makefiles
3. **VM Query**: For each distro:
   - Connects via SSH to the configured VM
   - Runs distro-specific package manager commands to find packages
   - Parses the output to extract package names
4. **Output Generation**: Formats the results as installation commands

## VM Setup

Each VM should have:

1. **SSH Server** configured with key-based authentication
2. **Package Manager** installed and updated:
   - Debian/Ubuntu: `apt-cache`
   - Arch: `pacman`
   - Alpine: `apk`
   - Gentoo: `emerge`, `eix`
   - openSUSE: `zypper`

Example VM setup for Debian:
```bash
# On your host machine
ssh-keygen -t ed25519
ssh-copy-id user@debian-vm

# Test connection
ssh user@debian-vm "apt-cache search libssl"
```

## Limitations

- Header-to-library mapping uses heuristics and may not be perfect
- Requires SSH access to VMs (can be resource-intensive)
- Some dependencies may need manual verification
- Only supports C/C++ projects currently

## Future Enhancements

- Support for CMake, Meson, and other build systems
- Pre-built package database mode (no VM required)
- Support for other languages (Python, Rust, Go, etc.)
- Web service mode for querying without local VMs
- Better header-to-package mapping with pkg-config integration
- Support for using Docker instead of VMs.

## License

GPL-3.0 (GNU General Public License v3.0)
