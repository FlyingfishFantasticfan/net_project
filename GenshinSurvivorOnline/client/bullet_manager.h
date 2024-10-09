#pragma once
#include "vector2.h"
#include "cJSON.h"
#include <vector>
#include <graphics.h> // 假设使用 EasyX 库进行渲染

class BulletManager
{
public:
    BulletManager()
    {
        bullets_positions.resize(6); // 初始化子弹位置为屏幕外
    }

    void updateFromJson(const cJSON* bullets_json)
    {
        if (!bullets_json) return;

        int bullet_count = 0;
        cJSON* bullet_json = nullptr;
        cJSON_ArrayForEach(bullet_json, bullets_json)
        {
            int x = cJSON_GetObjectItem(bullet_json, "x")->valueint;
            int y = cJSON_GetObjectItem(bullet_json, "y")->valueint;
            bullets_positions[bullet_count] = Vector2(x, y);
            bullet_count++;
        }

    }

    void on_render()
    {
        setlinecolor(RGB(255, 155, 50));
        setfillcolor(RGB(200, 75, 10));

        for (const auto& position : bullets_positions) {
            fillcircle(position.x, position.y, RADIUS);
        }
    }

private:
    static constexpr int RADIUS = 10; // 假设子弹的半径为 5
    std::vector<Vector2> bullets_positions;
};
