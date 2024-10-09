#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_net.h>
#include <SDL_thread.h>
#include <iostream>
#include <sstream>
#include <ctime>
#include <thread>

std::string to_string(std::time_t value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

int ReceiveMessages(void* data) {
    TCPsocket client = static_cast<TCPsocket>(data);
    while (true) {
        int networkLength;
        // ��ȡ��Ϣ����
        if (SDLNet_TCP_Recv(client, &networkLength, sizeof(networkLength)) <= 0) {
            break;
        }
        int length = SDL_SwapBE32(networkLength); // ת��Ϊ�����ֽ���

        // ��ȡ��Ϣ����
        char* buffer = new char[length];
        if (SDLNet_TCP_Recv(client, buffer, length) <= 0) {
            delete[] buffer;
            break;
        }

        std::string message(buffer, length);
        delete[] buffer;

        // ������յ�����Ϣ
        std::cout << "Server response: " << message << std::endl;
    }
    SDLNet_TCP_Close(client);
    return 0;
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
    if (SDLNet_ResolveHost(&ip, "127.0.0.1", 1234) == -1) {
        std::cerr << "SDLNet_ResolveHost error" << std::endl;
        SDLNet_Quit();
        SDL_Quit();
        return 3;
    }

    TCPsocket client = SDLNet_TCP_Open(&ip);
    if (!client) {
        std::cerr << "SDLNet_TCP_Open error" << std::endl;
        SDLNet_Quit();
        SDL_Quit();
        return 4;
    }

    SDL_Thread* thread = SDL_CreateThread(ReceiveMessages, "ReceiveMessages", (void*)client);
    if (!thread) {
        std::cerr << "SDL_CreateThread error" << std::endl;
        SDLNet_TCP_Close(client);
        SDLNet_Quit();
        SDL_Quit();
        return 5;
    }

    while (true) {
        std::time_t now = std::time(0);
        std::string timestamp = to_string(now);
        int length = timestamp.length() + 1;
        int networkLength = SDL_SwapBE32(length); // ת��Ϊ�����ֽ���

        // ������Ϣ����
        SDLNet_TCP_Send(client, &networkLength, sizeof(networkLength));
        // ������Ϣ����
        SDLNet_TCP_Send(client, timestamp.c_str(), length);

        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    SDLNet_TCP_Close(client);
    SDLNet_Quit();
    SDL_Quit();
    return 0;
}