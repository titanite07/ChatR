# Chat Room C++ Application

A clean and functional chat room application built in C++ using Windows Sockets (Winsock).

## 🏗️ Project Structure

```
chatRoomCpp/
├── chatRoom_basic.cpp      # Chat server implementation
├── client_basic.cpp        # Chat client implementation
├── message.hpp             # Message structure definitions
├── Makefile                # Build configuration
├── test_chat.bat          # Windows batch file to test the system
├── .gitignore             # Git ignore file for build artifacts
└── README.md               # This file
```

## 🚀 Quick Start

### Prerequisites
- Windows operating system
- MinGW-w64 or Visual Studio compiler
- Basic C++ knowledge

### Building the Project

1. **Compile the server:**
   ```bash
   g++ -std=c++17 -Wall -Wextra -g -O2 -c chatRoom_basic.cpp -o chatRoom_basic.o
   g++ chatRoom_basic.o -lws2_32 -o chatServer.exe
   ```

2. **Compile the client:**
   ```bash
   g++ -std=c++17 -Wall -Wextra -g -O2 -c client_basic.cpp -o client_basic.o
   g++ client_basic.o -lws2_32 -o clientApp.exe
   ```

3. **Or use the Makefile:**
   ```bash
   make all
   ```

### Running the Application

#### Option 1: Manual Testing
1. **Start the server:**
   ```bash
   .\chatServer.exe 8080 "My Chat Room"
   ```

2. **Start clients in separate terminals:**
   ```bash
   .\clientApp.exe 127.0.0.1 8080 Alice
   .\clientApp.exe 127.0.0.1 8080 Bob
   ```

#### Option 2: Automated Testing
1. **Run the test script:**
   ```bash
   .\test_chat.bat
   ```
   This will automatically start the server and two test clients.

## 🎯 Features

### Server Features
- ✅ Multi-client support
- ✅ Real-time message broadcasting
- ✅ User join/leave notifications
- ✅ Online user list
- ✅ Automatic client cleanup
- ✅ Non-blocking I/O with select()

### Client Features
- ✅ Simple command-line interface
- ✅ Real-time message reception
- ✅ Non-blocking input/output
- ✅ Clean disconnect

### Message Types
- **CHAT**: Regular chat messages
- **JOIN**: User joined notification
- **LEAVE**: User left notification
- **USER_LIST**: Online users list
- **SYSTEM**: Server system messages

## 🔧 Technical Details

### Architecture
- **Server**: Single-threaded with select() for I/O multiplexing
- **Client**: Non-blocking socket operations
- **Protocol**: Simple text-based protocol with newline delimiters
- **Networking**: TCP sockets with Winsock2

### Dependencies
- **Windows Sockets 2** (ws2_32.lib)
- **C++17 Standard Library**

### Compilation Flags
- `-std=c++17`: Use C++17 standard
- `-Wall -Wextra`: Enable all warnings
- `-g`: Include debug information
- `-O2`: Optimize for performance
- `-lws2_32`: Link Windows Sockets library

## 📱 Usage Instructions

### Server Commands
```bash
chatServer.exe [port] [room_name]
```
- **port**: Server port (default: 8080)
- **room_name**: Chat room name (default: "Basic Chat Room")

### Client Commands
```bash
clientApp.exe <server_ip> <port> <username>
```
- **server_ip**: Server IP address (e.g., 127.0.0.1)
- **port**: Server port number
- **username**: Your display name

### Client Controls
- Type messages and press Enter to send
- Type `quit` or `exit` to disconnect
- Messages are automatically received from other users

## 🧪 Testing

### Manual Testing
1. Start server on port 8080
2. Connect multiple clients with different usernames
3. Send messages between clients
4. Test disconnect/reconnect scenarios

### Automated Testing
The `test_chat.bat` script provides:
- Automatic server startup
- Multiple client connections
- Pre-configured test usernames
- Easy cleanup

## 🐛 Troubleshooting

### Common Issues

1. **"WSAStartup failed"**
   - Ensure Windows Sockets are available
   - Run as administrator if needed

2. **"Bind failed"**
   - Port may be in use
   - Try a different port number
   - Check firewall settings

3. **"Connection failed"**
   - Verify server is running
   - Check IP address and port
   - Ensure no firewall blocking

4. **Compilation errors**
   - Use C++17 or later
   - Ensure MinGW-w64 is properly installed
   - Check include paths

### Debug Mode
Compile with debug flags for detailed error information:
```bash
g++ -std=c++17 -Wall -Wextra -g -DDEBUG -c chatRoom_basic.cpp
```

## 📄 License

This project is provided as-is for educational purposes. Feel free to modify and extend it.

## 🤝 Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📞 Support

For issues or questions:
1. Check the troubleshooting section
2. Review the code comments
3. Test with the provided batch file
4. Ensure all dependencies are met

---

**Happy Chatting! 🎉**
