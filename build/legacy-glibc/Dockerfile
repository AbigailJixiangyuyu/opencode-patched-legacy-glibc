# Build on linux/amd64 to ensure correct baseline binary
ARG BUILD_PLATFORM=linux/amd64

FROM --platform=$BUILD_PLATFORM oven/bun:debian AS builder

ARG REPO_URL
ARG REF

RUN apt-get update && apt-get install -y git patchelf && rm -rf /var/lib/apt/lists/*

WORKDIR /build

RUN git clone --depth 1 --branch "${REF}" "${REPO_URL}" .

RUN bun install
RUN ./packages/opencode/script/build.ts

# Patch the binary to use a custom interpreter path that we'll symlink at runtime
RUN patchelf --set-interpreter /tmp/.opencode-ld/ld-musl-x86_64.so.1 \
  /build/packages/opencode/dist/opencode-linux-x64-baseline-musl/bin/opencode

# Get musl and libs from Alpine
FROM --platform=$BUILD_PLATFORM alpine:latest AS musl
RUN apk add --no-cache musl libstdc++ libgcc

FROM alpine:latest AS wrapper
COPY build/legacy-glibc/opencode /opencode/bin/opencode
COPY build/legacy-glibc/opencode.bin /opencode/bin/opencode.bin
RUN chmod +x /opencode/bin/opencode /opencode/bin/opencode.bin

# Build clear_ldpath.so: self-contained shared library with no dynamic deps
FROM --platform=$BUILD_PLATFORM alpine:latest AS ldpreload
RUN apk add --no-cache gcc musl-dev
COPY build/legacy-glibc/clear_ldpath.c /tmp/
RUN mkdir -p /opencode/lib && gcc -shared -fPIC -nostdlib \
  -o /opencode/lib/clear_ldpath.so /tmp/clear_ldpath.c

FROM scratch AS export
COPY --from=builder /build/packages/opencode/dist/opencode-linux-x64-baseline-musl/bin/opencode /opencode/lib/opencode
COPY --from=musl /lib/ld-musl-x86_64.so.1 /opencode/lib/
COPY --from=musl /usr/lib/libstdc++.so.6 /opencode/lib/
COPY --from=musl /usr/lib/libgcc_s.so.1 /opencode/lib/
COPY --from=ldpreload /opencode/lib/clear_ldpath.so /opencode/lib/
COPY --from=wrapper /opencode/bin/opencode /opencode/bin/opencode
COPY --from=wrapper /opencode/bin/opencode.bin /opencode/bin/opencode.bin