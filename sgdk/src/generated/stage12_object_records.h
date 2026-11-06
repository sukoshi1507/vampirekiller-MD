/* Literal Z80 output (C420..C5FF) recorded in VK-gpt.zip. NOT interpreted or wired to SGDK. */
#ifndef VK_STAGE12_OBJECT_RECORDS_H
#define VK_STAGE12_OBJECT_RECORDS_H
#include "../room_manager.h"
#define VK_STAGE12_RAW_OBJECT_COUNT 36
#define VK_STAGE12_RAW_ROOM_COUNT 12
/* slot is the original Z80 object-slot address; raw is the unchanged 16-byte slot. */
typedef struct { unsigned short slot; VKByte raw[16]; } VKStage12RawObject;
extern const VKStage12RawObject vk_stage12_raw_objects[VK_STAGE12_RAW_OBJECT_COUNT];
extern const VKByte vk_stage12_raw_object_start[13];
#endif
