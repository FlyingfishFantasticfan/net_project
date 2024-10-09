#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_net.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cstring>

void UpdateClients(SDLNet_SocketSet socketSet);
void SendMessagesToAllClients();
void DataReady(const std::string& message);

class Client {
public:
    Client(TCPsocket socket) : socket(socket), bytesInBuffer(0) {}

    void ReceiveMessage() {
        int received = SDLNet_TCP_Recv(socket, buffer + bytesInBuffer, sizeof(buffer) - bytesInBuffer);
        if (received <= 0) {
            // 处理错误或连接关闭
            SDLNet_TCP_Close(socket);
            valid = false;
            return;
        }
        bytesInBuffer += received;

        int bufferOffset = 0;

        while (bytesInBuffer - bufferOffset >= sizeof(int)) // 检查是否有足够的数据来读取消息长度
        {
            int networkLength;
            memcpy(&networkLength, buffer + bufferOffset, sizeof(int));
            int length = SDL_SwapBE32(networkLength); // 转换为主机字节序

            // 检查是否接收到完整的包体若没有则退出循环等待下一次接收
            if (bytesInBuffer - bufferOffset < sizeof(int) + length) {
                break;
            }

            // 读取消息内容
            char* messageBuffer = new char[length];
            memcpy(messageBuffer, buffer + bufferOffset + sizeof(int), length);

            std::string message(messageBuffer, length);
            delete[] messageBuffer;

            // 调用 DataReady 处理接收到的消息
            DataReady(message);

            // 更新缓冲区偏移量
            bufferOffset += sizeof(int) + length;
        }

        // 移动未处理的数据到缓冲区的起始位置
        bytesInBuffer -= bufferOffset;
        memmove(buffer, buffer + bufferOffset, bytesInBuffer);
    }

    void SendMessage(const std::string& message) {
        int length = message.length() + 1;
        int networkLength = SDL_SwapBE32(length); // 转换为网络字节序
        SDLNet_TCP_Send(socket, &networkLength, sizeof(networkLength));
        SDLNet_TCP_Send(socket, message.c_str(), length);
    }

    bool is_valid() const { return valid; }

    TCPsocket get_socket() const { return socket; }

private:
    bool valid = true;
    TCPsocket socket;
    char buffer[2048];
    int bytesInBuffer;
};

int count = 0;
std::vector<Client> clients;

std::string to_string(int value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

int main() {
    if (SDL_Init(0) == -1) {
        std::cerr << "SDL_Init error" << std::endl;
        return 1;
    }

    if (SDLNet_Init() == -1) {
        std::cerr << "SDLNet_Init error" << std::endl;
        SDL_Quit();
        return 2;
    }

    IPaddress ip;
    if (SDLNet_ResolveHost(&ip, NULL, 1234) == -1) {
        std::cerr << "SDLNet_ResolveHost error" << std::endl;
        SDLNet_Quit();
        SDL_Quit();
        return 3;
    }

    TCPsocket server = SDLNet_TCP_Open(&ip);
    if (!server) {
        std::cerr << "SDLNet_TCP_Open error" << std::endl;
        SDLNet_Quit();
        SDL_Quit();
        return 4;
    }

    SDLNet_SocketSet socketSet = SDLNet_AllocSocketSet(20);
    SDLNet_TCP_AddSocket(socketSet, server);

    while (true) {

        int numReady = SDLNet_CheckSockets(socketSet, -1); // 阻塞等待直到有套接字准备好
        if (numReady == -1) {
            std::cerr << "SDLNet_CheckSockets error" << std::endl;
            break;
        }

        if (SDLNet_SocketReady(server)) {
            // 接受新的客户端连接
            TCPsocket client = SDLNet_TCP_Accept(server);
            if (client) {
                SDLNet_TCP_AddSocket(socketSet, client);
                clients.emplace_back(client);
                std::cout << "New client connected!" << std::endl;
            }
        }


        // 更新现有客户端
        UpdateClients(socketSet);
    }

    SDLNet_FreeSocketSet(socketSet);
    SDLNet_TCP_Close(server);
    SDLNet_Quit();
    SDL_Quit();
    return 0;
}

void UpdateClients(SDLNet_SocketSet socketSet) {
    for (auto it = clients.begin(); it != clients.end();) {
        if (SDLNet_SocketReady(it->get_socket())) {
            it->ReceiveMessage();
            if (!it->is_valid()) {
                it = clients.erase(it);
                SDLNet_TCP_DelSocket(socketSet, it->get_socket());
            }
            else {
                ++it;
            }
        }
        else {
            ++it;
        }
    }
}

void SendMessagesToAllClients() {
    for (auto& client : clients) {
        client.SendMessage(to_string(count));
    }
}

void DataReady(const std::string& message) {
    std::cout << "Received message: " << message << std::endl;
    count++;
    SendMessagesToAllClients();
}