
#include "IDataSource.h"              
#include "utahTeapot.h"

// Index buffer address
void* UtahTeapot::IBuffer()
{
    return teapotPatches;
}
    
// Vertex buffer address
void* UtahTeapot::VBuffer()
{
    return teapotVertices;
}

// Index buffer size in byte
size_t UtahTeapot::IBufferSize()
{
    return sizeof(teapotPatches);
}
    
// Vndex buffer size in byte
size_t UtahTeapot::VBufferSize()
{
    return sizeof(teapotVertices);
}

// Index buffer size in element
size_t UtahTeapot::IBufferElement()
{
    return kTeapotNumPatches * 16;
}