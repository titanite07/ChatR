#include <iostream>
#include <string>
#include <vector>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <ctime>

#pragma comment(lib, "ws2_32.lib")

static std::string now_ts() {
    std::time_t t = std::time(nullptr);
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t));
    return std::string(buf);
}

static void log_info(const std::string& msg) {
    std::cout << "[" << now_ts() << "] " << msg << std::endl;
}

static void log_err(const std::string& msg) {
    std::cerr << "[" << now_ts() << "] ERROR: " << msg << std::endl;
}

static bool send_all(SOCKET s, const char* data, int len) {
    int sent = 0;
    while (sent < len) {
        int n = send(s, data + sent, len - sent, 0);
        if (n <= 0) return false;
        sent += n;
    }
    return true;
}

static bool recv_exact(SOCKET s, char* buf, int len) {
    int got = 0;
    while (got < len) {
        int n = recv(s, buf + got, len - got, 0);
        if (n <= 0) return false;
        got += n;
    }
    return true;
}

static bool send_frame(SOCKET s, const std::string& payload) {
    uint32_t nlen = htonl(static_cast<uint32_t>(payload.size()));
    if (!send_all(s, reinterpret_cast<const char*>(&nlen), 4)) return false;
    return send_all(s, payload.data(), static_cast<int>(payload.size()));
}

static bool recv_frame(SOCKET s, std::string& out) {
    uint32_t nlen = 0;
    if (!recv_exact(s, reinterpret_cast<char*>(&nlen), 4)) return false;
    uint32_t len = ntohl(nlen);
    if (len > 10 * 1024 * 1024) return false; // sanity limit 10MB
    out.resize(len);
    if (!recv_exact(s, &out[0], static_cast<int>(len))) return false;
    return true;
}

// minimal JSON helpers (expects flat JSON with string values and numeric ts)
static std::string json_escape(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 8);
    for (char c : s) {
        if (c == '\\' || c == '"') out.push_back('\\');
        if (c == '\n') { out += "\\n"; continue; }
        out.push_back(c);
    }
    return out;
}

static std::string make_chat_json(const std::string& type,
                                  const std::string& sender,
                                  const std::string& content) {
    std::string j = "{";
    j += "\"type\":\"" + json_escape(type) + "\",";
    j += "\"sender\":\"" + json_escape(sender) + "\",";
    j += "\"content\":\"" + json_escape(content) + "\",";
    j += "\"ts\":" + std::to_string(std::time(nullptr));
    j += "}";
    return j;
}

static std::string json_get_string(const std::string& j, const std::string& key) {
    std::string pat = "\"" + key + "\"";
    size_t p = j.find(pat);
    if (p == std::string::npos) return "";
    p = j.find(':', p);
    if (p == std::string::npos) return "";
    p = j.find('"', p);
    if (p == std::string::npos) return "";
    size_t q = j.find('"', p + 1);
    if (q == std::string::npos) return "";
    std::string val = j.substr(p + 1, q - (p + 1));
    // unescape minimal
    std::string out;
    out.reserve(val.size());
    for (size_t i = 0; i < val.size(); ++i) {
        if (val[i] == '\\' && i + 1 < val.size()) {
            char n = val[i + 1];
            if (n == 'n') { out.push_back('\n'); ++i; continue; }
            out.push_back(n); ++i; continue;
        }
        out.push_back(val[i]);
    }
    return out;
}

class BasicChatRoom {
private:
    SOCKET server_socket;
    std::vector<SOCKET> clients;
    std::vector<std::string> usernames;
    std::string room_name;
    bool running;
    
public:
    BasicChatRoom(const std::string& name = "Basic Chat Room") 
        : server_socket(INVALID_SOCKET), room_name(name), running(false) {}
    
    ~BasicChatRoom() {
        stop();
    }
    
    bool start(int port) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            log_err("WSAStartup failed");
            return false;
        }
        
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket == INVALID_SOCKET) {
            log_err("Socket creation failed");
            WSACleanup();
            return false;
        }
        
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_addr.s_addr = INADDR_ANY;
        server_addr.sin_port = htons(port);
        
        if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            log_err("Bind failed");
            closesocket(server_socket);
            WSACleanup();
            return false;
        }
        
        if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
            log_err("Listen failed");
            closesocket(server_socket);
            WSACleanup();
            return false;
        }
        
        running = true;
        log_info("Chat room '" + room_name + "' started on port " + std::to_string(port));
        log_info("Waiting for connections...");
        
        return true;
    }
    
    void stop() {
        running = false;
        
        for (SOCKET client : clients) {
            closesocket(client);
        }
        clients.clear();
        usernames.clear();
        
        if (server_socket != INVALID_SOCKET) {
            closesocket(server_socket);
            server_socket = INVALID_SOCKET;
        }
        
        WSACleanup();
    }
    
    void run() {
        while (running) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(server_socket, &read_fds);
            
            for (SOCKET client : clients) {
                FD_SET(client, &read_fds);
            }
            
            struct timeval timeout;
            timeout.tv_sec = 1;
            timeout.tv_usec = 0;
            
            int activity = select(0, &read_fds, NULL, NULL, &timeout);
            if (activity < 0) {
                log_err("Select failed");
                break;
            }
            
            if (FD_ISSET(server_socket, &read_fds)) {
                sockaddr_in client_addr;
                int client_addr_len = sizeof(client_addr);
                
                SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &client_addr_len);
                if (client_socket != INVALID_SOCKET) {
                    handle_new_client(client_socket);
                }
            }
            
            for (size_t i = 0; i < clients.size(); i++) {
                if (FD_ISSET(clients[i], &read_fds)) {
                    if (!handle_client_message(i)) {
                        remove_client(i);
                        i--;
                    }
                }
            }
        }
    }
    
private:
    void handle_new_client(SOCKET client_socket) {
        char buffer[1024];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            std::string username = buffer;
            
            while (!username.empty() && (username.back() == '\n' || username.back() == '\r')) {
                username.pop_back();
            }
            
            if (username.empty()) {
                username = "Anonymous_" + std::to_string(rand() % 1000);
            }
            
            clients.push_back(client_socket);
            usernames.push_back(username);
            
            std::string join_json = make_chat_json("JOIN", username, username + " joined the chat");
            broadcast_json(join_json, client_socket);
            
            std::string user_list = "Online users: ";
            for (const std::string& name : usernames) {
                user_list += name + ", ";
            }
            if (user_list.size() >= 2 && user_list.substr(user_list.size()-2) == ", ") {
                user_list.resize(user_list.size()-2);
            }
            std::string users_json = make_chat_json("USER_LIST", "Server", user_list);
            send_frame(client_socket, users_json);
            
            log_info("New client connected: " + username);
        } else {
            closesocket(client_socket);
        }
    }
    
    bool handle_client_message(size_t client_index) {
        SOCKET client_socket = clients[client_index];
        std::string frame;
        if (!recv_frame(client_socket, frame)) {
            return false;
        }
        
        const std::string& username = usernames[client_index];
        std::string type = json_get_string(frame, "type");
        std::string content = json_get_string(frame, "content");
        if (type.empty()) type = "CHAT";
        
        if (!content.empty()) {
            std::string chat_json = make_chat_json("CHAT", username, content);
            log_info("[" + username + "] " + content);
            broadcast_json(chat_json, client_socket);
        }
        
        return true;
    }
    
    void broadcast_json(const std::string& json, SOCKET sender = INVALID_SOCKET) {
        for (SOCKET client : clients) {
            if (client != sender) {
                if (!send_frame(client, json)) {
                    // best effort
                }
            }
        }
    }
    
    void remove_client(size_t index) {
        std::string username = usernames[index];
        SOCKET client_socket = clients[index];
        
        std::string leave_json = make_chat_json("LEAVE", username, username + " left the chat");
        broadcast_json(leave_json);
        
        clients.erase(clients.begin() + index);
        usernames.erase(usernames.begin() + index);
        closesocket(client_socket);
        
        log_info("Client disconnected: " + username);
    }
};

int main(int argc, char* argv[]) {
    int port = 8080;
    std::string room_name = "Basic Chat Room";
    
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }
    
    if (argc > 2) {
        room_name = argv[2];
    }
    
    log_info("Starting Basic Chat Room Server...");
    log_info("Room Name: " + room_name);
    log_info("Port: " + std::to_string(port));
    std::cout << "----------------------------------------" << std::endl;
    
    try {
        BasicChatRoom chat_room(room_name);
        
        if (!chat_room.start(port)) {
            log_err("Failed to start chat room server");
            return 1;
        }
        
        chat_room.run();
        
    } catch (const std::exception& e) {
        log_err(std::string("Server exception: ") + e.what());
        return 1;
    }
    
    log_info("Server stopped");
    return 0;
}
