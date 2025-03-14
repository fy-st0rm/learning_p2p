# Use a minimal C runtime
FROM debian:latest

# Install dependencies
RUN apt-get update && apt-get install -y gcc make

# Set working directory
WORKDIR /app

# Copy source code
COPY server.c .

# Compile the server
RUN gcc server.c -o server

# Expose the required port
EXPOSE 8080/udp

# Run the server
CMD ["./server"]
