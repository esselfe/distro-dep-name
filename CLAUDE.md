# CLAUDE.md - Developer Notes

This file contains architectural decisions, implementation notes, and guidance for working with this codebase.

## Project Overview

`distro-dep-name` is a C program that analyzes C/C++ source code to detect dependencies and queries remote VMs to find the exact package names for various Linux distributions. It then generates ready-to-run installation commands.

## Architecture

### Module Breakdown

```
src/
├── main.c          - CLI argument parsing, program flow orchestration
├── parser.c/h      - Source code and Makefile parsing
├── vm_query.c/h    - SSH-based VM querying and package resolution
├── distro.c/h      - Distribution metadata and configuration
└── output.c/h      - Output formatting (shell commands)
```

### Data Flow

1. **Input**: User provides source directory path and optional distro filters
2. **Parse**: `parser.c` recursively scans for `.c`, `.h`, `.cpp` files and Makefiles
3. **Extract**: Collects `#include` headers and `-l` linker flags
4. **Query**: `vm_query.c` SSH's into VMs and runs distro-specific package manager queries
5. **Output**: `output.c` formats results as shell installation commands

## Key Design Decisions

### 1. VM-Based Querying (Not Local Database)

**Decision**: Query actual VMs via SSH instead of maintaining a local package database.

**Rationale**:
- Always up-to-date with current package names
- Handles distro-specific variations naturally
- No database maintenance required
- Directly queries authoritative sources

**Trade-offs**:
- Slower (requires SSH connection per distro)
- Requires VM infrastructure
- Dependent on network connectivity

### 2. Environment Variable Configuration

**Pattern**: VMs configured via `DISTRO_VM_<NAME>` environment variables.

**Example**:
```bash
export DISTRO_VM_DEBIAN="user@192.168.1.10"
export DISTRO_VM_ARCH="user@arch-vm.local"
```

**Rationale**:
- Simple configuration without config files
- Easy to script and automate
- Standard Unix pattern
- No parsing complexity

### 3. Heuristic Header-to-Library Mapping

**Location**: `vm_query.c:header_to_library()`

**Approach**: Hardcoded mappings for common libraries:
- `ssl/openssl` → `ssl`
- `curl` → `curl`
- `pthread` → `pthread`
- `z.h` → `z`

**Limitation**: Not comprehensive, may miss less common libraries.

**Future Enhancement**: Could integrate `pkg-config` queries on VMs for better accuracy.

### 4. Distro-Specific Query Commands

**Location**: `vm_query.c:query_dependency()`

Each distro has tailored commands:

- **Debian/Ubuntu**: `apt-cache search --names-only 'lib<name>.*-dev'`
- **Arch**: `pacman -Ss '^<name>$'`
- **Alpine**: `apk search -v <name>-dev`
- **Gentoo**: `emerge -s '^<name>$'`
- **openSUSE**: `zypper search --match-exact -t package <name>-devel`

**Design Note**: These commands are brittle and may need updates if package manager output formats change.

## Implementation Details

### Memory Management

- All dynamic allocations use standard `malloc`/`realloc`/`free`
- Each module provides `free_*` functions for cleanup
- No memory pooling or custom allocators

**Critical Sections**:
- `parser.c`: Growing arrays for dependencies
- `vm_query.c`: SSH command output buffers (MAX_OUTPUT_LEN = 8KB)

### Error Handling

Currently minimal:
- NULL checks on allocations
- File open failures are silently skipped
- SSH failures print warnings but continue

**Improvement Needed**: More robust error propagation and user feedback.

### Parsing Strategy

**Source Files**:
- Line-by-line reading with `fgets()`
- Simple string matching for `#include`
- Supports both `<header.h>` and `"header.h"`

**Makefiles**:
- String search for `-l` patterns
- Extracts alphanumeric library names
- Does not parse Make syntax properly (fragile)

**Limitations**:
- No preprocessor macro expansion
- No conditional include detection
- Won't handle complex Makefile variables

### SSH Execution

**Function**: `execute_ssh_command()` in `vm_query.c`

**Implementation**:
```c
ssh -o ConnectTimeout=5 -o StrictHostKeyChecking=no <host> '<command>' 2>/dev/null
```

**Security Considerations**:
- Assumes key-based authentication
- `StrictHostKeyChecking=no` is insecure for production
- Command injection possible if library names aren't sanitized

**TODO**: Add proper command escaping and security hardening.

## Common Patterns

### Adding a New Distribution

1. Add enum to `distro.h:distro_type_t`
2. Add entry to `distros[]` array in `distro.c`
3. Add query command case in `vm_query.c:query_dependency()`
4. Document in README.md

### Improving Header Mapping

Extend `header_to_library()` in `vm_query.c`:

```c
if (strstr(header, "yourlib")) return "yourlib";
```

For systematic approach, consider:
- Parsing `pkg-config` `.pc` files on VMs
- Building a lookup table from system headers
- Using distro-specific package file indices

### Adding Build System Support

Create new parsing functions in `parser.c`:

```c
static void parse_cmake_file(const char *filepath, dependency_list_t *list);
static void parse_meson_file(const char *filepath, dependency_list_t *list);
```

Then call from `scan_directory()`.

## Testing Strategy

### Manual Testing Workflow

1. Create test project with known dependencies
2. Set up VM environment variables
3. Run: `./distro-dep-name test-project/`
4. Verify output commands are correct
5. Test commands on actual VMs

### Test Cases Needed

- [ ] Simple C project with standard libraries
- [ ] C++ project with STL
- [ ] Project with SSL/crypto dependencies
- [ ] Project with database libraries (sqlite, mysql, postgres)
- [ ] Project with no dependencies
- [ ] Malformed Makefiles
- [ ] Missing VM configurations

## Known Issues

1. **CMake/Meson not supported**: Only Makefiles are parsed
2. **Incomplete header mappings**: Many headers won't map correctly
3. **No deduplication across files**: Same dependency may be queried multiple times
4. **SSH timeout handling**: 5-second timeout may be too short for slow networks
5. **No progress indicators**: User has no feedback during long queries
6. **Security**: Command injection vulnerabilities exist

## Future Improvements

### Short Term
- Add CMake `target_link_libraries()` parsing
- Improve error messages and user feedback
- Add progress indicators for VM queries
- Better handling of missing VMs (skip gracefully)

### Medium Term
- Add local caching of query results
- Implement pkg-config integration
- Support for Python/Rust/Go dependency detection
- Parallel VM queries for speed

### Long Term
- Pre-built package database mode (no VMs needed)
- Web service for centralized queries
- Integration with GitHub Actions
- GUI frontend

## Development Tips

### Building
```bash
make              # Build project
make clean        # Clean build artifacts
make install      # Install to /usr/local/bin
```

### Debugging
```bash
# Compile with debug symbols
make CFLAGS="-Wall -Wextra -g -O0 -std=c11"

# Run with gdb
gdb ./distro-dep-name
```

### Testing SSH Queries Manually
```bash
# Test Debian query
ssh user@debian-vm "apt-cache search --names-only 'libssl.*-dev'"

# Test Arch query
ssh user@arch-vm "pacman -Ss '^openssl$' | grep -v '^ '"
```

### Adding Debug Output
Add to relevant functions:
```c
#ifdef DEBUG
fprintf(stderr, "DEBUG: Processing dependency: %s\n", dep->name);
#endif
```

Compile with: `make CFLAGS="-DDEBUG -g"`

## Code Style

- **Indentation**: 4 spaces (no tabs)
- **Braces**: K&R style
- **Naming**: `snake_case` for functions and variables
- **Headers**: Include guards with `#ifndef HEADER_H`
- **Memory**: Always pair `malloc` with `free`
- **Errors**: Print to `stderr`, not `stdout`

## Contributing Guidelines

When modifying this codebase:

1. **Test on multiple distros** before committing
2. **Update README.md** if changing user-facing behavior
3. **Update this file** if changing architecture
4. **Check for memory leaks** with valgrind
5. **Verify SSH commands** don't break on edge cases

## Resources

- Package manager documentation:
  - [apt-cache man page](https://manpages.debian.org/apt-cache)
  - [pacman man page](https://archlinux.org/pacman/pacman.8.html)
  - [apk documentation](https://wiki.alpinelinux.org/wiki/Alpine_Package_Keeper)
  - [emerge documentation](https://wiki.gentoo.org/wiki/Emerge)
  - [zypper man page](https://en.opensuse.org/Portal:Zypper)

- Relevant tools:
  - [pkg-config](https://www.freedesktop.org/wiki/Software/pkg-config/)
  - [repology.org](https://repology.org/) - Cross-distro package search

---

*Generated with Claude Code - Last updated: 2025-11-08*
