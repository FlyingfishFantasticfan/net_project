#include <httplib.h>
#include <cJSON.h>

#include <graphics.h>

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>
#include "charactor.h"
#include "enemy_manager.h"
#include "bullet_manager.h"


// ��Ϸ�׶�
enum class Stage
{
	Waiting,	// �ȴ���Ҽ���
	Start,		// ��Ϸ��ʼ
};

//��Դ�б�
Atlas atlas_player_left;
Atlas atlas_player_right;
Atlas atlas_enemy_left;
Atlas atlas_enemy_right;
IMAGE img_background;

std::string str_address;			// ��������ַ
httplib::Client* client = nullptr;	// HTTP�ͻ��˶���

int player_id = 0;	// ���ID
Stage stage = Stage::Waiting;	// ��Ϸ�׶�

bool is_W_pressed = false;
bool is_A_pressed = false;
bool is_S_pressed = false;
bool is_D_pressed = false;

Charactor player_1;	// ���1��ɫ
Charactor player_2;	// ���2��ɫ
EnemyManager enemy_manager;	// ���˹�����
BulletManager bullet_manager; // �ӵ�������
int direction = 0;

void load_resources(HWND hwnd);// ������Դ
void login_to_server(HWND hwnd);// ��¼��������

int main()
{
	HWND hwnd = initgraph(1280, 720);
	SetWindowText(hwnd, _T("�������������Ҵ���"));

	load_resources(hwnd);
	login_to_server(hwnd);


	ExMessage msg;

	BeginBatchDraw();

    auto last_time = std::chrono::high_resolution_clock::now();

    while (true)
    {
        auto current_time = std::chrono::high_resolution_clock::now();
        std::chrono::duration<float> delta_time = current_time - last_time;
        last_time = current_time;



        //////////////////�����������/////////////////
        while (peekmessage(&msg))
        {
            if (msg.message == WM_KEYDOWN)
            {
                printf("key down: %d\n", msg.vkcode);
                switch (msg.vkcode)
                {
                case 0x57:
                    is_W_pressed = true;
                    break;
                case  0x41:
                    is_A_pressed = true;
                    break;
                case 0x53:
                    is_S_pressed = true;
                    break;
                case 0x44:
                    is_D_pressed = true;
                    break;
                }
            }
            else if (msg.message == WM_KEYUP)
            {
                switch (msg.vkcode)
                {
                case 0x57:
                    is_W_pressed = false;
                    break;
                case  0x41:
                    is_A_pressed = false;
                    break;
                case 0x53:
                    is_S_pressed = false;
                    break;
                case 0x44:
                    is_D_pressed = false;
                    break;
                }
            }
        }

        std::cout<<is_A_pressed<<is_D_pressed<<is_S_pressed<<is_W_pressed<<std::endl;

        if (is_W_pressed && !is_S_pressed)
        {
            if (is_A_pressed && !is_D_pressed)
                direction = 8; // ����
            else if (is_D_pressed && !is_A_pressed)
                direction = 2; // ����
            else
                direction = 1; // ��
        }
        else if (is_S_pressed && !is_W_pressed)
        {
            if (is_A_pressed && !is_D_pressed)
                direction = 6; // ����
            else if (is_D_pressed && !is_A_pressed)
                direction = 4; // ����
            else
                direction = 5; // ��
        }
        else if (is_A_pressed && !is_D_pressed)
        {
            if (!is_W_pressed && !is_S_pressed)
                direction = 7; // ��
        }
        else if (is_D_pressed && !is_A_pressed)
        {
            if (!is_W_pressed && !is_S_pressed)
                direction = 3; // ��
        }
		else
		{
			direction = 0; // ֹͣ
		}
        std::cout<<"current direction" << direction << std::endl;
        //////////////////�������///////////////////



        if (stage == Stage::Start)
        {
            // ������Һ͵��˵ĸ��½ӿ�
            player_1.on_update(delta_time.count());
            player_2.on_update(delta_time.count());
        }
        

        //////////////////�������///////////////////
        setbkcolor(RGB(0, 0, 0));
        cleardevice();

        putimage(0, 0, &img_background);
        if (stage == Stage::Waiting)
        {
            settextcolor(RGB(195, 195, 195));
            outtextxy(15, 675, _T("��Ϸ������ʼ���ȴ�������Ҽ���..."));
        }
        else
        {
            player_1.on_render();
            player_2.on_render();

            enemy_manager.on_render();
            bullet_manager.on_render();
        }

        FlushBatchDraw();
    }

	EndBatchDraw();
	return 0;
}

void load_resources(HWND hwnd)
{
	loadimage(&img_background, _T("img/background.png"));
	atlas_enemy_left.load(_T("img/enemy_left_%d.png"), 5);
	atlas_enemy_right.load(_T("img/enemy_right_%d.png"), 5);
	atlas_player_left.load(_T("img/player_left_%d.png"), 5);
	atlas_player_right.load(_T("img/player_right_%d.png"), 5);

    player_1.set_atlas(&atlas_player_left, &atlas_player_right);
    player_2.set_atlas(&atlas_player_left, &atlas_player_right);
    enemy_manager.set_atlas(&atlas_enemy_left, &atlas_enemy_right);

	std::ifstream file("config.cfg");
	if (!file.good())
	{
		MessageBox(hwnd, L"�޷������� config.cfg", L"����ʧ��", MB_OK | MB_ICONERROR);
		exit(-1);
	}
	std::stringstream str_stream;
	str_stream << file.rdbuf();
	str_address = str_stream.str();
	file.close();
}

void login_to_server(HWND hwnd)// ��¼��������
{
	client = new httplib::Client(str_address);
	client->set_keep_alive(true);


	httplib::Result result = client->Post("/login");
	if (!result || result->status != 200)
	{
		MessageBox(hwnd, _T("�޷����ӵ���������"), _T("����ʧ��"), MB_OK | MB_ICONERROR);
		exit(-1);
	}

	player_id = std::stoi(result->body);
    std::cout << "player_id: " << player_id << std::endl;

	if (player_id <= 0)
	{
		MessageBox(hwnd, _T("��Ϸ�Ѿ���ʼ����"), _T("�ܾ�����"), MB_OK | MB_ICONERROR);
		exit(-1);
	}

    std::thread([&]()
        {
            while (true)
            {
                static int count = 0;
                count++;
                Sleep(1000/60);
                // ���������ŷ��� update ����
                std::string route = (player_id == 1) ? "/update_1" : "/update_2";
                std::string body = std::to_string(direction);
                httplib::Result result = client->Post(route, body, "text/plain");

                std::cout << "stage: " << (int)stage << std::endl;

                if (result && result->status == 200)
                {
                    // �������ص� JSON ����
                    cJSON* root = cJSON_Parse(result->body.c_str());
                    if (root)
                    {
                        // ������Ϸ�׶�
                        cJSON* stage_item = cJSON_GetObjectItem(root, "game_stage");
                        if (stage_item)
                        {
                            int new_stage = stage_item->valueint;
                            std::cout << "new_stage: " << new_stage << std::endl;
                            if (new_stage == -1)
                            {
                                MessageBox(hwnd, _T("��Ϸ������"), _T("��ʾ"), MB_OK | MB_ICONINFORMATION);
                                exit(0);
                            }
                            else if (new_stage == 0)
                            {
                                stage = Stage::Waiting;
                                continue;
                            }
                            else if (new_stage == 1)
                            {
                                stage = Stage::Start;
                            }
                        }
                        else
                        {
                            std::cout << "Failed to get game_stage" << std::endl;
                        }

                        if (stage == Stage::Start)
                        {
                            // ���������Ϣ
                            cJSON* _player_1 = cJSON_GetObjectItem(root, "player_1");
                            player_1.updateFromJson(_player_1);
                            // ���������Ϣ
                            cJSON* _player_2 = cJSON_GetObjectItem(root, "player_2");
                            player_2.updateFromJson(_player_2);

                            // �����ӵ���Ϣ
                            cJSON* bullets = cJSON_GetObjectItem(root, "bullets");
                            if (bullets)
                            {
                                bullet_manager.updateFromJson(bullets);
                            }

                            // ���µ�����Ϣ
                            cJSON* enemys = cJSON_GetObjectItem(root, "enemys");
                            if (enemys)
                            {
                                enemy_manager.updateFromJson(enemys);
                            }
                        }

                        cJSON_Delete(root);
                    }
                    else
                    {
                        std::cout << "Failed to parse JSON data" << std::endl;
                    }
                }
                else
                {
                    std::cout << "Failed to update" << std::endl;
                }
            }
        }).detach();


    
}