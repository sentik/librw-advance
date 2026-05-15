#pragma once
#ifndef RW_OBJECT_H
#define RW_OBJECT_H

#include "rwbase.h"

namespace rw {

struct Object
{
	uint8 type;
	uint8 subType;
	uint8 flags;
	uint8 privateFlags;
	void *parent;

	void init(uint8 type, uint8 subType){
		this->type = type;
		this->subType = subType;
		this->flags = 0;
		this->privateFlags = 0;
		this->parent = nullptr;
	}
	void copy(Object *o){
		this->type = o->type;
		this->subType = o->subType;
		this->flags = o->flags;
		this->privateFlags = o->privateFlags;
		this->parent = nullptr;
	}
};

}

#endif // RW_OBJECT_H
