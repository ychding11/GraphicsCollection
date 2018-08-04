#ifndef  IDATA_SOURCE_H 
#define  IDATA_SOURCE_H 

#include <vector>
#include <map>
#include <string>


enum ePrimitiveTopology
{
    ePoint = 0,
    eLine,
    eTriangleList,
    eControlPoint_4,
    eControlPoint_16,
};

class IDataSource
{

public:

   // Index buffer address
   virtual void* IBuffer() = 0;
    
   // Vertex buffer address
   virtual void* VBuffer() = 0;

   // Index buffer size in byte
   virtual size_t IBufferSize() = 0;
    
   // Vndex buffer size in byte
   virtual size_t VBufferSize() = 0;

   // Index buffer size in element
   virtual size_t IBufferElement() = 0;

   virtual ePrimitiveTopology PrimitiveTopology() = 0;

};

class UtahTeapot : public IDataSource
{
   // Index buffer address
   void* IBuffer() override;
    
   // Vertex buffer address
   void* VBuffer() override;

   // Index buffer size in byte
   size_t IBufferSize() override ;
    
   // Vndex buffer size in byte
   size_t VBufferSize() override;

   // Index buffer size in element
   size_t IBufferElement() override;

   ePrimitiveTopology PrimitiveTopology() override
   {
       return    eControlPoint_16;
   }
};

class Quad : public IDataSource
{
   // Index buffer address
   void* IBuffer() override;
    
   // Vertex buffer address
   void* VBuffer() override;

   // Index buffer size in byte
   size_t IBufferSize() override ;
    
   // Vndex buffer size in byte
   size_t VBufferSize() override;

   // Index buffer size in element
   size_t IBufferElement() override;

   ePrimitiveTopology PrimitiveTopology() override
   {
       return    eControlPoint_4;
   }
};

#endif