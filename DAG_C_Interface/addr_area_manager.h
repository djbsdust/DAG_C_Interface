#ifndef _ADDR_AREA_MANAGER_H
#define _ADDR_AREA_MANAGER_H
#include <iostream>
#include <map>

namespace AceMesh_runtime {
//one input addr may have three addr:
//itself, shade_addr, unshade_addr
//
void* get_shade_addr(void* input_addr);
void* get_unshade_addr(void* input_addr);
}
#endif
