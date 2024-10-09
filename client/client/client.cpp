#include <httplib.h>

#include <graphics.h>
#include <cJSON.h>


#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>

#pragma comment(linker, "/subsystem:windows /entry:mainCRTStartup")

int player_id = 0;// 玩家ID

std::string str_address;			// 服务器地址
httplib::Client* client = nullptr;	// HTTP客户端对象
int plate[10] = { 0 };	// 井字棋棋盘


void load_resources(HWND hwnd);// 加载资源
void login_to_server(HWND hwnd);// 登录到服务器
void draw_plate();// 绘制棋盘

int main()
{
	HWND hwnd = initgraph(720, 720/*, EW_SHOWCONSOLE*/);
	SetWindowText(hwnd, _T("井字棋"));


	load_resources(hwnd);
	login_to_server(hwnd);

	ExMessage msg;



	BeginBatchDraw();

	while (true)
	{

		///处理玩家输入///
		while (peekmessage(&msg))
		{
			if (msg.message == WM_LBUTTONDOWN)
			{
				int x = msg.x;
				int y = msg.y;
				// 计算点击的格子索引
				int row = y / 240;
				int col = x / 240;
				int index = row * 3 + col;
				// 检查点击的格子是否为空
				if (plate[index] == 0)
				{
					int move_info = player_id * 10 + index;

					std::string route = "/move";
					std::string body = std::to_string(move_info);
					httplib::Result result = client->Post(route.c_str(), body, "text/plain");

				}
			}
		}


		///处理更新///
		if (plate[9] == 1)
		{
			MessageBox(hwnd, _T("玩家1获胜！"), _T("游戏结束"), MB_OK | MB_ICONINFORMATION);
			break;
		}
		else if (plate[9] == 2)
		{
			MessageBox(hwnd, _T("玩家2获胜！"), _T("游戏结束"), MB_OK | MB_ICONINFORMATION);
			break;
		}


		///绘制///
		setbkcolor(RGB(0, 0, 0));
		cleardevice();

		draw_plate();


		FlushBatchDraw();
	}

	EndBatchDraw();
	return 0;
}


void load_resources(HWND hwnd)// 加载资源
{
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

	if (player_id <= 0)
	{
		MessageBox(hwnd, _T("比赛已经开始啦！"), _T("拒绝加入"), MB_OK | MB_ICONERROR);
		exit(-1);
	}

	std::thread([&]()
		{
			while (true)
			{
				using namespace std::chrono;

				std::string route = "/update";
				httplib::Result result = client->Post(route);

				if (result && result->status == 200)
				{
					cJSON* json_plate = cJSON_Parse(result->body.c_str());
					if (json_plate)
					{
						for (int i = 0; i < 10; ++i)
						{
							cJSON* item = cJSON_GetArrayItem(json_plate, i);
							if (item)
							{
								plate[i] = item->valueint;
							}
						}
						cJSON_Delete(json_plate);

					}
				}
				std::this_thread::sleep_for(nanoseconds(1000000000 / 10));

			}
		}).detach();
}

void draw_plate()// 绘制棋盘
{
	setlinecolor(RGB(255, 255, 255));
	setlinestyle(PS_SOLID, 2);
	line(240, 0, 240, 720);
	line(480, 0, 480, 720);
	line(0, 240, 720, 240);
	line(0, 480, 720, 480);

	for (int i = 0; i < 9; ++i)
	{
		int x_center = 240 * (i % 3) + 120; // 计算格子中心的x坐标
		int y_center = 240 * (i / 3) + 120; // 计算格子中心的y坐标

		if (plate[i] == 1)
		{
			setlinecolor(RGB(255, 0, 0));
			setlinestyle(PS_SOLID, 2);
			circle(x_center, y_center, 60); // 调整圆的半径和位置
		}
		else if (plate[i] == 2)
		{
			setlinecolor(RGB(0, 0, 255));
			setlinestyle(PS_SOLID, 2);
			line(x_center - 40, y_center - 40, x_center + 40, y_center + 40); // 调整线条位置
			line(x_center + 40, y_center - 40, x_center - 40, y_center + 40); // 调整线条位置
		}
	}
}




