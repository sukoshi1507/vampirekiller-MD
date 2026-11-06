/* Read-only view of original Z80 object slots, with NO gameplay interpretation.
 * Inputs and bytes come exclusively from the archived 5A49/5B1B execution. */
#ifndef VK_STAGE12_OBJECT_VIEW_H
#define VK_STAGE12_OBJECT_VIEW_H
#include "generated/stage12_object_records.h"
/* Returns number of original non-zero memory slots in one room; invalid room -> 0. */
unsigned vk_stage12_object_count(unsigned room);
/* Returns NULL for invalid room/index; never returns a record from another room. */
const VKStage12RawObject *vk_stage12_object_at(unsigned room, unsigned index);
/* Raw byte offsets as written by the Z80, not interpreted item properties. */
#endif
