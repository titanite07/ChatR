#ifndef MESSAGE_HPP
#define MESSAGE_HPP

#include <string>
#include <sstream>
#include <ctime>

enum class MessageType {
    CHAT = 0,
    JOIN = 1,
    LEAVE = 2,
    USER_LIST = 3,
    SYSTEM = 4,
    MSG_ERROR = 5
};

struct ChatMessage {
    MessageType type;
    std::string sender;
    std::string content;
    std::string timestamp;
    
    ChatMessage() : type(MessageType::CHAT) {}
    
    ChatMessage(MessageType t, const std::string& s, const std::string& c)
        : type(t), sender(s), content(c) {
        time_t now = time(0);
        timestamp = std::to_string(now);
    }
    
    std::string serialize() const {
        std::ostringstream oss;
        oss << static_cast<int>(type) << "|" 
            << sender << "|" 
            << content << "|" 
            << timestamp;
        return oss.str();
    }
    
    bool deserialize(const std::string& data) {
        std::istringstream iss(data);
        std::string token;
        std::vector<std::string> tokens;
        
        while (std::getline(iss, token, '|')) {
            tokens.push_back(token);
        }
        
        if (tokens.size() >= 4) {
            type = static_cast<MessageType>(std::stoi(tokens[0]));
            sender = tokens[1];
            content = tokens[2];
            timestamp = tokens[3];
            return true;
        }
        return false;
    }
};

#endif
