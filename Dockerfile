FROM ubuntu:24.04 as builder

# Install build dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    git \
    curl \
    libpq-dev \
    libpqxx-dev \
    libjsoncpp-dev \
    libssl-dev \
    zlib1g-dev \
    libuv1-dev \
    libpcre3-dev \
    pkg-config \
    postgresql-client \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build

# Copy the entire project
COPY . /build/

# Build the application
RUN mkdir -p build && \
    cd build && \
    cmake .. && \
    make -j$(nproc) && \
    cmake --install .

# Runtime stage
FROM ubuntu:24.04

# Install runtime dependencies
RUN apt-get update && apt-get install -y \
    libpq5 \
    libjsoncpp1 \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# Copy built binary and libraries from builder
COPY --from=builder /usr/local/bin/aeromind_backend /app/
COPY --from=builder /usr/local/lib /usr/local/lib

# Create logs directory
RUN mkdir -p /app/logs && ldconfig

# Expose port 8848
EXPOSE 8848

# Health check
HEALTHCHECK --interval=30s --timeout=10s --start-period=40s --retries=3 \
    CMD curl -f http://localhost:8848/health || exit 1

# Run the application
CMD ["./aeromind_backend"]
