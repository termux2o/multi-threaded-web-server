// Simple HTTP File Server
// Serves files from 'www' folder on port 8080

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <cstring>
#include <chrono>
#include <iomanip>
#include <sstream>

// Platform-specific network setup
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "Ws2_32.lib")
    #define CLOSESOCKET closesocket
    #define PLATFORM "Windows"
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    #define CLOSESOCKET close
    #define PLATFORM "Linux/Unix"
#endif

// Global variables
std::mutex log_mutex;  // For thread-safe logging
bool server_running = true;

// Function to get current time as string
std::string get_current_time() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time), "%H:%M:%S") 
       << "." << std::setfill('0') << std::setw(3) << ms.count();
    return ss.str();
}

// Initialize network (Windows only)
bool init_network() {
    #ifdef _WIN32
        WSADATA wsa;
        return WSAStartup(MAKEWORD(2, 2), &wsa) == 0;
    #else
        return true;
    #endif
}

// Cleanup network (Windows only)
void cleanup_network() {
    #ifdef _WIN32
        WSACleanup();
    #endif
}

// Read file from disk
std::string read_file(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) return "";
    
    std::string content;
    file.seekg(0, std::ios::end);
    content.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    file.read(&content[0], content.size());
    
    return content;
}

// Get content type from filename
std::string get_content_type(const std::string& filename) {
    if (filename.find(".html") != std::string::npos) return "text/html";
    if (filename.find(".css") != std::string::npos) return "text/css";
    if (filename.find(".js") != std::string::npos) return "application/javascript";
    if (filename.find(".png") != std::string::npos) return "image/png";
    if (filename.find(".jpg") != std::string::npos) return "image/jpeg";
    if (filename.find(".jpeg") != std::string::npos) return "image/jpeg";
    return "text/plain";
}

// Create HTTP response
std::string create_response(const std::string& content, const std::string& type, bool is_ok) {
    if (is_ok) {
        return "HTTP/1.1 200 OK\r\n"
               "Content-Type: " + type + "\r\n"
               "Content-Length: " + std::to_string(content.size()) + "\r\n"
               "Connection: close\r\n\r\n" + content;
    } else {
        std::string error_page = "<h1>404 - File Not Found</h1>";
        return "HTTP/1.1 404 Not Found\r\n"
               "Content-Type: text/html\r\n"
               "Content-Length: " + std::to_string(error_page.size()) + "\r\n"
               "Connection: close\r\n\r\n" + error_page;
    }
}

// Detect browser/client type from User-Agent
std::string detect_client_type(const std::string& request) {
    size_t pos = request.find("User-Agent:");
    if (pos == std::string::npos) return "Unknown";
    
    std::string user_agent = request.substr(pos + 11);
    user_agent = user_agent.substr(0, user_agent.find("\r\n"));
    
    if (user_agent.find("Chrome") != std::string::npos) return "Chrome";
    if (user_agent.find("Firefox") != std::string::npos) return "Firefox";
    if (user_agent.find("Safari") != std::string::npos) return "Safari";
    if (user_agent.find("Edge") != std::string::npos) return "Edge";
    if (user_agent.find("curl") != std::string::npos) return "Terminal (curl)";
    if (user_agent.find("Wget") != std::string::npos) return "Terminal (wget)";
    
    return "Other (" + user_agent.substr(0, 20) + "...)";
}

// Extract requested file path from HTTP request
std::string get_requested_file(const std::string& request) {
    size_t start = request.find("GET ");
    if (start == std::string::npos) return "/index.html";
    
    start += 4;
    size_t end = request.find(" ", start);
    if (end == std::string::npos) return "/index.html";
    
    std::string path = request.substr(start, end - start);
    
    // Security: prevent path traversal attacks
    if (path.find("..") != std::string::npos) return "/index.html";
    if (path == "/") return "/index.html";
    
    return path;
}

// Handle client connection
void handle_client(int client_socket, struct sockaddr_in client_addr) {
    char buffer[2048];
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes_received <= 0) {
        CLOSESOCKET(client_socket);
        return;
    }
    
    buffer[bytes_received] = '\0';
    std::string request(buffer);
    
    // Get client info
    std::string client_ip = inet_ntoa(client_addr.sin_addr);
    int client_port = ntohs(client_addr.sin_port);
    std::string client_type = detect_client_type(request);
    std::string requested_file = get_requested_file(request);
    
    // Prepare file path
    std::string file_path = "www" + requested_file;
    std::string file_content = read_file(file_path);
    bool file_exists = !file_content.empty();
    
    // Create response
    std::string content_type = get_content_type(file_path);
    std::string response = create_response(file_content, content_type, file_exists);
    
    // Send response
    send(client_socket, response.c_str(), response.size(), 0);
    CLOSESOCKET(client_socket);
    
    // Get current time for logging
    std::string current_time = get_current_time();
    
    // Log the request (thread-safe)
    {
        std::lock_guard<std::mutex> lock(log_mutex);
        
        std::cout << "╔═══════════════════════════════════════════════════\n";
        std::cout << "║ CLIENT CONNECTION [" << current_time << "]\n";
        std::cout << "║ IP: " << client_ip << ":" << client_port << "\n";
        std::cout << "║ Client Type: " << client_type << "\n";
        std::cout << "║ Requested: " << requested_file << "\n";
        std::cout << "║ Status: " << (file_exists ? "200 OK" : "404 Not Found") << "\n";
        std::cout << "║ File: " << file_path << "\n";
        std::cout << "║ Content-Type: " << content_type << "\n";
        std::cout << "║ File Size: " << file_content.size() << " bytes\n";
        std::cout << "║ Response Size: " << response.size() << " bytes sent\n";
        std::cout << "║ Thread ID: " << std::this_thread::get_id() << "\n";
        std::cout << "╚═══════════════════════════════════════════════════\n\n";
    }
}

int main() {
    std::cout << "Starting Simple HTTP Server...\n";
    std::cout << "Platform: " << PLATFORM << "\n";
    std::cout << "Serving files from: www/\n";
    std::cout << "Port: 8080\n\n";
    
    // Initialize network
    if (!init_network()) {
        std::cerr << "Failed to initialize network!\n";
        return 1;
    }
    
    // Create server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Failed to create socket!\n";
        cleanup_network();
        return 1;
    }
    
    // Set socket options (reuse address)
    #ifndef _WIN32
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    #endif
    
    // Bind socket to port 8080
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);
    
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Failed to bind to port 8080!\n";
        CLOSESOCKET(server_socket);
        cleanup_network();
        return 1;
    }
    
    // Start listening
    if (listen(server_socket, 10) < 0) {
        std::cerr << "Failed to listen on socket!\n";
        CLOSESOCKET(server_socket);
        cleanup_network();
        return 1;
    }
    
    std::cout << "Server is running! Press Ctrl+C to stop.\n";
    std::cout << "Waiting for connections...\n\n";
    
    // Main server loop
    while (server_running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Accept incoming connection
        int client_socket = accept(server_socket, 
                                 (struct sockaddr*)&client_addr, 
                                 &client_len);
        
        if (client_socket < 0) {
            continue;  // Skip failed connections
        }
        
        // Handle each client in a new thread
        std::thread client_thread([client_socket, client_addr]() {
            handle_client(client_socket, client_addr);
        });
        
        client_thread.detach();  // Don't wait for thread to finish
    }
    
    // Cleanup
    CLOSESOCKET(server_socket);
    cleanup_network();
    
    std::cout << "Server stopped.\n";
    return 0;
}