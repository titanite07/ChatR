#include <iostream>
#include <string>
#include <atomic>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <conio.h>
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
    if (len > 10 * 1024 * 1024) return false;
    out.resize(len);
    if (!recv_exact(s, &out[0], static_cast<int>(len))) return false;
    return true;
}

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

class BasicChatClient {
private:
    SOCKET socket_;
    std::string username_;
    std::atomic<bool> connected_;
    
public:
    BasicChatClient() : socket_(INVALID_SOCKET), connected_(false) {}
    
    ~BasicChatClient() {
        disconnect();
    }
    
    bool connect(const std::string& server, int port, const std::string& username) {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            log_err("WSAStartup failed");
            return false;
        }
        
        socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_ == INVALID_SOCKET) {
            log_err("Socket creation failed");
            WSACleanup();
            return false;
        }
        
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        
        server_addr.sin_addr.s_addr = inet_addr(server.c_str());
        if (server_addr.sin_addr.s_addr == INADDR_NONE) {
            log_err("Invalid server address");
            closesocket(socket_);
            WSACleanup();
            return false;
        }
        
        if (::connect(socket_, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            log_err("Connection failed");
            closesocket(socket_);
            WSACleanup();
            return false;
        }
        
        username_ = username;
        connected_ = true;
        log_info("Connected to " + server + ":" + std::to_string(port));
        
        std::string uname_line = username + "\n";
        send_all(socket_, uname_line.c_str(), static_cast<int>(uname_line.size()));
        
        return true;
    }
    
    void disconnect() {
        if (connected_.exchange(false)) {
            if (socket_ != INVALID_SOCKET) {
                closesocket(socket_);
                socket_ = INVALID_SOCKET;
            }
            WSACleanup();
            log_info("Disconnected");
        }
    }
    
    bool send_message(const std::string& message) {
        if (!connected_) return false;
        std::string json = make_chat_json("CHAT", username_, message);
        return send_frame(socket_, json);
    }
    
    void run() {
        if (!connected_) {
            log_err("Not connected");
            return;
        }
        
        log_info("Connected as " + username_);
        std::cout << "Type your messages (type 'quit' to exit):" << std::endl;
        std::cout << "----------------------------------------" << std::endl;
        
        while (connected_) {
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(socket_, &read_fds);
            struct timeval timeout; timeout.tv_sec = 0; timeout.tv_usec = 100000;
            int activity = select(0, &read_fds, NULL, NULL, &timeout);
            if (activity > 0 && FD_ISSET(socket_, &read_fds)) {
                std::string frame;
                if (!recv_frame(socket_, frame)) {
                    log_err("Connection lost");
                    break;
                }
                std::string type = json_get_string(frame, "type");
                std::string sender = json_get_string(frame, "sender");
                std::string content = json_get_string(frame, "content");
                if (type == "USER_LIST" || type == "SYSTEM" || type == "JOIN" || type == "LEAVE") {
                    log_info(content);
                } else {
                    std::cout << sender << ": " << content << std::endl;
                }
            }
            
            if (_kbhit()) {
                std::string line;
                std::getline(std::cin, line);
                if (line == "quit" || line == "exit") {
                    break;
                }
                if (!line.empty()) {
                    if (!send_message(line)) {
                        log_err("Send failed");
                        break;
                    }
                }
            }
        }
        
        disconnect();
    }
};

static std::string prompt_or_default(const std::string& label, const std::string& def) {
    std::cout << label << " [" << def << "]: ";
    std::string v; std::getline(std::cin, v);
    if (v.empty()) return def;
    return v;
}

int main(int argc, char* argv[]) {
    std::string server;
    int port = 0;
    std::string username;
    
    if (argc >= 4) {
        server = argv[1];
        port = std::stoi(argv[2]);
        username = argv[3];
    } else {
        server = prompt_or_default("Server", "127.0.0.1");
        std::string port_s = prompt_or_default("Port", "8080");
        try { port = std::stoi(port_s); } catch (...) { std::cout << "Invalid port, using 8080\n"; port = 8080; }
        while (true) {
            std::cout << "Username: ";
            std::getline(std::cin, username);
            if (!username.empty()) break;
            std::cout << "Username cannot be empty.\n";
        }
    }
    
    BasicChatClient client;
    if (!client.connect(server, port, username)) {
        return 1;
    }
    
    client.run();
    return 0;
}
