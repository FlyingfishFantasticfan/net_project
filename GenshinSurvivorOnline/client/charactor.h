#pragma once

#include "vector2.h"
#include "animation.h"
#include "cJSON.h"

class Charactor
{
public:
    Charactor() = default;

    void set_atlas(Atlas* atlas_left, Atlas* atlas_right)
	{
        anim_left.set_loop(true);
        anim_left.set_interval(0.1f);
        anim_left.add_frame(atlas_left);

        anim_right.set_loop(true);
        anim_right.set_interval(0.1f);
        anim_right.add_frame(atlas_right);

        current_anim = &anim_right;
    }
	

    void set_position(const Vector2& position)
    {
        this->position = position;
        current_anim->set_position(position);
    }

    void set_face_right(bool is_face_right)
    {
        if (is_face_right)
            current_anim = &anim_right;
        else
            current_anim = &anim_left;
    }

    void on_update(float delta)
    {
        current_anim->on_update(delta);
    }

    void on_render()
    {
        current_anim->set_position(position);
        current_anim->on_render();
    }

    void updateFromJson(const cJSON* charactor_json)
    {
        if (!charactor_json) return;

        cJSON* x_json = cJSON_GetObjectItem(charactor_json, "x");
        cJSON* y_json = cJSON_GetObjectItem(charactor_json, "y");
        cJSON* is_right_json = cJSON_GetObjectItem(charactor_json, "is_right");

        if (x_json && y_json) {
            int x = x_json->valueint;
            int y = y_json->valueint;
            set_position(Vector2(x, y));
        }

        if (is_right_json) {
            bool is_right = is_right_json->valueint;
            set_face_right(is_right);
        }
    }

private:
    Animation anim_left;
    Animation anim_right;
    Animation* current_anim = nullptr;
    Vector2 position;
    bool is_face_right = true;
};