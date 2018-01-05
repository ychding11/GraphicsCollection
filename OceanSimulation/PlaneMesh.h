#include<vector>

struct PlaneVertex
{
    float x, y, z;
    float nx, ny, nz;
    float tx, ty;

    PlaneVertex(float p1 = 0.0, float p2=0, float p3=0, float n1=0, float n2=0, float n3 = 0, float t1=0, float t2=0)
        : x(p1), y(p2), z(p3),nx(n1), ny(n2), nz(n3), tx(t1), ty(t2)
    { }
};

class PlaneMesh
{
public:
    std::vector<PlaneVertex> mVertexBuffer;
    std::vector<int> mIndexBuffer;
    int mXDivision;
    int mYDivision;

public:
    PlaneMesh(int xv = 16, int yv = 16)
        : mXDivision(xv)
        , mYDivision(yv)
    {
        generateMesh();
    }

private:
    void generateMesh()
    {
        //generate vertex buffer
        for (int i = 0; i < mYDivision; ++i)
        {
            for (int j = 0; j < mXDivision; ++j)
            {
                PlaneVertex vertex(j * (1.0/mXDivision), i * (1.0 / mYDivision), 0.2, 0, 1, 0, (float)j / (float)mXDivision, (float)i / (float)mYDivision );
                mVertexBuffer.push_back(vertex);
            }
        }
        //generate index buffer
        for (int i = 0; i < mYDivision - 1; ++i)
        {
            for (int j = 0; j < mXDivision - 1; ++j)
            {
                int a = i * mXDivision + j;
                int b = (i + 1) * mXDivision + j;
                int c = b + 1;
                int d = a + 1;
                mIndexBuffer.push_back(a);
                mIndexBuffer.push_back(b);
                mIndexBuffer.push_back(c);

                mIndexBuffer.push_back(a);
                mIndexBuffer.push_back(c);
                mIndexBuffer.push_back(d);
            }
        }
    }
};