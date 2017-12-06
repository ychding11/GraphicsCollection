#include <vector>
#include <iostream>


#define PI 3.1415926f

struct SphereVertex
{
    float vertex[3];
    float normal[3];
	float tex[2];
    float color[4];
    SphereVertex(float x, float y, float z, float tex1 = 0.5, float tex2 = 0.5)
    {
        vertex[0] = x; vertex[1] = y; vertex[2] = z;
        normal[0] = x; normal[1] = y; normal[2] = z;
		tex[0] = tex1; tex[1] = tex2;
        color[0] = 1.0; color[1] = 0.0; color[2] = 0.0; color[3] = 1.0;
    }
};

enum MeshType
{
	LINE_LIST,
	TRIANGLE_LIST,
};

class MeshGenerator
{
public:
    int mNumVertex;
    int mNumIndex;
    std::vector<int> mIndex;
	MeshType mType;

    MeshGenerator(MeshType type = TRIANGLE_LIST) : mType(type)
    { }
	virtual void generate() = 0;
};

class Sphere : public MeshGenerator
{

public:
	double mRadius;
	int    mStack;
	int    mSlice;
    std::vector<SphereVertex> mVertex;

	Sphere(double radius=1.0, int stack = 16, int slice = 32)
		:mRadius(radius),  mStack(stack), mSlice(slice)
	{
		generate();
	}

public:
	void generate() override
	{
		const float Ydet = PI / mStack;
		const float Xdet = (2.0 * PI) / mSlice;
		const float r = 1.0;
		mVertex.clear();
		mIndex.clear();
		mNumIndex = mNumVertex = 0;
		for (int i = 0; i <= mStack; ++i)
		{
			float theta = Ydet * i;
			float z = r * cos(theta);
			float texY = i * (1.0 / (float)mStack);
			for (int j = 0; j <= mSlice; ++j)
			{
				float phi = Xdet * j;
				float x = r * sin(theta) * cos(phi);
				float y = r * sin(theta) * sin(phi);
				float texX = j * (1.0 / (float)mSlice);
				SphereVertex tmp(x, z, y, texX, texY);
				mVertex.push_back(tmp);
			}
		}

		// index
		int n = mVertex.size() / mSlice;
		const int stride = mSlice;
		for (int i = 0; i < n - 1; ++i)
		{
			int offset = i * stride;
			for (int j = 0; j <= mSlice; ++j)
			{
				int a = offset + j;
				int b = offset + (j + 1) % (mSlice+1);
				int c = a + stride;
				int d = b + stride;
				mIndex.push_back(a);
				mIndex.push_back(b);
				mIndex.push_back(d);

				mIndex.push_back(a);
				mIndex.push_back(d);
				mIndex.push_back(c);
			}
		}
		mNumIndex  = mIndex.size();
		mNumVertex = mVertex.size();
	}
};

class Cylinder : public MeshGenerator
{
public:
	double mZmin;
	double mZmax;
	double mRadius;
	double mPhi;
	int    mStack;
	int    mSlice;
    std::vector<SphereVertex> mVertex;

	Cylinder(double zmin=0.0, double zmax=0.0, double radius=1.0, double phi=2*PI, int stack = 8, int slice = 16)
		:mZmin(zmin),mZmax(zmax),mRadius(radius), mPhi(phi), mStack(stack), mSlice(slice)
	{
		generate();
	}

public:
	void generate() override
	{

	}
};