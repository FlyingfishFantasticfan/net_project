#pragma once
#include "vector2.h" 

//用ObjectData类来存储敌人、角色、子弹对象的位置和半径，并依据位置和半径来判断碰撞

class ObjectData
{
public:
	ObjectData(Vector2 position, float radius)
	{
		this->position = position;
		this->radius = radius;
	}


	static bool isOverlap(ObjectData& obj1, ObjectData& obj2)
	{
		float distance = sqrt(pow(obj1.position.x - obj2.position.x, 2) + pow(obj1.position.y - obj2.position.y, 2));
		return distance < obj1.radius + obj2.radius;
	}

public:
	Vector2 position;
	float radius;
	bool is_face_right = true;
};
