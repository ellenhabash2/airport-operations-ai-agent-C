# syntax=docker/dockerfile:1

##############################################################################
# Build stage
##############################################################################
FROM ubuntu:24.04 AS builder

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    cmake \
    git \
    ca-certificates \
    pkg-config \
    libpq-dev \
    libpqxx-dev \
    libjsoncpp-dev \
    libssl-dev \
    zlib1g-dev \
    libuv1-dev \
    libpcre3-dev \
    libc-ares-dev \
    uuid-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . /build/

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release \
    && cmake --build build -j"$(nproc)" \
    && cmake --install build

RUN mkdir -p /runtime-deps \
    && ldd /usr/local/bin/aeromind_backend \
       | awk '/=> \// { print $3 }' \
       | sort -u \
       | xargs -I '{}' cp -L '{}' /runtime-deps/

##############################################################################
# Runtime stage
##############################################################################
FROM ubuntu:24.04 AS runtime

RUN apt-get update && apt-get install -y --no-install-recommends \
    curl \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

COPY --from=builder /usr/local/bin/aeromind_backend /app/aeromind_backend
COPY --from=builder /runtime-deps/ /usr/local/lib/aeromind/

RUN echo "/usr/local/lib/aeromind" > /etc/ld.so.conf.d/aeromind.conf \
    && ldconfig \
    && mkdir -p /app/logs

WORKDIR /app
EXPOSE 8848

HEALTHCHECK --interval=30s --timeout=10s --start-period=40s --retries=3 \
    CMD curl -f http://localhost:8848/health || exit 1

CMD ["./aeromind_backend"]
