#ifndef UQUAD_TEAPOT_HTAH_TEAPOT_H
#define UQUAD_TEAPOT_HTAH_TEAPOT_H

typedef unsigned int   uint32_t;

static uint32_t quadIndex[4] = { 0, 1, 2, 3 };

static float    quadVertices[4][3] =
{
    { -1.f,  .0f, -1.f },
    { -1.f,  .0f, 1.f },
    { 1.f,   .0f, -1.f },
    { 1.f,   .0f, 1.f }
};

#endif