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


// 游戏阶段
enum class Stage
{
	Waiting,	// 等待玩家加入
	Start,		// 游戏开始
};

//资源列表
Atlas atlas_player_left;
Atlas atlas_player_right;
Atlas atlas_enemy_left;
Atlas atlas_enemy_right;
IMAGE img_background;

std::string str_address;			// 服务器地址
httplib::Client* client = nullptr;	// HTTP客户端对象

int player_id = 0;	// 玩家ID
Stage stage = Stage::Waiting;	// 游戏阶段

bool is_W_pressed = false;
bool is_A_pressed = false;
bool is_S_pressed = false;
bool is_D_pressed = false;

Charactor player_1;	// 玩家1角色
Charactor player_2;	// 玩家2角色
EnemyManager enemy_manager;	// 敌人管理器
BulletManager bullet_manager; // 子弹管理器
int direction = 0;

void load_resources(HWND hwnd);// 加载资源
void login_to_server(HWND hwnd);// 登录到服务器

int main()
{
	HWND hwnd = initgraph(1280, 720);
	SetWindowText(hwnd, _T("联机版提瓦特幸存者"));

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



        //////////////////处理玩家输入/////////////////
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
                direction = 8; // 左上
            else if (is_D_pressed && !is_A_pressed)
                direction = 2; // 右上
            else
                direction = 1; // 上
        }
        else if (is_S_pressed && !is_W_pressed)
        {
            if (is_A_pressed && !is_D_pressed)
                direction = 6; // 左下
            else if (is_D_pressed && !is_A_pressed)
                direction = 4; // 右下
            else
                direction = 5; // 下
        }
        else if (is_A_pressed && !is_D_pressed)
        {
            if (!is_W_pressed && !is_S_pressed)
                direction = 7; // 左
        }
        else if (is_D_pressed && !is_A_pressed)
        {
            if (!is_W_pressed && !is_S_pressed)
                direction = 3; // 右
        }
		else
		{
			direction = 0; // 停止
		}
        std::cout<<"current direction" << direction << std::endl;
        //////////////////处理更新///////////////////



        if (stage == Stage::Start)
        {
            // 调用玩家和敌人的更新接口
            player_1.on_update(delta_time.count());
            player_2.on_update(delta_time.count());
        }
        

        //////////////////处理绘制///////////////////
        setbkcolor(RGB(0, 0, 0));
        cleardevice();

        putimage(0, 0, &img_background);
        if (stage == Stage::Waiting)
        {
            settextcolor(RGB(195, 195, 195));
            outtextxy(15, 675, _T("游戏即将开始，等待其他玩家加入..."));
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
		MessageBox(hwnd, L"无法打开配置 config.cfg", L"启动失败", MB_OK | MB_ICONERROR);
		exit(-1);
	}
	std::stringstream str_stream;
	str_stream << file.rdbuf();
	str_address = str_stream.str();
	file.close();
}

void login_to_server(HWND hwnd)// 登录到服务器
{
	client = new httplib::Client(str_address);
	client->set_keep_alive(true);


	httplib::Result result = client->Post("/login");
	if (!result || result->status != 200)
	{
		MessageBox(hwnd, _T("无法连接到服务器！"), _T("启动失败"), MB_OK | MB_ICONERROR);
		exit(-1);
	}

	player_id = std::stoi(result->body);
    std::cout << "player_id: " << player_id << std::endl;

	if (player_id <= 0)
	{
		MessageBox(hwnd, _T("游戏已经开始啦！"), _T("拒绝加入"), MB_OK | MB_ICONERROR);
		exit(-1);
	}

    std::thread([&]()
        {
            while (true)
            {
                static int count = 0;
                count++;
                Sleep(1000/60);
                // 根据玩家序号发送 update 请求
                std::string route = (player_id == 1) ? "/update_1" : "/update_2";
                std::string body = std::to_string(direction);
                httplib::Result result = client->Post(route, body, "text/plain");

                std::cout << "stage: " << (int)stage << std::endl;

                if (result && result->status == 200)
                {
                    // 解析返回的 JSON 数据
                    cJSON* root = cJSON_Parse(result->body.c_str());
                    if (root)
                    {
                        // 更新游戏阶段
                        cJSON* stage_item = cJSON_GetObjectItem(root, "game_stage");
                        if (stage_item)
                        {
                            int new_stage = stage_item->valueint;
                            std::cout << "new_stage: " << new_stage << std::endl;
                            if (new_stage == -1)
                            {
                                MessageBox(hwnd, _T("游戏结束！"), _T("提示"), MB_OK | MB_ICONINFORMATION);
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
                            // 更新玩家信息
                            cJSON* _player_1 = cJSON_GetObjectItem(root, "player_1");
                            player_1.updateFromJson(_player_1);
                            // 更新玩家信息
                            cJSON* _player_2 = cJSON_GetObjectItem(root, "player_2");
                            player_2.updateFromJson(_player_2);

                            // 更新子弹信息
                            cJSON* bullets = cJSON_GetObjectItem(root, "bullets");
                            if (bullets)
                            {
                                bullet_manager.updateFromJson(bullets);
                            }

                            // 更新敌人信息
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