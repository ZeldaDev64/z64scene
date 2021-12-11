#include "Editor.h"

/* / / / / / / / / / / / / / / / / / / / / / / / / / / */

char* Debug_Split_GetStateStrings(Split* split) {
	static char buffer[1024];
	s32 t = 0;
	char* states[] = {
		"NONE",
		"POINT_BL",
		"POINT_TL",
		"POINT_TR",
		"POINT_BR",
		"SIDE_L",
		"SIDE_T",
		"SIDE_R",
		"SIDE_B",
	};
	
	String_Copy(buffer, states[0]);
	
	for (s32 i = 0; (1 << i) <= 0xFFFF; i++) {
		if (split->stateFlag & (1 << i)) {
			if (t++ == 0) {
				String_Copy(buffer, states[i + 1]);
			} else {
				String_Merge(buffer, "|");
				String_Merge(buffer, states[i + 1]);
			}
		}
	}
	
	return buffer;
}

/* / / / / / / / / / / / / / / / / / / / / / / / / / / */

SplitVtx* Split_AddVertex(GuiContext* guiCtx, s16 x, s16 y) {
	SplitVtx* head = guiCtx->vtxHead;
	
	while (head) {
		if (head->pos.x == x && head->pos.y == y) {
			OsPrintfEx("VtxConnect %d %d", x, y);
			
			return head;
		}
		head = head->next;
	}
	
	SplitVtx* vtx = Lib_Calloc(0, sizeof(SplitVtx));
	
	vtx->pos.x = x;
	vtx->pos.y = y;
	Node_Add(guiCtx->vtxHead, vtx);
	
	return vtx;
}

SplitEdge* Split_AddEdge(GuiContext* guiCtx, SplitVtx* v1, SplitVtx* v2) {
	SplitEdge* head = guiCtx->edgeHead;
	SplitEdge* edge = NULL;
	
	OsAssert(v1 != NULL && v2 != NULL);
	
	if (v1->pos.y == v2->pos.y) {
		if (v1->pos.x > v2->pos.x) {
			Lib_Swap(v1, v2);
		}
	} else {
		if (v1->pos.y > v2->pos.y) {
			Lib_Swap(v1, v2);
		}
	}
	
	while (head) {
		if (head->vtx[0] == v1 && head->vtx[1] == v2) {
			OsPrintfEx("EdgeConnect");
			
			edge = head;
			break;
		}
		head = head->next;
	}
	
	if (edge == NULL) {
		edge = Lib_Calloc(0, sizeof(SplitEdge));
		
		edge->vtx[0] = v1;
		edge->vtx[1] = v2;
		Node_Add(guiCtx->edgeHead, edge);
	}
	
	if (edge->vtx[0]->pos.y == edge->vtx[1]->pos.y) {
		edge->state |= EDGE_HORIZONTAL;
		edge->pos = edge->vtx[0]->pos.y;
		if (edge->pos < guiCtx->workRect.y + 1) {
			edge->state |= EDGE_STICK_T;
		}
		if (edge->pos > guiCtx->workRect.y + guiCtx->workRect.h - 1) {
			edge->state |= EDGE_STICK_B;
		}
	} else {
		edge->state |= EDGE_VERTICAL;
		edge->pos = edge->vtx[0]->pos.x;
		if (edge->pos < guiCtx->workRect.x + 1) {
			edge->state |= EDGE_STICK_L;
		}
		if (edge->pos > guiCtx->workRect.x + guiCtx->workRect.w - 1) {
			edge->state |= EDGE_STICK_R;
		}
	}
	
	return edge;
}

void Split_AddSplit(GuiContext* guiCtx, Rect* rect) {
	Split* split = Lib_Calloc(0, sizeof(Split));
	
	split->vtx[VTX_BOT_L] = Split_AddVertex(
		guiCtx,
		rect->x,
		rect->y + rect->h
	);
	split->vtx[VTX_TOP_L] = Split_AddVertex(
		guiCtx,
		rect->x,
		rect->y
	);
	split->vtx[VTX_TOP_R] = Split_AddVertex(
		guiCtx,
		rect->x + rect->w,
		rect->y
	);
	split->vtx[VTX_BOT_R] = Split_AddVertex(
		guiCtx,
		rect->x + rect->w,
		rect->y + rect->h
	);
	
	split->edge[EDGE_L] = Split_AddEdge(guiCtx, split->vtx[VTX_BOT_L], split->vtx[VTX_TOP_L]);
	split->edge[EDGE_T] = Split_AddEdge(guiCtx, split->vtx[VTX_TOP_L], split->vtx[VTX_TOP_R]);
	split->edge[EDGE_R] = Split_AddEdge(guiCtx, split->vtx[VTX_TOP_R], split->vtx[VTX_BOT_R]);
	split->edge[EDGE_B] = Split_AddEdge(guiCtx, split->vtx[VTX_BOT_R], split->vtx[VTX_BOT_L]);
	
	Node_Add(guiCtx->splitHead, split);
}

/* / / / / / / / / / / / / / / / / / / / / / / / / / / */

s32 Split_Cursor_GetDistTo(SplitState flag, Split* split) {
	Vec2s mouse[] = {
		{ split->mousePos.x, split->mousePos.y },
		{ split->mousePos.x, split->mousePos.y },
		{ split->mousePos.x, split->mousePos.y },
		{ split->mousePos.x, split->mousePos.y },
		{ split->mousePos.x, 0                 },
		{ 0,                 split->mousePos.y },
		{ split->mousePos.x, 0                 },
		{ 0,                 split->mousePos.y },
	};
	Vec2s pos[] = {
		{ 0,             split->rect.h, },
		{ 0,             0,             },
		{ split->rect.w, 0,             },
		{ split->rect.w, split->rect.h, },
		{ 0,             0,             },
		{ 0,             0,             },
		{ split->rect.w, 0,             },
		{ 0,             split->rect.h, },
	};
	s32 i;
	
	for (i = 0; (1 << i) <= SPLIT_SIDE_B; i++) {
		if (flag & (1 << i)) {
			break;
		}
	}
	
	return Vec_Vec2s_DistXZ(&mouse[i], &pos[i]);
}

bool Split_Cursor_InSplit(Split* split) {
	s32 resX = (split->mousePos.x < split->rect.w && split->mousePos.x >= 0);
	s32 resY = (split->mousePos.y < split->rect.h && split->mousePos.y >= 0);
	
	return (resX && resY);
}

SplitState Split_GetState_CursorPos(Split* split, s32 range) {
	for (s32 i = 0; (1 << i) <= SPLIT_SIDE_B; i++) {
		if (Split_Cursor_GetDistTo((1 << i), split) <= range) {
			return (1 << i);
		}
	}
	
	return SPLIT_POINT_NONE;
}

void Split_UpdateRect(Split* split) {
	split->rect = (Rect) {
		floor(split->vtx[1]->pos.x),
		floor(split->vtx[1]->pos.y),
		
		floor(split->vtx[3]->pos.x) - floor(split->vtx[1]->pos.x),
		floor(split->vtx[3]->pos.y) - floor(split->vtx[1]->pos.y)
	};
}

void Split_Reset(GuiContext* guiCtx) {
	OsAssert(guiCtx->actionSplit);
	guiCtx->actionSplit->stateFlag &= ~(
		SPLIT_POINTS | SPLIT_SIDES
	);
	
	guiCtx->actionSplit = NULL;
}

void Split_Update_ActionSplit(GuiContext* guiCtx) {
	const char* stringDirection[] = {
		"EDGE_L",
		"EDGE_T",
		"EDGE_R",
		"EDGE_B",
	};
	const char* stringEdgeState[] = {
		"EDGE_HORIZONTAL",
		"EDGE_VERTICAL",
		"EDGE_STICK_L",
		"EDGE_STICK_T",
		"EDGE_STICK_R",
		"EDGE_STICK_B",
	};
	Split* split = guiCtx->actionSplit;
	
	if (split->mousePress) {
		split->stateFlag |= Split_GetState_CursorPos(split, 20);
		if (split->stateFlag & SPLIT_SIDES) {
			s32 i;
			
			for (i = 0; i < 4; i++) {
				if (split->stateFlag & (1 << (4 + i))) {
					break;
				}
			}
			
			#ifndef NDEBUG
			char buffer[1024] = "None";
			s32 t = 0;
			OsPrintfEx("Split: %s", stringDirection[i]);
			
			for (s32 j = 0; (1 << j) <= EDGE_STICK_B; j++) {
				if (split->edge[i]->state & (1 << j)) {
					if (t++ == 0) {
						String_Copy(buffer, stringEdgeState[j]);
					} else {
						String_Merge(buffer, "|");
						String_Merge(buffer, stringEdgeState[j]);
					}
				}
			}
			
			OsPrintf("Edge: [%08X] [%s]", split->edge[i], buffer);
			
			#endif
			
			OsAssert(split->edge[i] != NULL);
			
			guiCtx->actionEdge = split->edge[i];
		}
	}
	
	if (__inputCtx->mouse.click.hold) {
		if (split->stateFlag & SPLIT_POINTS) {
		}
	}
}

void Split_Update_Vtx(GuiContext* guiCtx) {
	SplitVtx* vtx = guiCtx->vtxHead;
	
	while (vtx) {
		vtx = vtx->next;
	}
}

void Split_Update_Edges(GuiContext* guiCtx) {
	SplitEdge* edge = guiCtx->edgeHead;
	f64 diffCentX = (f64)guiCtx->workRect.w / guiCtx->prevWorkRect.w;
	f64 diffCentY = (f64)guiCtx->workRect.h / guiCtx->prevWorkRect.h;
	
	if (!__inputCtx->mouse.cursorAction) {
		guiCtx->actionEdge = NULL;
	}
	
	if (guiCtx->actionEdge) {
		if (guiCtx->actionEdge->state & EDGE_HORIZONTAL) {
			guiCtx->actionEdge->pos = __inputCtx->mouse.pos.y;
		}
		if (guiCtx->actionEdge->state & EDGE_VERTICAL) {
			guiCtx->actionEdge->pos = __inputCtx->mouse.pos.x;
		}
	}
	
	while (edge) {
		OsAssert(!(edge->state & EDGE_HORIZONTAL && edge->state & EDGE_VERTICAL));
		
		if (edge->state & EDGE_STICK) {
			if (edge->state & EDGE_STICK_L) {
				edge->pos = guiCtx->workRect.x;
			}
			if (edge->state & EDGE_STICK_T) {
				edge->pos = guiCtx->workRect.y;
			}
			if (edge->state & EDGE_STICK_R) {
				edge->pos = guiCtx->workRect.x + guiCtx->workRect.w;
			}
			if (edge->state & EDGE_STICK_B) {
				edge->pos = guiCtx->workRect.y + guiCtx->workRect.h;
			}
		} else {
			if (edge->state & EDGE_HORIZONTAL) {
				edge->pos *= diffCentY;
			}
			if (edge->state & EDGE_VERTICAL) {
				edge->pos *= diffCentX;
			}
		}
		
		if (edge->state & EDGE_HORIZONTAL) {
			edge->vtx[0]->pos.y = edge->pos;
			edge->vtx[1]->pos.y = edge->pos;
		}
		if (edge->state & EDGE_VERTICAL) {
			edge->vtx[0]->pos.x = edge->pos;
			edge->vtx[1]->pos.x = edge->pos;
		}
		
		edge = edge->next;
	}
}

void Split_Update_Splits(EditorContext* editorCtx) {
	GuiContext* guiCtx = &editorCtx->guiCtx;
	Split* split = guiCtx->splitHead;
	MouseInput* mouse = &editorCtx->inputCtx.mouse;
	
	Cursor_SetCursor(CURSOR_DEFAULT);
	
	if (guiCtx->actionSplit != NULL && mouse->cursorAction == false) {
		Split_Reset(guiCtx);
	}
	
	while (split) {
		Split_UpdateRect(split);
		Vec2s rectPos = {
			split->rect.x,
			split->rect.y
		};
		Vec_Vec2s_Substract(
			&split->mousePos,
			&mouse->pos,
			(Vec2s*)&rectPos
		);
		
		split->mouseInRegion = Split_Cursor_InSplit(split);
		split->center.x = split->rect.x + split->rect.w * 0.5f;
		split->center.y = split->rect.y + split->rect.h * 0.5f;
		
		if (guiCtx->actionSplit == NULL &&
		    split->mouseInRegion &&
		    mouse->cursorAction) {
			if (mouse->click.press) {
				split->mousePressPos = split->mousePos;
				split->mousePress = true;
			}
			guiCtx->actionSplit = split;
		}
		
		if (guiCtx->actionSplit) {
			Split_Update_ActionSplit(guiCtx);
		} else {
			if (Split_GetState_CursorPos(split, 20) & SPLIT_POINTS && split->mouseInRegion) {
				// editorCtx->inputCtx.mouse.cursorIcon = CURSOR_CROSSHAIR;
			} else if (Split_GetState_CursorPos(split, 10) & SPLIT_SIDE_H && split->mouseInRegion) {
				Cursor_SetCursor(CURSOR_ARROW_H);
			} else if (Split_GetState_CursorPos(split, 10) & SPLIT_SIDE_V && split->mouseInRegion) {
				Cursor_SetCursor(CURSOR_ARROW_V);
			}
		}
		
		if (split->update) {
			if (split != guiCtx->splitHead &&
			    ((guiCtx->actionSplit && guiCtx->actionSplit == split) ||
			    guiCtx->actionSplit == NULL)) {
				split->update(editorCtx, split);
			}
		}
		
		split->mousePress = false;
		split = split->next;
	}
}

/* / / / / / / / / / / / / / / / / / / / / / / / / / / */

void Split_Draw_SplitBorder(NVGcontext* vg, Rect* rect) {
	nvgBeginPath(vg);
	nvgRect(
		vg,
		0,
		0,
		rect->w,
		rect->h
	);
	nvgRoundedRect(
		vg,
		0 + SPLIT_SPLIT_W,
		0 + SPLIT_SPLIT_W,
		rect->w - SPLIT_SPLIT_W * 2,
		rect->h - SPLIT_SPLIT_W * 2,
		SPLIT_ROUND_R
	);
	nvgPathWinding(vg, NVG_HOLE);
	nvgFillColor(vg, Theme_GetColor(THEME_SPLITTER));
	nvgFill(vg);
}

void Split_Draw_Splits(EditorContext* editorCtx) {
	GuiContext* guiCtx = &editorCtx->guiCtx;
	Split* split = guiCtx->splitHead;
	MouseInput* mouse = &editorCtx->inputCtx.mouse;
	Vec2s* winDim = &editorCtx->appInfo.winDim;
	
	while (split != NULL) {
		glViewport(
			split->rect.x,
			winDim->y - split->rect.y - split->rect.h,
			split->rect.w,
			split->rect.h
		);
		nvgBeginFrame(editorCtx->vg, split->rect.w, split->rect.h, 1.0f); {
			if (split->draw)
				split->draw(editorCtx, split);
			
			Split_Draw_SplitBorder(editorCtx->vg, &split->rect);
		} nvgEndFrame(editorCtx->vg);
		
		split = split->next;
	}
}

/* / / / / / / / / / / / / / / / / / / / / / / / / / / */

void Gui_UpdateWorkRegion(EditorContext* editorCtx) {
	GuiContext* guiCtx = &editorCtx->guiCtx;
	Vec2s* winDim = &editorCtx->appInfo.winDim;
	
	guiCtx->workRect = (Rect) {
		0,
		0 + guiCtx->bar[GUI_BAR_TOP].rect.h,
		winDim->x,
		winDim->y - guiCtx->bar[GUI_BAR_BOT].rect.h - guiCtx->bar[GUI_BAR_TOP].rect.h
	};
}

void Gui_SetTopBarHeight(EditorContext* editorCtx, s32 h) {
	GuiContext* guiCtx = &editorCtx->guiCtx;
	Vec2s* winDim = &editorCtx->appInfo.winDim;
	
	guiCtx->bar[GUI_BAR_TOP].rect.x = 0;
	guiCtx->bar[GUI_BAR_TOP].rect.y = 0;
	guiCtx->bar[GUI_BAR_TOP].rect.w = winDim->x;
	guiCtx->bar[GUI_BAR_TOP].rect.h = h;
	Gui_UpdateWorkRegion(editorCtx);
}

void Gui_SetBotBarHeight(EditorContext* editorCtx, s32 h) {
	GuiContext* guiCtx = &editorCtx->guiCtx;
	Vec2s* winDim = &editorCtx->appInfo.winDim;
	
	guiCtx->bar[GUI_BAR_BOT].rect.x = 0;
	guiCtx->bar[GUI_BAR_BOT].rect.y = winDim->y - h;
	guiCtx->bar[GUI_BAR_BOT].rect.w = winDim->x;
	guiCtx->bar[GUI_BAR_BOT].rect.h = h;
	Gui_UpdateWorkRegion(editorCtx);
}

/* / / / / / / / / / / / / / / / / / / / / / / / / / / */

void Gui_Init(EditorContext* editorCtx) {
	GuiContext* guiCtx = &editorCtx->guiCtx;
	Vec2s* winDim = &editorCtx->appInfo.winDim;
	
	Gui_SetTopBarHeight(editorCtx, 30);
	Gui_SetBotBarHeight(editorCtx, 30);
	
	Rect lHalf = {
		guiCtx->workRect.x, guiCtx->workRect.y,
		guiCtx->workRect.w / 2, guiCtx->workRect.h
	};
	Rect rHalf = {
		guiCtx->workRect.x + guiCtx->workRect.w / 2, guiCtx->workRect.y,
		guiCtx->workRect.w / 2, guiCtx->workRect.h
	};
	
	Split_AddSplit(guiCtx, &lHalf);
	Split_AddSplit(guiCtx, &rHalf);
	
	guiCtx->prevWorkRect = guiCtx->workRect;
}

void Gui_Update(EditorContext* editorCtx) {
	GuiContext* guiCtx = &editorCtx->guiCtx;
	
	Gui_SetTopBarHeight(editorCtx, guiCtx->bar[GUI_BAR_TOP].rect.h);
	Gui_SetBotBarHeight(editorCtx, guiCtx->bar[GUI_BAR_BOT].rect.h);
	Split_Update_Vtx(guiCtx);
	Split_Update_Edges(guiCtx);
	Split_Update_Splits(editorCtx);
	
	guiCtx->prevWorkRect = guiCtx->workRect;
}

void Gui_Draw(EditorContext* editorCtx) {
	GuiContext* guiCtx = &editorCtx->guiCtx;
	Vec2s* winDim = &editorCtx->appInfo.winDim;
	
	// Draw Bars
	for (s32 i = 0; i < 2; i++) {
		glViewport(
			guiCtx->bar[i].rect.x,
			winDim->y - guiCtx->bar[i].rect.y - guiCtx->bar[i].rect.h,
			guiCtx->bar[i].rect.w,
			guiCtx->bar[i].rect.h
		);
		nvgBeginFrame(editorCtx->vg, guiCtx->bar[i].rect.w, guiCtx->bar[i].rect.h, 1.0f); {
			nvgBeginPath(editorCtx->vg);
			nvgRect(
				editorCtx->vg,
				0,
				0,
				guiCtx->bar[i].rect.w,
				guiCtx->bar[i].rect.h
			);
			nvgFillColor(editorCtx->vg, Theme_GetColor(THEME_SPLITTER_DARKER));
			nvgFill(editorCtx->vg);
			
			if (i == 1) {
				static Split* lastActive;
				
				if (lastActive == NULL) {
					lastActive = guiCtx->splitHead;
				}
				
				if (guiCtx->actionSplit) {
					lastActive = guiCtx->actionSplit;
				}
				
				nvgFillColor(editorCtx->vg, Theme_GetColor(THEME_BASE_CONT));
				nvgFontSize(editorCtx->vg, 30 - 16);
				nvgFontFace(editorCtx->vg, "sans");
				nvgTextAlign(editorCtx->vg, NVG_ALIGN_LEFT | NVG_ALIGN_TOP);
				nvgText(
					editorCtx->vg,
					8,
					8,
					Debug_Split_GetStateStrings(lastActive),
					NULL
				);
			}
		} nvgEndFrame(editorCtx->vg);
	}
	
	Split_Draw_Splits(editorCtx);
	
	#ifndef NDEBUG
	if (guiCtx->actionEdge != NULL) {
		SplitEdge* edge = guiCtx->actionEdge;
		glViewport(0, 0, editorCtx->appInfo.winDim.x, editorCtx->appInfo.winDim.y);
		
		nvgBeginFrame(editorCtx->vg, editorCtx->appInfo.winDim.x, editorCtx->appInfo.winDim.y, 1.0f); {
			nvgBeginPath(editorCtx->vg);
			nvgLineCap(editorCtx->vg, NVG_ROUND);
			nvgStrokeWidth(editorCtx->vg, 1.5f);
			nvgMoveTo(editorCtx->vg, edge->vtx[0]->pos.x, edge->vtx[0]->pos.y);
			nvgLineTo(editorCtx->vg, edge->vtx[1]->pos.x, edge->vtx[1]->pos.y);
			nvgStrokeColor(editorCtx->vg, nvgRGBA(255, 0, 0, 255));
			nvgStroke(editorCtx->vg);
		} nvgEndFrame(editorCtx->vg);
	}
	#endif
}