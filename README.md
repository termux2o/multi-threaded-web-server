# Simple C++ Multi-Threaded Web Server
- Serves static files from `www/`
- Per-connection threads (very simple)
- Linux & Windows build via CMake

## Build
```bash
mkdir build && cd build
cmake ..
cmake --build .
./server 8080
### Windows
- Install CMake + MSVC Build Tools
- Configure with CMake GUI or:
- cmake -S . -B build
- cmake --build build --config Release
- .\build\Release\server.exe 8080
Simple Multi-threaded HTTP Server
Overview

This is a simple multi-threaded HTTP server written in C++ that serves static files from a www directory. It handles multiple client connections simultaneously using threads, making it efficient for serving multiple requests at the same time.
How to Compile and Run
Compilation

On Linux/Mac:
bash

g++ -std=c++11 -pthread server.cpp -o server

On Windows (MinGW):
bash

g++ -std=c++11 server.cpp -o server.exe -lws2_32

Running

Create www directory and add some files:
bash

mkdir www
echo "<h1>Welcome to My Server!</h1>" > www/index.html
echo "body { background: lightblue; }" > www/style.css

Run the server:
bash

./server

How Multi-threading Works
The Core Concept

Instead of handling one client at a time (single-threaded), this server creates a new thread for each client connection. This allows multiple clients to be served simultaneously without blocking each other.
Step-by-Step Process

    Main Thread (Acceptor)

        Creates the server socket

        Binds to port 8080

        Listens for incoming connections

        Runs in an infinite loop accepting connections

    Client Handling (Worker Threads)

        When a new client connects, accept() returns a new socket

        The main thread immediately creates a new thread for that client

        The new thread handles ALL communication with that client

        Main thread goes back to accepting more connections

Code Flow
cpp

while (server_running) {
    // Wait for new connection
    int client_socket = accept(server_socket, ...);
    
    // Create thread for this client
    std::thread client_thread([client_socket]() {
        handle_client(client_socket);  // This runs in parallel!
    });
    
    client_thread.detach();  // Don't wait for thread to finish
}

Server Architecture
text

Main Thread (Acceptor)
    |
    |--> Client 1 Thread --> Handles Client 1
    |
    |--> Client 2 Thread --> Handles Client 2
    |
    |--> Client 3 Thread --> Handles Client 3
    |
    |--> ... (more threads as clients connect)

Thread Safety
The Logging Mutex
cpp

std::mutex log_mutex;  // Global mutex for thread-safe logging

{
    std::lock_guard<std::mutex> lock(log_mutex);  // Auto-locks
    std::cout << "Thread-safe logging!" << std::endl;
    // Auto-unlocks when lock goes out of scope
}

Why needed? Without the mutex, multiple threads writing to console simultaneously would create garbled output.
Detailed Client Handling

Each client thread performs these steps:

    Receive Request: Read HTTP request from client socket

    Parse Request: Extract requested file path

    Detect Client: Identify browser type (Chrome, Firefox, etc.)

    Read File: Load file from www directory

    Create Response: Build proper HTTP response

    Send Response: Send data back to client

    Log Details: Thread-safe logging of the transaction

    Cleanup: Close client socket

Supported Features
File Types

    HTML (.html) - text/html

    CSS (.css) - text/css

    JavaScript (.js) - application/javascript

    PNG images (.png) - image/png

    JPEG images (.jpg, .jpeg) - image/jpeg

    All others - text/plain

Browser Detection

    Chrome

    Firefox

    Safari

    Edge

    Terminal clients (curl, wget)

    Unknown clients

Security Features

    Path traversal protection (blocks ../ attacks)

    Socket timeouts

    Proper resource cleanup

Example Output
text

Starting Simple HTTP Server...
Platform: Linux/Unix
Serving files from: www/
Port: 8080

Server is running! Press Ctrl+C to stop.
Waiting for connections...

CLIENT CONNECTION
IP: 192.168.1.15:54321
Client Type: Chrome
Requested: /index.html
Status: 200 OK
File: www/index.html
Content-Type: text/html
File Size: 127 bytes
Response Size: 243 bytes sent
Thread ID: 140245678912320

CLIENT CONNECTION
IP: 192.168.1.22:54322
Client Type: Firefox
Requested: /style.css
Status: 200 OK
File: www/style.css
Content-Type: text/css
File Size: 45 bytes
Response Size: 161 bytes sent
Thread ID: 140245678915456

Performance Considerations
Advantages of Multi-threading

    Parallel Processing: Multiple clients served simultaneously

    Responsive: Main thread never blocks on client processing

    Scalable: Can handle many concurrent connections

    Efficient: Threads are lightweight compared to processes

Limitations

    No thread pool (creates unlimited threads)

    No rate limiting

    Basic error handling

    No HTTPS support

For Production Use

This is an educational server. For production use, consider:

    Using a thread pool instead of unlimited threads

    Adding connection limits

    Implementing proper timeouts

    Adding HTTPS support

    Including more robust error handling

Troubleshooting
Common Issues

    Port already in use
    bash

# Find and kill process using port 8080
lsof -i :8080
kill -9 <PID>

Permission denied
bash

    # Run with sudo if needed (not recommended)
    sudo ./server

    File not found errors

        Ensure www directory exists

        Check file permissions in www directory

Learning Points

This server demonstrates:

    Socket programming in C++

    Multi-threading with std::thread

    Thread synchronization with std::mutex

    HTTP protocol basics

    Cross-platform networking

    Resource management and cleanup

Alternative Approaches
Single-threaded with select()
cpp

// Could use select() for non-blocking I/O
// Handles multiple clients in one thread
// More complex but avoids thread overhead

Thread Pool Pattern
cpp

// Pre-create fixed number of threads
// Better resource control than unlimited threads
// More complex implementation

Support

This is an educational project. For questions:

    Check the code comments

    Review C++ networking documentation

    Examine the detailed logging output