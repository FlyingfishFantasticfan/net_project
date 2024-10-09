#include "../thirdparty/httplib.h"
#include <mutex>
#include <cJSON.h>
#include <iostream>

std::mutex g_mutex;	// 全局互斥锁
int plate[10] = {0};	// 井字棋棋盘
int current_player_id = 1;	// 当前玩家
int current_player_number = 0;	// 当前玩家数量
int current_player_win = 0;	// 当前获胜玩家

bool is_player_1_win()
{
	if (plate[0] == 1 && plate[1] == 1 && plate[2] == 1)
	{
		return true;
	}
	if (plate[3] == 1 && plate[4] == 1 && plate[5] == 1)
	{
		return true;
	}
	if (plate[6] == 1 && plate[7] == 1 && plate[8] == 1)
	{
		return true;
	}
	if (plate[0] == 1 && plate[3] == 1 && plate[6] == 1)
	{
		return true;
	}
	if (plate[1] == 1 && plate[4] == 1 && plate[7] == 1)
	{
		return true;
	}
	if (plate[2] == 1 && plate[5] == 1 && plate[8] == 1)
	{
		return true;
	}
	if (plate[0] == 1 && plate[4] == 1 && plate[8] == 1)
	{
		return true;
	}
	if (plate[2] == 1 && plate[4] == 1 && plate[6] == 1)
	{
		return true;
	}
	return false;
}

bool is_player_2_win()
{
	if (plate[0] == 2 && plate[1] == 2 && plate[2] == 2)
	{
		return true;
	}
	if (plate[3] == 2 && plate[4] == 2 && plate[5] == 2)
	{
		return true;
	}
	if (plate[6] == 2 && plate[7] == 2 && plate[8] == 2)
	{
		return true;
	}
	if (plate[0] == 2 && plate[3] == 2 && plate[6] == 2)
	{
		return true;
	}
	if (plate[1] == 2 && plate[4] == 2 && plate[7] == 2)
	{
		return true;
	}
	if (plate[2] == 2 && plate[5] == 2 && plate[8] == 2)
	{
		return true;
	}
	if (plate[0] == 2 && plate[4] == 2 && plate[8] == 2)
	{
		return true;
	}
	if (plate[2] == 2 && plate[4] == 2 && plate[6] == 2)
	{
		return true;
	}
	return false;
}

int main()
{
	httplib::Server svr;

	svr.Post("/login", [&](const httplib::Request& req, httplib::Response& res)
		{
			std::lock_guard<std::mutex> lock(g_mutex);

			if (current_player_number == 2)
			{
				res.set_content("-1", "text/plain");
				return;
			}

			res.set_content(current_player_number == 0 ? "1" : "2", "text/plain");// 返回玩家ID
			current_player_number++;
		});

	svr.Post("/update", [&](const httplib::Request& req, httplib::Response& res)
		{
			std::lock_guard<std::mutex> lock(g_mutex);

			cJSON* json_plate = cJSON_CreateIntArray(plate, 10);

			char* json_str = cJSON_Print(json_plate);
			res.set_content(json_str, "application/json");

			cJSON_Delete(json_plate);
			free(json_str);
		});

	svr.Post("/move", [&](const httplib::Request& req, httplib::Response& res)
		{
			if (current_player_id == std::stoi(req.body) / 10)// 确认当前玩家
			{
				int point = std::stoi(req.body) % 10;// 获取落子位置
				
				plate[point] = current_player_id;
				if (is_player_1_win())
				{
					plate[9] = 1;
				}
				if (is_player_2_win())
				{
					plate[9] = 2;
				}
				current_player_id = current_player_id == 1 ? 2 : 1;// 切换玩家
			}
		});

	svr.listen("0.0.0.0", 25565);

	return 0;

}