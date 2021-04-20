#include <iostream>

namespace AceMesh_runtime {
void* get_shade_addr(void* input_addr)
{
    return (void*)((char*)input_addr + 1);
}
void* get_unshade_addr(void* input_addr)
{
	/*NOTE!!! about UNSHADE address: NOT +2 anymore, but +0                      */
	/*since a mesh variable cannot have patch-based neighbours                   */
	/*and non-patched mesh neighbors in one single application                   */
	/*AM I WRONG?    TODO:                                                       */
	/*driven by the non-OO interface(clang interface),                           */
	/* how to choose the area-type for neighbors data items ?                    */
	
	/*In the previous OO interface, they are unified or explicitly specified     */
	/*in one single tuple         */
	/*But NOw, these are scattered in different function calls, and in           */
    /* _acemesh_task_generator_with_neighbor(), users do not specify area type!  */

	/*1) non-patch stencils, one in _acemesh_task_generator_with_neighbors       */ 
    /* and the other in **_push_wlist, they should both use NORMAL type.         */
	/*2) in patch-based apps, they cannot be NORMAL types, but MUST use UNSHADE, */
	/*   OR else no dep edges between stencil and exchanges.                     */
	
	/*Now we let UNSHADE==NORMAL in respect to true address computation,         */
    /* by chenli */
    return (void*)((char*)(input_addr) + 0); 
}
}
