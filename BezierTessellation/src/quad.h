#ifndef UQUAD_TEAPOT_HTAH_TEAPOT_H
#define UQUAD_TEAPOT_HTAH_TEAPOT_H

// Teapot data
typedef unsigned int   uint32_t;
typedef unsigned short uint16_t;

static uint32_t quadIndex[4] = { 0, 1, 2, 3 };

static float    quadVertices[4][3] =
#if 0
{
    { -1.0f, -1.0f, 0.5f },
    { -1.0f,  1.0f, 0.5f },
    {  1.0f, -1.0f, 0.5f },
    {  1.0f,  1.0f, 0.5f }
};
#else
{
    { -1.f,  .0f, -1.f },
    { -1.f,  .0f, 1.f },
    { 1.f,   .0f, -1.f },
    { 1.f,   .0f, 1.f }
};
#endif

#endif