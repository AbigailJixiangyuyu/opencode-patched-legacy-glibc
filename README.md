# OpenCode Patched Legacy GLIBC Builds

Automated legacy-GLIBC builds for a patched OpenCode source tree, with bundled musl libraries for systems with older GLIBC versions.

[![Build legacy-glibc package](https://github.com/AbigailJixiangyuyu/opencode-patched-legacy-glibc/actions/workflows/build.yml/badge.svg)](https://github.com/AbigailJixiangyuyu/opencode-patched-legacy-glibc/actions/workflows/build.yml)

## What is customized here

This repository intentionally does not build directly from upstream OpenCode releases.
It tracks:

- source repo: `https://github.com/AbigailJixiangyuyu/opencode`
- source branch: `patched-main`
- release tag format: `<UPSTREAM_TAG>-cjk-legacy-glibc`

The GitHub Actions workflow reads `UPSTREAM_TAG` from recent commits in the patched source repo, builds the legacy-compatible package, and publishes a matching release in this repository.

## Installation

1. Download the latest release from the [Releases](https://github.com/AbigailJixiangyuyu/opencode-patched-legacy-glibc/releases) page
2. Extract the tarball
3. Run `./opencode/bin/opencode`

## How it works

The packaged binary uses bundled musl runtime libraries so it can run on older GLIBC systems. A wrapper script prepares the musl loader path, and a small preload library clears `LD_PRELOAD` before child processes inherit it.

See `docs/TROUBLESHOOTING.md` for architecture notes, testing details, and runtime caveats.
