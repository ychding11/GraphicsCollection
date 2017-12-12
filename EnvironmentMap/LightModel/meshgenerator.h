#include <vector>
#include <iostream>
#include "./tiny_obj_loader/tiny_obj_loader.h"

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
    int mVertexStride;
	MeshType mType;
    void *mVertexBuffer;
    std::vector<int> mIndex;

    MeshGenerator(int stride = 0, MeshType type = TRIANGLE_LIST)
        : mType(type), mVertexStride(stride), mVertexBuffer(nullptr)
        , mNumIndex(0), mIndex(0)
    { }
	virtual void generate() = 0;
};

class Sphere : public MeshGenerator
{

public:
	double mRadius;
    double mPhiMax;
    double mThetaMax;
    double mThetaMin;
	int    mStack;
	int    mSlice;
    std::vector<SphereVertex> mVertex;

	Sphere(int stack = 16, int slice = 32, double radius=1.0, double thetaMin=0.0, double thetaMax= PI, double phiMax=2 * PI)
		: MeshGenerator(sizeof(SphereVertex)), mRadius(radius),  mStack(stack), mSlice(slice)
        , mPhiMax(phiMax), mThetaMin(thetaMin), mThetaMax(thetaMax)
	{
		generate();
        mVertexBuffer = &mVertex[0];
	}

public:
	void generate() override
	{
		mVertex.clear();
		mIndex.clear();
		mNumIndex = mNumVertex = 0;
		for (int i = 0; i <= mStack; ++i)
		{
            float v = ((double)i / (double)mStack);
			float theta = v * (mThetaMax - mThetaMin) + mThetaMin;
			float z = mRadius * cos(theta);
			for (int j = 0; j <= mSlice; ++j)
			{
				float u = ((double)j / (double)mSlice);
				float phi = mPhiMax * u;
				float x = mRadius * sin(theta) * cos(phi);
				float y = mRadius * sin(theta) * sin(phi);
				SphereVertex tmp(x, z, y, u, v);
				mVertex.push_back(tmp);
			}
		}

		// index
		const int stride = mSlice + 1;
		for (int i = 0; i < mStack; ++i)
		{
			int offset = i * stride;
			for (int j = 0; j < mSlice; ++j)
			{
				int a = offset + j;
				int b = offset + (j + 1) % stride;
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

struct CylinderVertex
{
    float vertex[3];
    float normal[3];
    float tex[2];
    float color[4];
    CylinderVertex(float x, float y, float z, float nx, float ny, float nz, float tx = 0.5, float ty = 0.5)
    {
        vertex[0] = x;  vertex[1] = y;  vertex[2] = z;
        normal[0] = nx; normal[1] = ny; normal[2] = nz;
        tex[0] = tx; tex[1] = ty;
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
    bool   mClosed;
    std::vector<CylinderVertex> mVertex;

    Cylinder(int stack = 16, int slice = 32, double zmin = -1.0, double zmax = 1.0, double radius = 1.0, double phi = 2 * PI)
        : MeshGenerator(sizeof(CylinderVertex)), mZmin(zmin), mZmax(zmax), mRadius(radius), mPhi(phi), mStack(stack), mSlice(slice), mClosed(false)
	{
		generate();
        mVertexBuffer = &mVertex[0];
	}

public:
	void generate() override
	{
		mVertex.clear();
		mIndex.clear();
		mNumIndex = mNumVertex = 0;
        for (int i = 0; i <= mStack; ++i)
        {
            double z = mZmin + (mZmax - mZmin) * ((double)i / (double)mStack);
            double texY = ((double)i / (double)mStack);
            for (int j = 0; j <= mSlice; ++j)
            {
                double phi = mPhi *  ((double)j / (double)mSlice);
                double texX = ((double)j / (double)mSlice);
                double x = mRadius * cos(phi);
                double y = mRadius * sin(phi);
                CylinderVertex vertex(x, y, z, x, y, 0.0, texX, texY);
                mVertex.push_back(vertex);
            }
        }

        // index
		const int stride = mSlice + 1;
        for (int i = 0; i < mStack; ++i)
        {
            int offset = i * stride;
            for (int j = 0; j < mSlice; ++j)
            {
				int a = offset + j;
				int b = offset + (j + 1) % stride;
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
        if (mClosed)
        {

        }
		mNumIndex  = mIndex.size();
		mNumVertex = mVertex.size();
	}
};

//Cone
struct ConeVertex
{
    float vertex[3];
    float normal[3];
    float tex[2];
    float color[4];
    ConeVertex(float x, float y, float z, float nx, float ny, float nz, float tx = 0.5, float ty = 0.5)
    {
        vertex[0] = x;  vertex[1] = y;  vertex[2] = z;
        normal[0] = nx; normal[1] = ny; normal[2] = nz;
        tex[0] = tx; tex[1] = ty;
    }
};

class Cone : public MeshGenerator
{
public:
	double mZmin;
	double mZmax;
	double mRadius;
	double mPhi;
	int    mStack;
	int    mSlice;
    bool   mClosed;
    std::vector<ConeVertex> mVertex;

    Cone(int stack = 16, int slice = 32, double zmin = -1.0, double zmax = 1.0, double radius = 1.0, double phi = 2 * PI )
        : MeshGenerator(sizeof(ConeVertex)), mZmin(zmin), mZmax(zmax), mRadius(radius), mPhi(phi), mStack(stack), mSlice(slice), mClosed(false)
	{
		generate();
        mVertexBuffer = &mVertex[0];
	}

public:
	void generate() override
	{
		mVertex.clear();
		mIndex.clear();
		mNumIndex = mNumVertex = 0;
        for (int i = 0; i <= mStack; ++i)
        {
            double v = ((double)i / (double)mStack);
            double z = mZmin + (mZmax - mZmin) * v;
            for (int j = 0; j <= mSlice; ++j)
            {
                double u = ((double)j / (double)mSlice);
                double phi = mPhi *  u;
                double x = mRadius * (1.0 - v) * cos(phi);
                double y = mRadius * (1.0 - v) * sin(phi);
                ConeVertex vertex(x, y, z, x, y, 0.0, u, v);
                mVertex.push_back(vertex);
            }
        }

        // index
		const int stride = mSlice + 1;
        for (int i = 0; i < mStack; ++i)
        {
            int offset = i * stride;
            for (int j = 0; j < mSlice; ++j)
            {
				int a = offset + j;
				int b = offset + (j + 1) % stride;
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
        if (mClosed)
        {

        }
		mNumIndex  = mIndex.size();
		mNumVertex = mVertex.size();
	}
};


//Torus
struct TorusVertex
{
    float vertex[3];
    float normal[3];
    float tex[2];
    float color[4];
    TorusVertex(float x, float y, float z, float nx, float ny, float nz, float tx = 0.5, float ty = 0.5)
    {
        vertex[0] = x;  vertex[1] = y;  vertex[2] = z;
        normal[0] = nx; normal[1] = ny; normal[2] = nz;
        tex[0] = tx; tex[1] = ty;
    }
};

class Torus : public MeshGenerator
{
public:
	double mRingRadius;
	double mRadius;
	double mTheta;
	double mPhi;
	int    mStack;
	int    mSlice;
    std::vector<TorusVertex> mVertex;

    Torus(int stack = 16, int slice = 32, double ringradius = 0.25, double radius = 1.0, double theta = 2.0 * PI, double phi = 2.0 * PI )
        : MeshGenerator(sizeof(TorusVertex)), mRingRadius(ringradius), mTheta(theta), mRadius(radius), mPhi(phi), mStack(stack), mSlice(slice)
	{
		generate();
        mVertexBuffer = &mVertex[0];
	}

public:
	void generate() override
	{
		mVertex.clear();
		mIndex.clear();
		mNumIndex = mNumVertex = 0;
        for (int i = 0; i <= mStack; ++i)
        {
            double v = ((double)i / (double)mStack);
            for (int j = 0; j <= mSlice; ++j)
            {
                double u = ((double)j / (double)mSlice);
                double z = mRingRadius * sin(u * mPhi);
                double phi = mPhi *  u;
                double x = (mRadius + cos(mPhi * u) * mRingRadius ) * cos(v * mTheta);
                double y = (mRadius + cos(mPhi * u) * mRingRadius ) * sin(v * mTheta);

                TorusVertex vertex(x, y, z, x-mRadius*cos(u*mPhi), y-mRadius*sin(u*mPhi), z-mRingRadius, u, v);
                //TorusVertex vertex(x, y, z, x, y, 0, u, v);
                mVertex.push_back(vertex);
            }
        }

        // index
		const int stride = mSlice + 1;
        for (int i = 0; i < mStack; ++i)
        {
            int offset = i * stride;
            for (int j = 0; j < mSlice; ++j)
            {
				int a = offset + j;
				int b = offset + (j + 1) % stride;
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

//Obj Model
struct ObjVertex
{
    float vertex[3];
    float normal[3];
    float tex[2];
    float color[4];
    ObjVertex(float x, float y, float z, float nx, float ny, float nz, float tx = 0.5, float ty = 0.5)
    {
        vertex[0] = x;  vertex[1] = y;  vertex[2] = z;
        normal[0] = nx; normal[1] = ny; normal[2] = nz;
        tex[0] = tx; tex[1] = ty;
    }
};

class Obj : public MeshGenerator
{
public:
	std::string mObjPath;
    std::vector<ObjVertex> mVertex;

    Obj(std::string objpath = "dragon.obj" )
        : MeshGenerator(sizeof(ObjVertex)), mObjPath(objpath)
	{
		generate();
        mVertexBuffer = &mVertex[0];
	}

public:
	void generate() override
	{
		mVertex.clear();
		mIndex.clear();
		mNumIndex = mNumVertex = 0;

		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;

		std::string err;
		bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &err, mObjPath.c_str());

		if (!err.empty())
		{
			std::cerr << err << std::endl;
		}
		if (!ret)
		{
			exit(1);
		}

		// Loop over shapes
		for (size_t s = 0; s < shapes.size(); s++)
		{
			// Loop over faces(polygon)
			size_t index_offset = 0;

			for (size_t f = 0; f < shapes[s].mesh.num_face_vertices.size(); f++)
			{
				int fv = shapes[s].mesh.num_face_vertices[f];
				// Loop over vertices in the face.
				for (size_t v = 0; v < fv; v++)
				{
					// access to vertex
					tinyobj::index_t idx = shapes[s].mesh.indices[index_offset + v];
					tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
					tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
					tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];
					tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
					tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
					tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];
					//tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
					//tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];
					mVertex.push_back( ObjVertex(vx,vy,vz, nx,ny,nz ) );
					mIndex.push_back(idx.vertex_index);
				}
				index_offset += fv;
			}
		}

		mNumIndex  = mIndex.size();
		mNumVertex = mVertex.size();
	}
};
