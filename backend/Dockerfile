# Start from a Linux system with g++ already set up
FROM gcc:latest

# Install SQLite development files (needed to compile with -lsqlite3)
RUN apt-get update && apt-get install -y libsqlite3-dev

# Set the working folder inside the container
WORKDIR /app

# Copy all backend files (main.cpp, dsa/, db/, third_party/) into the container
COPY . .

# Compile the server (Linux doesn't need -lws2_32, that was Windows-only)
RUN g++ main.cpp -o server -std=c++17 -lsqlite3 -pthread

# Tell Docker which port the app will use
EXPOSE 10000

# Start the server when the container runs
CMD ["./server"]