
#include "IDataSource.h"              
#include "utahTeapot.h"   
#include "quad.h"   

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

/////////////////////////////////////////////////////////////////////////
//Quad
/////////////////////////////////////////////////////////////////////////

// Index buffer address
void* Quad::IBuffer()
{
    return quadIndex;
}
    
// Vertex buffer address
void* Quad::VBuffer()
{
    return quadVertices;
}

// Index buffer size in byte
size_t Quad::IBufferSize()
{
    return sizeof(quadIndex);
}
    
// Vndex buffer size in byte
size_t Quad::VBufferSize()
{
    return sizeof(quadVertices);
}

// Index buffer size in element
size_t Quad::IBufferElement()
{
    return  4;
}

/////////////////////////////////////////////////////////////////////////
//EyePoint
/////////////////////////////////////////////////////////////////////////

static float dummyEyeVertex[1][3] = { { 1.f, 1.f, 1.f } };
static float dummyEyeIndex[1]  = { 0 };

// Index buffer address
void* EyePoint::IBuffer()
{
    return dummyEyeIndex;
}
    
// Vertex buffer address
void* EyePoint::VBuffer()
{
    return dummyEyeVertex;
}

// Index buffer size in byte
size_t EyePoint::IBufferSize()
{
    return sizeof(dummyEyeIndex);
}
    
// Vndex buffer size in byte
size_t EyePoint::VBufferSize()
{
    return sizeof(dummyEyeVertex);
}

// Index buffer size in element
size_t EyePoint::IBufferElement()
{
    return  1;
}
