# Troubleshooting & Technical Notes

## Architecture Overview

This repository repackages a patched OpenCode source tree so it can run on systems with old GLIBC
(for example QNAP NAS devices). It replaces the host GLIBC dependency with bundled musl runtime
libraries while preserving the customized upstream source selection used by this fork.

### Source selection in this fork

The release workflow builds from:

- `SOURCE_REPO=https://github.com/AbigailJixiangyuyu/opencode.git`
- `SOURCE_BRANCH=patched-main`

The workflow extracts `UPSTREAM_TAG` from recent commit messages in that source repository and
publishes releases here as `<UPSTREAM_TAG>-cjk-legacy-glibc`.

### Wrapper chain

```
bin/opencode            (outer wrapper - creates musl loader symlink + .path file)
  -> bin/opencode.bin   (inner wrapper - sets LD_PRELOAD=clear_ldpath.so, execs real binary)
       -> lib/opencode  (patched ELF binary using musl dynamic linker)
```

When OpenCode re-invokes itself via `process.execPath` during plugin installation:

```
lib/opencode            (called directly - musl linker finds libs via .path file)
```

No wrapper or `LD_LIBRARY_PATH` is required for re-invocation. The musl linker reads
`/tmp/etc/ld-musl-x86_64.path` to find bundled libraries.

### Key files

| File | Purpose |
|------|---------|
| `bin/opencode` | Outer shell wrapper - creates musl loader symlink + `.path` file |
| `bin/opencode.bin` | Inner shell wrapper - sets `LD_PRELOAD`, execs the real binary |
| `lib/opencode` | Patched OpenCode ELF binary |
| `lib/ld-musl-x86_64.so.1` | musl dynamic linker from Alpine |
| `lib/libstdc++.so.6` | C++ standard library from Alpine |
| `lib/libgcc_s.so.1` | GCC support library from Alpine |
| `lib/clear_ldpath.so` | Self-contained `LD_PRELOAD` library with no dynamic deps |

### Runtime files created by `bin/opencode`

| File | Purpose |
|------|---------|
| `/tmp/.opencode-ld/ld-musl-x86_64.so.1` | Symlink to `lib/ld-musl-x86_64.so.1` used as the ELF interpreter target |
| `/tmp/etc/ld-musl-x86_64.path` | Contains the absolute `lib/` path used by musl library resolution |

## How musl Finds Libraries

The musl dynamic linker resolves its `.path` file relative to its grandparent directory. Since the
ELF interpreter is patched to `/tmp/.opencode-ld/ld-musl-x86_64.so.1`, the grandparent is `/tmp`,
so the linker reads `/tmp/etc/ld-musl-x86_64.path`.

This is musl-specific and invisible to glibc children. That avoids the old `LD_LIBRARY_PATH`
catch-22 where musl binaries needed extra search paths but glibc subprocesses could break when they
inherited them.

## Plugin Installation Mechanism

OpenCode installs plugins by re-invoking itself with `BUN_BE_BUN=1`:

```text
process.execPath add --force --exact --cwd ~/.cache/opencode <pkg>@<version>
```

- `process.execPath` resolves via `/proc/self/exe` to `lib/opencode`, the real binary
- the re-invoked binary finds its musl libraries through the `.path` file
- `BUN_BE_BUN=1` tells the compiled Bun binary to behave like the `bun` CLI
- `OPENCODE_DISABLE_DEFAULT_PLUGINS=true` can be used to skip built-in npm plugins when debugging

## Environment Sanitization

`clear_ldpath.so` is compiled with `-nostdlib` to produce a self-contained shared object with zero
dynamic dependencies. Its only job is to clear `LD_PRELOAD` from the environment so that the shared
object does not break glibc-linked child processes.

`LD_LIBRARY_PATH` is never set. The musl dynamic linker uses the `.path` file instead.

### Why `-nostdlib` is required

Bun caches `process.env` at startup and uses that cache, not the C `environ`, when spawning child
processes via `execve`. That means `LD_PRELOAD` can leak to glibc children even if the constructor
removes it from the C environment. If `clear_ldpath.so` depended on musl libc, glibc children would
fail trying to load `libc.musl-x86_64.so.1`. Building with `-nostdlib` avoids that problem.

## Known Issues & Gotchas

### 1. `patchelf --set-rpath` can corrupt Bun binaries

Do not use `--set-rpath` on the large Bun-based binary. Patch only the interpreter path.

### 2. Docker testing should use `--platform linux/amd64`

This build targets x86_64 systems. On arm64 hosts, use `--platform linux/amd64` for both build and
test containers.

### 3. Archived distro repositories need adjustments

- Debian Stretch uses `archive.debian.org`
- Debian Buster uses `archive.debian.org`
- CentOS 7 uses `vault.centos.org`

The included test Dockerfiles already handle those repository changes.

### 4. `process.execPath` resolves through `/proc/self/exe`

Bun resolves `process.execPath` to the final exec'd binary, not to shell wrappers or `argv[0]`.
That is why the `.path` file approach is necessary.

## Testing

### Build the artifact locally

```bash
docker build \
  -f build/legacy-glibc/Dockerfile \
  --target export \
  --build-arg REPO_URL=https://github.com/AbigailJixiangyuyu/opencode.git \
  --build-arg REF=patched-main \
  --output type=local,dest=out \
  .
```

### Run the deterministic compatibility suite

The test script at `build/test/test-env.sh` covers wrapper startup, direct binary execution,
plugin install, process re-invocation, `LD_PRELOAD` safety, and basic child-process compatibility.

```bash
docker build --platform linux/amd64 -t opencode-test -f build/test/Dockerfile .
docker run --platform linux/amd64 --rm opencode-test sh /opt/test-env.sh

docker build --platform linux/amd64 -t opencode-test-buster -f build/test/Dockerfile.buster .
docker run --platform linux/amd64 --rm opencode-test-buster sh /opt/test-env.sh

docker build --platform linux/amd64 -t opencode-test-centos7 -f build/test/Dockerfile.centos7 .
docker run --platform linux/amd64 --rm opencode-test-centos7 sh /opt/test-env.sh
```
