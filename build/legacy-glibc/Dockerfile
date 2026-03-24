FROM oven/bun:1 AS build

ARG REPO_URL
ARG REF

WORKDIR /build

RUN apt-get update && apt-get install -y git python3 make g++ patchelf file

RUN git clone --depth 1 --branch "${REF}" "${REPO_URL}" .

RUN bun install
RUN ./packages/opencode/script/build.ts

RUN test -f /build/packages/opencode/dist/opencode-linux-x64-baseline-musl/bin/opencode

FROM alpine:latest AS runtime_libs
RUN apk add --no-cache libstdc++ patchelf

FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y patchelf file && rm -rf /var/lib/apt/lists/*

WORKDIR /opencode

COPY --from=build /build/packages/opencode/dist/opencode-linux-x64-baseline-musl/bin/opencode /opencode/lib/opencode
COPY --from=runtime_libs /lib/ld-musl-x86_64.so.1 /opencode/lib/ld-musl-x86_64.so.1
COPY --from=runtime_libs /usr/lib/libstdc++.so.6 /opencode/lib/libstdc++.so.6
COPY --from=runtime_libs /lib/libgcc_s.so.1 /opencode/lib/libgcc_s.so.1

COPY build/legacy-glibc/opencode /opencode/bin/opencode
COPY build/legacy-glibc/opencode.bin /opencode/bin/opencode.bin
COPY build/legacy-glibc/clear_ldpath.c /tmp/clear_ldpath.c

RUN apt-get update && apt-get install -y gcc && rm -rf /var/lib/apt/lists/* \
    && mkdir -p /opencode/bin /tmp/etc /tmp/.opencode-ld \
    && gcc -shared -fPIC -O2 -o /opencode/lib/clear_ldpath.so /tmp/clear_ldpath.c \
    && chmod +x /opencode/bin/opencode /opencode/bin/opencode.bin \
    && patchelf --set-interpreter /tmp/.opencode-ld/ld-musl-x86_64.so.1 /opencode/lib/opencode \
    && file /opencode/lib/opencode

WORKDIR /output
CMD ["/bin/bash"]