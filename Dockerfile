# Build stage
FROM alpine:latest AS builder

# Install build dependencies
RUN apk add --no-cache \
    build-base \
    cmake \
    linux-headers

# Set working directory
WORKDIR /app

# Copy source code
COPY . .

# Build the project
RUN mkdir build && cd build && cmake .. && make

# Run stage
FROM alpine:latest

WORKDIR /app

# Install runtime dependencies for C++ binaries
RUN apk add --no-cache libstdc++ libgcc

# Copy the built executables from the builder stage
COPY --from=builder /app/build/rampage /app/rampage
COPY --from=builder /app/build/rampage_cli /app/rampage_cli

# Expose the default port
EXPOSE 2006

# Command to run the server with persistence enabled by default
ENTRYPOINT ["/app/rampage", "--persist"]
