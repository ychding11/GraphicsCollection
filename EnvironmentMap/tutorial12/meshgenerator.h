#include <vector>
#include <iostream>


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
    std::vector<SphereVertex> mVertex;
    std::vector<int> mIndex;
	MeshType mType;

    //MeshGenerator(MeshType type = LINE_LIST)
    MeshGenerator(MeshType type = TRIANGLE_LIST)
		: mType(type)
    {
        if(type == TRIANGLE_LIST)  generateShpereTriangleMesh(32, 64);
		else generateShpere(16, 32);
        //generateCircle();
    }

#define PI 3.1415926f

	void generateCircle(int slice = 32)
	{
		mVertex.clear();
		mIndex.clear();
		mNumIndex = mNumVertex = 0;

		float det = (2 * PI) / (float) slice;
		float r = 1.0;
												// index
		for (int j = 0; j < slice; ++j)
		{
			float phi = det * j;
			float x = r  * cos(phi);
			float y = r  * sin(phi);
			SphereVertex tmp(x, 0, y);
			mVertex.push_back(tmp);
		}

		for (int j = 0; j < slice; ++j)
		{
			int a = j;
			int b = (a + 1) % slice;
			mIndex.push_back(a);
			mIndex.push_back(b);
		}
		mNumIndex = mIndex.size();
		mNumVertex = mVertex.size();
	}

    void generateShpere(int stack = 8, int slice = 16)
    {
        const float Ydet = PI / stack;
        const float Xdet = (2 * PI) / slice;
        const float r = 1.0;
        mVertex.clear();
        mIndex.clear();
        mNumIndex = mNumVertex = 0;
        for (int i = 0; i <= stack; ++i)
        {
            float theta = Ydet * i;
            float z = r * cos(theta);
            for (int j = 0; j < slice; ++j)
            {
                float phi = Xdet * j;
                float x = r * sin(theta) * cos(phi);
                float y = r * sin(theta) * sin(phi);
                SphereVertex tmp(x, z, y);
                mVertex.push_back(tmp);
            }
        }
       // mVertex.push_back(SphereVertex(0, r, 0)); // top
        //mVertex.push_back(SphereVertex(0, -r, 0)); // bottom

        // index
        int n = mVertex.size() / slice;
        const int stride = slice;
        for (int i = 0; i < n - 1; ++i)
        {
			int offset = i * stride;
            for (int j = 0; j < slice; ++j)
            {
                int a = offset + j;
                int b = offset + (j + 1) % slice;
                int c = a + stride;
                mIndex.push_back(a);
                mIndex.push_back(b);
                mIndex.push_back(a);
                mIndex.push_back(c);
            }
        }
#if 0
        int top = n - 2;
        for (int i = 0; i < slice; ++i)
        {
            mIndex.push_back(top);
            mIndex.push_back(i);

        }
        int bottom = n - 1;
        for (int i = 0; i < slice; ++i)
        {
            int b = stride * (stack - 3);
            mIndex.push_back(i + b);
            mIndex.push_back(bottom);
        }
#endif
        mNumIndex = mIndex.size();
        mNumVertex = mVertex.size();
    }
	void generateShpereTriangleMesh(int stack = 8, int slice = 16)
	{
		const float Ydet = PI / stack;
		const float Xdet = (2 * PI) / slice;
		const float r = 1.0;
		mVertex.clear();
		mIndex.clear();
		mNumIndex = mNumVertex = 0;
		for (int i = 0; i <= stack; ++i)
		{
			float theta = Ydet * i;
			float z = r * cos(theta);
			float texY = i * (1.0 / (float)stack);
			for (int j = 0; j <= slice; ++j)
			{
				float phi = Xdet * j;
				float x = r * sin(theta) * cos(phi);
				float y = r * sin(theta) * sin(phi);
				float texX = j * (1.0 / (float)slice);
				SphereVertex tmp(x, z, y, texX, texY);
				mVertex.push_back(tmp);
			}
		}
		// mVertex.push_back(SphereVertex(0, r, 0)); // top
		//mVertex.push_back(SphereVertex(0, -r, 0)); // bottom

		// index
		int n = mVertex.size() / slice;
		const int stride = slice;
		for (int i = 0; i < n - 1; ++i)
		{
			int offset = i * stride;
			for (int j = 0; j <= slice; ++j)
			{
				int a = offset + j;
				int b = offset + (j + 1) % (slice+1);
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
#if 0
		int top = n - 2;
		for (int i = 0; i < slice; ++i)
		{
			mIndex.push_back(top);
			mIndex.push_back(i);

		}
		int bottom = n - 1;
		for (int i = 0; i < slice; ++i)
		{
			int b = stride * (stack - 3);
			mIndex.push_back(i + b);
			mIndex.push_back(bottom);
		}
#endif
		mNumIndex = mIndex.size();
		mNumVertex = mVertex.size();
	}
};