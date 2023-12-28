#pragma once
#include "smartpointerhelp.h"
#include "glm_includes.h"
#include <array>
#include <unordered_map>
#include <cstddef>
#include <drawable.h>
#include <mutex>


class BoundingBox
{
public:
    glm::vec3 pMax, pMin;
    BoundingBox();

    bool ifIntersect(const std::vector<std::vector<glm::vec4>> &frustrumPlanesInWorld);
};

//using namespace std;
extern std::array<int, 72000> chunkMeshIdx;
// C++ 11 allows us to define the size of an enum. This lets us use only one byte
// of memory to store our different block types. By default, the size of a C++ enum
// is that of an int (so, usually four bytes). This *does* limit us to only 256 different
// block types, but in the scope of this project we'll never get anywhere near that many.
enum BlockType : unsigned char
{
    EMPTY, GRASS, DIRT, STONE, WATER, ICE, SNOW, LAVA, BEDROCK
};

// The six cardinal directions in 3D space
enum Direction : unsigned char
{
    XPOS, XNEG, YPOS, YNEG, ZPOS, ZNEG
};

// Using a struct to store all the neighbors information for a block
struct blockNeighbor {
    Direction dir;
    glm::ivec3 offset;
    glm::ivec3 normal;
    // The offset amount after a pos adding the offset
    std::vector <glm::ivec3> quadPosList;
};

// Lets us use any enum class as the key of a
// std::unordered_map
struct EnumHash {
    template <typename T>
    size_t operator()(T t) const {
        return static_cast<size_t>(t);
    }
};

// One Chunk is a 16 x 256 x 16 section of the world,
// containing all the Minecraft blocks in that area.
// We divide the world into Chunks in order to make
// recomputing its VBO data faster by not having to
// render all the world at once, while also not having
// to render the world block by block.

// TODO have Chunk inherit from Drawable
class Chunk : public Drawable {
private:
    // All of the blocks contained within this Chunk
    std::array<BlockType, 65536> m_blocks;
    int minX, minZ;
    // This Chunk's four neighbors to the north, south, east, and west
    // The third input to this map just lets us use a Direction as
    // a key for this map.
    // These allow us to properly determine
    std::unordered_map<Direction, Chunk*, EnumHash> m_neighbors;

    bool m_ready2VBO = false;
    bool m_ready2Draw = false;
    bool m_initialized = false;

public:
    BoundingBox bBox;

    Chunk(OpenGLContext* context);
    Chunk(OpenGLContext* context, int x, int z);
    BlockType getBlockAt(unsigned int x, unsigned int y, unsigned int z) const;
    BlockType getBlockAt(int x, int y, int z) const;
    void setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t);
    void linkNeighbor(uPtr<Chunk>& neighbor, Direction dir);
    void linkNeighbor(Chunk *neighbor, Direction dir);

    // NEW MEMBER
    static const std::array<blockNeighbor, 6> neighborsInfo;
    std::vector<glm::vec4> m_PosColNor;
    //std::vector<int> m_indices;
    int m_idxOpaqueSize;

    std::vector<glm::vec4> m_transPosColNor;
    //std::vector<int> m_transIndices;
    int m_idxTransSize;

    // NEW FUNC
    // create chunk block data --> create interleaved data opaque and transparent --> create vbo --> draw
    virtual void createVBOdata();

    void createInterleavedData();
    void fillVboByInterData(const std::vector<glm::vec4> &iPosColNor);

    void createInterleavedTransData();
    void fillVboByInterTransData(const std::vector<glm::vec4> &iPosColNor);

    void setReadyToVBO(bool ready);
    void setReadyToDraw(bool ready);

    void destroyVBOdata();

    bool readyToVBO();
    bool readyToDraw();
    inline bool isInitialized()
    {
        return m_initialized;
    }

    float getGrassLandHeight(glm::vec2 xz);
    float getMountainHeight(glm::vec2 xz);
    float getHeight(glm::vec2 xz);

    void fillColumn();

    inline bool neighborsInitialized()
    {
        return (this->m_initialized && this->m_neighbors[XPOS]->m_initialized && this->m_neighbors[XNEG]->m_initialized && this->m_neighbors[ZPOS]->m_initialized && this->m_neighbors[ZNEG]->m_initialized);
    }

    void updateNeighbors();

    glm::ivec2 getMinCoords();
};

glm::vec4 getColByBlockType(BlockType _type);

bool isTransparent(BlockType _type);

bool haveBorder(BlockType _type);
