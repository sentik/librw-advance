#pragma once
#ifndef RW_FRAME_H
#define RW_FRAME_H

#include "rwbase.h"
#include "rwplg.h"
#include "rwobject.h"

namespace rw {

struct Frame
{
	PLUGINBASE
	typedef Frame *(*Callback)(Frame *f, void *data);
	enum { ID = 0 };
	enum {		// private flags
		// The hierarchy has unsynched frames
		HIERARCHYSYNCLTM = 0x01,	// LTM not synched
		HIERARCHYSYNCOBJ = 0x02,	// attached objects not synched
		HIERARCHYSYNC    = HIERARCHYSYNCLTM  | HIERARCHYSYNCOBJ,
		// This frame is not synched
		SUBTREESYNCLTM   = 0x04,
		SUBTREESYNCOBJ   = 0x08,
		SUBTREESYNC      = SUBTREESYNCLTM | SUBTREESYNCOBJ,
		SYNCLTM          = HIERARCHYSYNCLTM | SUBTREESYNCLTM,
		SYNCOBJ          = HIERARCHYSYNCOBJ | SUBTREESYNCOBJ
		// STATIC = 0x10
	};

	Object object;
	LLLink inDirtyList;
	LinkList objectList;
	Matrix matrix;
	Matrix ltm;

	Frame *child;
	Frame *next;
	Frame *root;

	static int32 numAllocated;

	static Frame *create(void);
	Frame *cloneHierarchy(void);
	void destroy(void);
	void destroyHierarchy(void);
	Frame *addChild(Frame *f, bool32 append = 0);
	Frame *removeChild(void);
	Frame *forAllChildren(Callback cb, void *data);
	Frame *getParent(void) const {
		return (Frame*)this->object.parent; }
	int32 count(void);
	bool32 dirty(void) const {
		return !!(this->root->object.privateFlags & HIERARCHYSYNC); }
	Matrix *getLTM(void);
	void rotate(const V3d *axis, float32 angle, CombineOp op = rw::COMBINEPOSTCONCAT);
	void rotate(const Quat *q, CombineOp op = rw::COMBINEPOSTCONCAT);
	void translate(const V3d *trans, CombineOp op = rw::COMBINEPOSTCONCAT);
	void scale(const V3d *scale, CombineOp op = rw::COMBINEPOSTCONCAT);
	void transform(const Matrix *mat, CombineOp op = rw::COMBINEPOSTCONCAT);
	void updateObjects(void);


	void syncHierarchyLTM(void);
	void setHierarchyRoot(Frame *root);
	Frame *cloneAndLink(void);
	void purgeClone(void);

#ifndef RWPUBLIC
	static void registerModule(void);
#endif
	static void syncDirty(void);
};

struct FrameList_
{
	int32 numFrames;
	Frame **frames;

	FrameList_ *streamRead(Stream *stream);
	void streamWrite(Stream *stream);
	static uint32 streamGetSize(Frame *f);
};
Frame **makeFrameList(Frame *frame, Frame **flist);

struct ObjectWithFrame
{
	typedef void (*Sync)(ObjectWithFrame*);

	Object object;
	LLLink inFrame;
	Sync syncCB;

	void setFrame(Frame *f){
		if(this->object.parent)
			this->inFrame.remove();
		this->object.parent = f;
		if(f){
			f->objectList.add(&this->inFrame);
			f->updateObjects();
		}
	}
	void sync(void){ this->syncCB(this); }
	static ObjectWithFrame *fromFrame(LLLink *lnk){
		return LLLinkGetData(lnk, ObjectWithFrame, inFrame);
	}
};

}

#endif // RW_FRAME_H
