#ifndef __EDITOR_H__
#define __EDITOR_H__

#include <ExtLib.h>
#include <ExtGui/Interface.h>
#include <ExtGui/Collision.h>
#include "SkelAnime.h"
#include "Light.h"
#include "Scene.h"

typedef struct Editor {
	AppInfo app;
	void*   vg;
	GeoGrid geo;
	Cursor  cursor;
	Input   input;
	Scene   scene;
} Editor;

void* NewMtx();

Editor* GetEditor(void);
void Editor_Init(Editor* editor);
void Editor_DropCallback(GLFWwindow* window, s32 count, char* item[]);
void Editor_Update(Editor* editor);
void Editor_Draw(Editor* editor);

void* DataNode_Copy(DataContext* ctx, SceneCmd* cmd);
void DataNode_Free(DataContext* ctx, u8 code);

extern Gfx* gSetupDL;
#define gSetupDList(x) & gSetupDL[6 * x]

#endif