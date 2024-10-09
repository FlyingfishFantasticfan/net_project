#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <SDL_net.h>
#include <SDL_thread.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <thread>

int count = 0;
std::vector<TCPsocket> clients;

std::string to_string(int value) {
    std::ostringstream os;
    os << value;
    return os.str();
}

void SendMessages() {
    std::string response = "Count: " + to_string(count);
    int length = response.length() + 1;
    int networkLength = SDL_SwapBE32(length); // ת��Ϊ�����ֽ���
    for (auto it = clients.begin(); it != clients.end(); ++it) {
        // ������Ϣ����
        SDLNet_TCP_Send(*it, &networkLength, sizeof(networkLength));
        // ������Ϣ����
        SDLNet_TCP_Send(*it, response.c_str(), length);
    }
}

int ClientThread(void* data) {
    TCPsocket client = static_cast<TCPsocket>(data);
    bool running = true;
    char buffer[2048];
    int bytesInBuffer = 0;

    while (running) {
        int received = SDLNet_TCP_Recv(client, buffer + bytesInBuffer, sizeof(buffer) - bytesInBuffer);
        if (received <= 0) {
            running = false;
            break;
        }
        bytesInBuffer += received;

        int bufferOffset = 0;

        while (bytesInBuffer - bufferOffset >= sizeof(int)) // ����Ƿ����㹻����������ȡ��Ϣ����
        {
            int networkLength;
            memcpy(&networkLength, buffer + bufferOffset, sizeof(int));
            int length = SDL_SwapBE32(networkLength); // ת��Ϊ�����ֽ���

            // ����Ƿ���յ������İ�����û�����˳�ѭ���ȴ���һ�ν���
            if (bytesInBuffer - bufferOffset < sizeof(int) + length) {
                break;
            }

            // ��ȡ��Ϣ����
            char* messageBuffer = new char[length];
            memcpy(messageBuffer, buffer + bufferOffset + sizeof(int), length);

            std::string message(messageBuffer, length);
            delete[] messageBuffer;

            // ������յ�����Ϣ
            std::cout << "Received message: " << message << std::endl;

            // ���»�����ƫ����
            bufferOffset += sizeof(int) + length;

            count++;
            SendMessages();
        }

        // �ƶ�δ��������ݵ�����������ʼλ��
        bytesInBuffer -= bufferOffset;
        memmove(buffer, buffer + bufferOffset, bytesInBuffer);

        // �ȴ������ٽ�����Ϣ
        std::this_thread::sleep_for(std::chrono::seconds(1));
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

    while (true) {
        // �����µĿͻ�������
        TCPsocket client = SDLNet_TCP_Accept(server);
        if (client) {
            clients.push_back(client);
            std::cout << "New client connected!" << std::endl;
            SDL_Thread* thread = SDL_CreateThread(ClientThread, "ClientThread", (void*)client);
        }
    }

    SDLNet_TCP_Close(server);
    SDLNet_Quit();
    SDL_Quit();
    return 0;
}
