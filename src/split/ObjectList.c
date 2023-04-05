#include "ObjectList.h"
#include "Viewport.h"

void ObjectList_Init(Editor* editor, ObjectList* this, Split* split);
void ObjectList_Destroy(Editor* editor, ObjectList* this, Split* split);
void ObjectList_Update(Editor* editor, ObjectList* this, Split* split);
void ObjectList_Draw(Editor* editor, ObjectList* this, Split* split);

SplitTask gObjectListTask = {
    .taskName = "Object List",
    .init     = (void*)ObjectList_Init,
    .destroy  = (void*)ObjectList_Destroy,
    .update   = (void*)ObjectList_Update,
    .draw     = (void*)ObjectList_Draw,
    .size     = sizeof(ObjectList)
};

void ObjectList_Init(Editor* editor, ObjectList* this, Split* split) {
    this->list.showHexID = true;
    this->list.drag = true;
    this->list.textBox.size = 4;
}

void ObjectList_Destroy(Editor* editor, ObjectList* this, Split* split) {
}

void ObjectList_Update(Editor* editor, ObjectList* this, Split* split) {
    Scene* scene = &editor->scene;
    
    if (editor->scene.segment) {
        RoomHeader* room = Scene_GetRoomHeader(scene, scene->curRoom);
        
        Element_Container_SetArli(&this->list, &room->objectList, 4);
    } else
        Element_Disable(&this->list);
    
    this->list.element.heightAdd = (split->dispRect.h - SPLIT_ELEM_Y_PADDING * 2) - SPLIT_TEXT_H;
    split->scroll.offset = split->scroll.voffset = 0;
    
    Element_Header(split->taskCombo, 92);
    Element_Combo(split->taskCombo);
    
    Element_Row(&this->list, 1.0f);
    Element_Container(&this->list);
    int index;
    
    if ((index = Element_Container(&this->list)) > -1) {
        RoomHeader* room = Scene_GetRoomHeader(scene, scene->curRoom);
        u16* obj = Arli_At(&room->objectList, index);
        
        if (obj)
            *obj = shex(this->list.textBox.txt);
    }
}

void ObjectList_Draw(Editor* editor, ObjectList* this, Split* split) {
    
}
