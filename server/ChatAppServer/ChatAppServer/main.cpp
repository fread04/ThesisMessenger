#include "include/client_handler.h"
#include "include/db.h"
#include "include/auth.h"
#include "include/config.h"
#include "include/db_utils.h"

#include <winsock2.h>
#include <ws2tcpip.h>
#include <thread>
#include <tchar.h>
#include <codecvt>
#include <locale>

bool InitializeWinsock() {
    WSADATA data;
    return WSAStartup(MAKEWORD(2, 2), &data) == 0;
}

std::wstring ToWide(const std::string& str) {
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
    return converter.from_bytes(str);
}

int main() {
    system("chcp 1251");
    
    if (!config.load(".config/server_config.ini")) {
        std::cerr << "Не удалось загрузить конфигурацию!" << std::endl;
        return 1;
    }

    if (!InitializeWinsock()) {
        return 1;
    }

    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket == INVALID_SOCKET) {
        return 1;
    }

    int port = std::stoi(config.get("Server", "port", "default_server_port"));
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    InetPton(AF_INET, ToWide(config.get("Server", "host", "default_server_host")).c_str(), &serverAddr.sin_addr);

    if (bind(listenSocket, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == SOCKET_ERROR || listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Сервер запущен на порту " << port << std::endl;

    while (true) {
        SOCKET clientSocket = accept(listenSocket, nullptr, nullptr);
        if (clientSocket == INVALID_SOCKET) continue;
        std::thread clientThread(HandleClientConnection, clientSocket);
        clientThread.detach();
    }

    closesocket(listenSocket);
    WSACleanup();
    return 0;
}
