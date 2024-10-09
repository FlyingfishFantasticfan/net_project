#pragma once
#include "enemy.h"
#include <vector>
#include "cJSON.h"

#pragma once
#include "enemy.h"
#include <vector>
#include "cJSON.h"

class EnemyManager
{
public:
    EnemyManager() = default;

    void set_atlas(Atlas* atlas_left, Atlas* atlas_right)
    {
        this->atlas_left = atlas_left;
        this->atlas_right = atlas_right;
    }

    void updateFromJson(cJSON* enemys)
    {
        if (!enemys) {
            return;
        }

        int enemy_count = cJSON_GetArraySize(enemys);
        ensureCapacity(enemy_count);

        for (int i = 0; i < enemy_count; ++i) {
            cJSON* enemy_json = cJSON_GetArrayItem(enemys, i);
            if (enemy_json) {
                cJSON* x_item = cJSON_GetObjectItem(enemy_json, "x");
                cJSON* y_item = cJSON_GetObjectItem(enemy_json, "y");
                cJSON* is_right_item = cJSON_GetObjectItem(enemy_json, "is_right");

                if (x_item && y_item && is_right_item) {
                    int x = x_item->valueint;
                    int y = y_item->valueint;
                    bool is_right = is_right_item->valueint;

                    enemies[i].set_position(Vector2(x, y));
                    enemies[i].set_face_right(is_right);
                    enemies[i].set_active(true);
                }
            }
        }

        // Deactivate extra enemies
        for (size_t i = enemy_count; i < enemies.size(); ++i) {
            enemies[i].set_position(Vector2(-1000, -1000)); // Move off-screen
            enemies[i].set_active(false);
        }
    }

    void on_update(float delta)
    {
        for (size_t i = 0; i < enemies.size(); ++i) {
            if (enemies[i].is_active()) {
                enemies[i].on_update(delta);
            }
        }
    }

    void on_render()
    {
        for (size_t i = 0; i < enemies.size(); ++i) {
            if (enemies[i].is_active()) {
                enemies[i].on_render();
            }
        }
    }

private:
    void ensureCapacity(int count)
    {
        if (count > enemies.size()) {
            enemies.resize(count, Enemy(atlas_left, atlas_right)); // Initialize with default atlas
        }
    }

private:
    Atlas* atlas_left;
    Atlas* atlas_right;

    std::vector<Enemy> enemies;
};
