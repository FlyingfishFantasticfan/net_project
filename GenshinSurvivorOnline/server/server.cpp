#include <httplib.h>
#include <cJSON.h>

#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <fstream>
#include <sstream>

#include "object_data.h"


std::mutex g_mutex;		// ȫ�ֻ�����

//��Ϸ���ݲ���
int game_stage = 0;//��Ϸ�׶�,0Ϊ׼���׶Σ�1Ϊ��Ϸ�У�-1Ϊ��Ϸ������
int current_player_number = 0;//��ǰ���������

const float PLAYER_SPEED = 3.0f;//��ɫ�ƶ��ٶȣ�
const float PLAYER_RADIUS = 20;//��ɫ�뾶��

const float ENEMY_SPEED = 2.0f;//�����ƶ��ٶȣ�
const float ENEMY_RADIUS = 20;//���˰뾶��

const double RADTAL_SPEED = 0.0025;//�ӵ����򲨶��ٶȣ�
const double TANGENT_SPEED = 0.0035;//�ӵ����򲨶��ٶȣ�
const float BULLET_RADIUS = 5;//�ӵ��뾶��

ObjectData player_1(Vector2(600, 360), PLAYER_RADIUS);//��ɫ1����
int direction_1 = 0;//��ɫ1�ƶ����򣻷�ΧΪ0-8��Ӧ�����ƶ��ͽ�ֹ��

ObjectData player_2(Vector2(700, 360), PLAYER_RADIUS);//��ɫ2����
int direction_2 = 0;//��ɫ1�ƶ����򣻷�ΧΪ0-8��Ӧ�����ƶ��ͽ�ֹ��

std::vector<ObjectData> enemies;//���˶���

std::vector<ObjectData> bullets_1 = {
	ObjectData(Vector2(0, 0), BULLET_RADIUS),
	ObjectData(Vector2(0, 0), BULLET_RADIUS),
	ObjectData(Vector2(0, 0), BULLET_RADIUS)
};//��ɫ1�ӵ�����

std::vector<ObjectData> bullets_2 = {
	ObjectData(Vector2(0, 0), BULLET_RADIUS),
	ObjectData(Vector2(0, 0), BULLET_RADIUS),
	ObjectData(Vector2(0, 0), BULLET_RADIUS)
};//��ɫ2�ӵ�����

//��Ϸ���ݸ��º���
void update_game();
void update_players();
void update_bullets();
void update_bullets(std::vector<ObjectData>& bullets, const ObjectData& player, double radial_speed, double tangent_speed);
void update_enemys();

int main(int argc, char** argv)
{
	httplib::Server server;

	server.Post("/login", [&](const httplib::Request& req, httplib::Response& res)
		{
			std::lock_guard<std::mutex> lock(g_mutex);

			if (current_player_number >= 2)
			{
				res.set_content("-1", "text/plain");
				return;
			}

			current_player_number++;
			if(current_player_number == 2)
				game_stage = 1;
			std::cout << "current_player_number: " << current_player_number << std::endl;
			std::cout << "game_stage: " << game_stage << std::endl;

			res.set_content(std::to_string(current_player_number), "text/plain");
		});

	server.Post("/update_1", [&](const httplib::Request& req, httplib::Response& res)
		{
			std::lock_guard<std::mutex> lock(g_mutex);

			direction_1 = std::stoi(req.body);

			// ���� JSON ����
			cJSON* root = cJSON_CreateObject();
			cJSON_AddNumberToObject(root, "game_stage", game_stage);

			// ��� player_1
			cJSON* player1 = cJSON_CreateObject();
			cJSON_AddNumberToObject(player1, "x", player_1.position.x);
			cJSON_AddNumberToObject(player1, "y", player_1.position.y);
			cJSON_AddBoolToObject(player1, "is_right", player_1.is_face_right);
			cJSON_AddItemToObject(root, "player_1", player1);

			// ��� player_2
			cJSON* player2 = cJSON_CreateObject();
			cJSON_AddNumberToObject(player2, "x", player_2.position.x);
			cJSON_AddNumberToObject(player2, "y", player_2.position.y);
			cJSON_AddBoolToObject(player2, "is_right", player_2.is_face_right);
			cJSON_AddItemToObject(root, "player_2", player2);

			// ��� enemies
			cJSON* enemies_json = cJSON_CreateObject();
			for (size_t i = 0; i < enemies.size(); ++i) {
				cJSON* enemy = cJSON_CreateObject();
				cJSON_AddNumberToObject(enemy, "x", enemies[i].position.x);
				cJSON_AddNumberToObject(enemy, "y", enemies[i].position.y);
				cJSON_AddBoolToObject(enemy, "is_right", enemies[i].is_face_right); 
				std::string enemy_name = "enemy_" + std::to_string(i + 1);
				cJSON_AddItemToObject(enemies_json, enemy_name.c_str(), enemy);
			}
			cJSON_AddItemToObject(root, "enemys", enemies_json);

			// ��� bullets
			cJSON* bullets_json = cJSON_CreateObject();
			for (size_t i = 0; i < bullets_1.size(); ++i) {
				cJSON* bullet = cJSON_CreateObject();
				cJSON_AddNumberToObject(bullet, "x", bullets_1[i].position.x);
				cJSON_AddNumberToObject(bullet, "y", bullets_1[i].position.y);
				std::string bullet_name = "bullet_" + std::to_string(i + 1);
				cJSON_AddItemToObject(bullets_json, bullet_name.c_str(), bullet);
			}
			for (size_t i = 0; i < bullets_2.size(); ++i) {
				cJSON* bullet = cJSON_CreateObject();
				cJSON_AddNumberToObject(bullet, "x", bullets_2[i].position.x);
				cJSON_AddNumberToObject(bullet, "y", bullets_2[i].position.y);
				std::string bullet_name = "bullet_" + std::to_string(bullets_1.size() + 1);
				cJSON_AddItemToObject(bullets_json, bullet_name.c_str(), bullet);
			}
			cJSON_AddItemToObject(root, "bullets", bullets_json);

			// �� JSON ����ת��Ϊ�ַ���
			char* json_str = cJSON_Print(root);

			// ������Ӧ����
			res.set_content(json_str, "application/json");

			// �ͷ� JSON ����
			cJSON_Delete(root);
			free(json_str);
		});

	server.Post("/update_2", [&](const httplib::Request& req, httplib::Response& res)
		{
			std::lock_guard<std::mutex> lock(g_mutex);

			direction_2 = std::stoi(req.body);

			// ���� JSON ����
			cJSON* root = cJSON_CreateObject();
			cJSON_AddNumberToObject(root, "game_stage", game_stage);

			// ��� player_1
			cJSON* player1 = cJSON_CreateObject();
			cJSON_AddNumberToObject(player1, "x", player_1.position.x);
			cJSON_AddNumberToObject(player1, "y", player_1.position.y);
			cJSON_AddBoolToObject(player1, "is_right", player_1.is_face_right);
			cJSON_AddItemToObject(root, "player_1", player1);

			// ��� player_2
			cJSON* player2 = cJSON_CreateObject();
			cJSON_AddNumberToObject(player2, "x", player_2.position.x);
			cJSON_AddNumberToObject(player2, "y", player_2.position.y);
			cJSON_AddBoolToObject(player2, "is_right", player_2.is_face_right);
			cJSON_AddItemToObject(root, "player_2", player2);

			// ��� enemies
			cJSON* enemies_json = cJSON_CreateObject();
			for (size_t i = 0; i < enemies.size(); ++i) {
				cJSON* enemy = cJSON_CreateObject();
				cJSON_AddNumberToObject(enemy, "x", enemies[i].position.x);
				cJSON_AddNumberToObject(enemy, "y", enemies[i].position.y);
				cJSON_AddBoolToObject(enemy, "is_right", enemies[i].is_face_right);
				std::string enemy_name = "enemy_" + std::to_string(i + 1);
				cJSON_AddItemToObject(enemies_json, enemy_name.c_str(), enemy);
			}
			cJSON_AddItemToObject(root, "enemys", enemies_json);

			// ��� bullets
			cJSON* bullets_json = cJSON_CreateObject();
			for (size_t i = 0; i < bullets_1.size(); ++i) {
				cJSON* bullet = cJSON_CreateObject();
				cJSON_AddNumberToObject(bullet, "x", bullets_1[i].position.x);
				cJSON_AddNumberToObject(bullet, "y", bullets_1[i].position.y);
				std::string bullet_name = "bullet_" + std::to_string(i + 1);
				cJSON_AddItemToObject(bullets_json, bullet_name.c_str(), bullet);
			}
			for (size_t i = 0; i < bullets_2.size(); ++i) {
				cJSON* bullet = cJSON_CreateObject();
				cJSON_AddNumberToObject(bullet, "x", bullets_2[i].position.x);
				cJSON_AddNumberToObject(bullet, "y", bullets_2[i].position.y);
				std::string bullet_name = "bullet_" + std::to_string(bullets_1.size() + 1);
				cJSON_AddItemToObject(bullets_json, bullet_name.c_str(), bullet);
			}
			cJSON_AddItemToObject(root, "bullets", bullets_json);

			// �� JSON ����ת��Ϊ�ַ���
			char* json_str = cJSON_Print(root);

			// ������Ӧ����
			res.set_content(json_str, "application/json");

			// �ͷ� JSON ����
			cJSON_Delete(root);
			free(json_str);
		});

	// �������ݸ����߳�
	std::thread([&]()
		{
			update_game();
		}).detach();
		

	// ��������������
	server.listen("0.0.0.0", 25565);


	return 0;
}

void update_game()
{
	constexpr std::chrono::milliseconds frame_duration(1000 / 60); // ÿ��60֡

	while (true) {
		auto frame_start = std::chrono::steady_clock::now();

		if (game_stage == 0) {
			std::this_thread::sleep_for(frame_duration);
			continue;
		}

		{
			std::lock_guard<std::mutex> lock(g_mutex);

			update_players();
			update_bullets();
			update_enemys();
		}

		auto frame_end = std::chrono::steady_clock::now();
		auto frame_time = std::chrono::duration_cast<std::chrono::milliseconds>(frame_end - frame_start);

		if (frame_time < frame_duration) {
			Sleep(frame_duration.count() - frame_time.count());
		}
	}
}

void update_players()
{
	// ����˸�����ĵ�λ����
	std::vector<Vector2> directions = {
		Vector2(0, 0),    // 0: ��ֹ
		Vector2(0, -1),   // 1: ��
		Vector2(1, -1),   // 2: ����
		Vector2(1, 0),    // 3: ��
		Vector2(1, 1),    // 4: ����
		Vector2(0, 1),    // 5: ��
		Vector2(-1, 1),   // 6: ����
		Vector2(-1, 0),   // 7: ��
		Vector2(-1, -1)   // 8: ����
	};


	// ���� player_1 ��λ��
	if (direction_1 >= 0 && direction_1 < directions.size())
		player_1.position += directions[direction_1].normalize() * PLAYER_SPEED;
		

	// ���� player_2 ��λ��
	if (direction_2 >= 0 && direction_2 < directions.size()) 
		player_2.position += directions[direction_2].normalize() * PLAYER_SPEED;
	

	// ��� player_1 �Ƿ���߽��ཻ
	if (player_1.position.x < PLAYER_RADIUS) {
		player_1.position.x = PLAYER_RADIUS;
	}
	else if (player_1.position.x > 1280 - PLAYER_RADIUS) {
		player_1.position.x = 1280 - PLAYER_RADIUS;
	}
	if (player_1.position.y < PLAYER_RADIUS) {
		player_1.position.y = PLAYER_RADIUS;
	}
	else if (player_1.position.y > 720 - PLAYER_RADIUS) {
		player_1.position.y = 720 - PLAYER_RADIUS;
	}
	// ��� player_2 �Ƿ���߽��ཻ
	if (player_2.position.x < PLAYER_RADIUS) {
		player_2.position.x = PLAYER_RADIUS;
	}
	else if (player_2.position.x > 1280 - PLAYER_RADIUS) {
		player_2.position.x = 1280 - PLAYER_RADIUS;
	}
	if (player_2.position.y < PLAYER_RADIUS) {
		player_2.position.y = PLAYER_RADIUS;
	}
	else if (player_2.position.y > 720 - PLAYER_RADIUS) {
		player_2.position.y = 720 - PLAYER_RADIUS;
	}

	//���ý�ɫ����
	if (direction_1 >= 1 && direction_1 <= 8)
	{
		if (direction_1 == 2 || direction_1 == 3 || direction_1 == 4)
		{
			player_1.is_face_right = true;
		}
		else if(direction_1 == 6 || direction_1 == 7 || direction_1 == 8)
		{
			player_1.is_face_right = false;
		}
	}
	if (direction_2 >= 1 && direction_2 <= 8)
	{
		if (direction_2 == 2 || direction_2 == 3 || direction_2 == 4)
		{
			player_2.is_face_right = true;
		}
		else if (direction_2 == 6 || direction_2 == 7 || direction_2 == 8)
		{
			player_2.is_face_right = false;
		}
	}
}

void update_bullets(std::vector<ObjectData>& bullets, const ObjectData& player, double radial_speed, double tangent_speed)
{
	double radian_interval = 2 * 3.14159 / bullets.size(); // �ӵ�֮��Ļ��ȼ��
	Vector2 player_position = player.position;
	double radius = 100 + 25 * sin(GetTickCount() * radial_speed); // ����뾶

	for (size_t i = 0; i < bullets.size(); i++)
	{
		double radian = GetTickCount() * tangent_speed + radian_interval * i;
		bullets[i].position.x = player_position.x + (int)(radius * sin(radian));
		bullets[i].position.y = player_position.y + (int)(radius * cos(radian));
	}
}

void update_bullets()
{
	update_bullets(bullets_1, player_1, RADTAL_SPEED, TANGENT_SPEED);
	update_bullets(bullets_2, player_2, RADTAL_SPEED, TANGENT_SPEED);
}

void update_enemys()
{
	static int count = 0;
	// ÿ�� 10 �ε������һ���µĵ���
	if (++count >= 50)
	{
		// ������ɵ��˵�λ��
		int edge = std::rand() % 4; // 0: ��, 1: ��, 2: ��, 3: ��
		float x, y;

		switch (edge)
		{
		case 0: // ��
			x = std::rand() % 1280;
			y = 0;
			break;
		case 1: // ��
			x = 1280;
			y = std::rand() % 720;
			break;
		case 2: // ��
			x = std::rand() % 1280;
			y = 720;
			break;
		case 3: // ��
			x = 0;
			y = std::rand() % 720;
			break;
		}

		enemies.push_back(ObjectData(Vector2(x, y), ENEMY_RADIUS));
		std::cout<< "enemy added"<<enemies.size() << std::endl;
		count = 0;
	}

	for (auto it = enemies.begin(); it != enemies.end(); )
	{
		// ������˵� player_1 �� player_2 �ľ���
		float distance_to_player_1 = it->position.distance(player_1.position);
		float distance_to_player_2 = it->position.distance(player_2.position);

		// ȷ����������
		ObjectData* nearest_player = (distance_to_player_1 < distance_to_player_2) ? &player_1 : &player_2;

		// ���㷽������
		Vector2 direction = nearest_player->position - it->position;
		direction = direction.normalize();

		// ���ó���
		if (direction.x > 0)
		{
			it->is_face_right = true;
		}
		else
		{
			it->is_face_right = false;
		}

		// ���µ���λ��
		it->position += direction * ENEMY_SPEED;

		bool erased = false;

		// ����� player_1/2 ���ص�
		if (ObjectData::isOverlap(*it, player_1)|| ObjectData::isOverlap(*it, player_2))
		{
			game_stage = -1;
			return;
		}

		// ����� bullets_1 ���ص�
		for (auto bullet_it = bullets_1.begin(); bullet_it != bullets_1.end(); ++bullet_it)
		{
			if (ObjectData::isOverlap(*it, *bullet_it))
			{
				it = enemies.erase(it);
				erased = true;
				break;
			}
		}

		if (!erased)
		{
			// ����� bullets_2 ���ص�
			for (auto bullet_it = bullets_2.begin(); bullet_it != bullets_2.end(); ++bullet_it)
			{
				if (ObjectData::isOverlap(*it, *bullet_it))
				{
					it = enemies.erase(it);
					erased = true;
					break;
				}
			}
		}

		if (!erased)
		{
			++it;
		}
		else
		{
			std::cout << "enemy erased" << enemies.size() << std::endl;
		}
	}
}