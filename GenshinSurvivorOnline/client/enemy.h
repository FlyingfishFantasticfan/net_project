#pragma once

#include "vector2.h"
#include "animation.h"

class Enemy
{
public:
	Enemy(Atlas* atlas_left, Atlas* atlas_right)
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

	void set_active(bool is_active)
	{
		_is_active = is_active;
	}

	bool is_active() const
	{
		return _is_active;
	}

private:
	Animation anim_left;
	Animation anim_right;
	Animation* current_anim = nullptr;
	Vector2 position;
	bool is_face_right = true;
	bool _is_active = false;
};
