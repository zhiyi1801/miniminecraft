#include "chunk.h"
#include "noise.h"
#include <ostream>
#include <thread>
#include <mutex>
#include <chrono>
#include <iostream>

// neighbors contain DIRECTION, OFFSET, NORMAL, 4 POS OFFSET according to (x,y,z) in the low left back corner
const std::array<blockNeighbor, 6> Chunk::neighborsInfo = {
    blockNeighbor{XPOS, glm::ivec3(1,0,0), glm::ivec3(1,0,0),
        std::vector{glm::ivec3(1,0,1),glm::ivec3(1,0,0), glm::ivec3(1,1,0), glm::ivec3(1,1,1)},},

    blockNeighbor{XNEG, glm::ivec3(-1,0,0), glm::ivec3(-1,0,0),
        std::vector{glm::ivec3(0,0,1),glm::ivec3(0,0,0), glm::ivec3(0,1,0), glm::ivec3(0,1,1)},},

    blockNeighbor{YPOS, glm::ivec3(0,1,0), glm::ivec3(0,1,0),
        std::vector{glm::ivec3(0,1,0),glm::ivec3(1,1,0), glm::ivec3(1,1,1), glm::ivec3(0,1,1)},},

    blockNeighbor{YNEG, glm::ivec3(0,-1,0), glm::ivec3(0,-1,0),
        std::vector{glm::ivec3(0,0,0),glm::ivec3(1,0,0), glm::ivec3(1,0,1), glm::ivec3(0,0,1)},},

    blockNeighbor{ZPOS, glm::ivec3(0,0,1), glm::ivec3(0,0,1),
        std::vector{glm::ivec3(0,0,1),glm::ivec3(1,0,1), glm::ivec3(1,1,1), glm::ivec3(0,1,1)},},

    blockNeighbor{ZNEG, glm::ivec3(0,0,-1), glm::ivec3(0,0,-1),
        std::vector{glm::ivec3(0,0,0),glm::ivec3(1,0,0), glm::ivec3(1,1,0), glm::ivec3(0,1,0)},}
};

const static std::unordered_map<Direction, Direction, EnumHash> oppositeDirection {
    {XPOS, XNEG},
    {XNEG, XPOS},
    {YPOS, YNEG},
    {YNEG, YPOS},
    {ZPOS, ZNEG},
    {ZNEG, ZPOS}
};

BoundingBox::BoundingBox():pMax(-FLT_MAX), pMin(FLT_MAX) {}

//roughly estimate
bool BoundingBox::ifIntersect(const std::vector<std::vector<glm::vec4>> &frustrumPlanesInWorld)
{
    std::vector<bool> intersectVector;

    for(int ifrustrum = 0; ifrustrum < frustrumPlanesInWorld.size(); ++ifrustrum)
    {
        const std::vector<glm::vec4> &planesInWorld = frustrumPlanesInWorld[ifrustrum];
        bool flag = true;
        for(int i = 0; i < planesInWorld.size(); ++i)
        {
            glm::vec4 curPlane = planesInWorld[i];
            // start p end q
            glm::vec4 p{0, 0, 0, 1}, q{0, 0, 0, 1};

            for(int j = 0; j < 3; ++j)
            {
                if(curPlane[j] >= 0)
                {
                    p[j] = this->pMin[j];
                    q[j] = this->pMax[j];
                }

                else
                {
                    p[j] = this->pMax[j];
                    q[j] = this->pMin[j];
                }
            }

            float pDis = glm::dot(p, curPlane);
            float qDis = glm::dot(q, curPlane);

            // if p and q are all in the negative direction, not intersected
            // TODO why have to add a bias -10? is it because of the floating number?
            if(pDis <= 0 && qDis <= 0)
            {
                flag = false;
                break;
            }
        }
        if(flag)
        {
            return true;
        }
    }

    return false;
}

Chunk::Chunk(OpenGLContext* context):Drawable(context),m_blocks(), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}}, bBox(), m_idxOpaqueSize(0), m_idxTransSize(0){}

Chunk::Chunk(OpenGLContext* context, int x, int z) : Drawable(context), m_blocks(), minX(x), minZ(z), m_neighbors{{XPOS, nullptr}, {XNEG, nullptr}, {ZPOS, nullptr}, {ZNEG, nullptr}}, bBox(), m_idxOpaqueSize(0), m_idxTransSize(0)
{
    std::fill_n(m_blocks.begin(), 65536, EMPTY);
    //this->m_PosColNor.reserve(512);
    //this->m_indices.reserve(512);
}

// Does bounds checking with at()
BlockType Chunk::getBlockAt(unsigned int x, unsigned int y, unsigned int z) const {
    if(x + 16 * y + 16 * 256 * z >= 65536)
    {
        return EMPTY;
    }
    return m_blocks.at((x + 16 * y + 16 * 256 * z) & 65535);
}

// Exists to get rid of compiler warnings about int -> unsigned int implicit conversion
BlockType Chunk::getBlockAt(int x, int y, int z) const {

    Chunk *cPtr = nullptr;
    if(x < 0 && (cPtr = this->m_neighbors.find(XNEG)->second) != nullptr)
    {
        return cPtr->getBlockAt(x & 15, y, z);
    }

    if(x > 15 && (cPtr = this->m_neighbors.find(XPOS)->second) != nullptr)
    {
        return cPtr->getBlockAt(x & 15, y, z);
    }

    if(z < 0 && (cPtr = this->m_neighbors.find(ZNEG)->second) != nullptr)
    {
        return cPtr->getBlockAt(x, y, z & 15);
    }

    if(z > 15 && (cPtr = this->m_neighbors.find(ZPOS)->second) != nullptr)
    {
        return cPtr->getBlockAt(x, y, z & 15);
    }

    if (x < 0 || x > 15 || z < 0 || z > 15 ||y < 0 || y > 255) {
        return EMPTY;
    }

    return getBlockAt(static_cast<unsigned int>(x), static_cast<unsigned int>(y), static_cast<unsigned int>(z));
}

// Does bounds checking with at()
void Chunk::setBlockAt(unsigned int x, unsigned int y, unsigned int z, BlockType t) {
    m_blocks.at(x + 16 * y + 16 * 256 * z) = t;
}

void Chunk::linkNeighbor(uPtr<Chunk> &neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor.get();
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

void Chunk::linkNeighbor(Chunk *neighbor, Direction dir) {
    if(neighbor != nullptr) {
        this->m_neighbors[dir] = neighbor;
        neighbor->m_neighbors[oppositeDirection.at(dir)] = this;
    }
}

void Chunk::createVBOdata()
{
    if(!this->readyToVBO())
    {
        createInterleavedData();
        createInterleavedTransData();
        m_ready2VBO = true;
    }
    fillVboByInterData(m_PosColNor);
    fillVboByInterTransData(m_transPosColNor);
    this->setReadyToDraw(true);
}

std::unordered_map<BlockType, std::unordered_map<Direction, glm::vec2, EnumHash>, EnumHash> blockFaceUvs
{
    {GRASS, std::unordered_map<Direction, glm::vec2, EnumHash> {
            {XPOS, glm::vec2(3.f/16.f, 15.f/16.f)},
            {XNEG, glm::vec2(3.f/16.f, 15.f/16.f)},
            {YPOS, glm::vec2(8.f/16.f, 13.f/16.f)}, // 8, 13
            {YNEG, glm::vec2(2.f/16.f, 15.f/16.f)}, // 2, 15
            {ZPOS, glm::vec2(3.f/16.f, 15.f/16.f)}, // 3, 15
            {ZNEG, glm::vec2(3.f/16.f, 15.f/16.f)}
                    }},
    {DIRT, std::unordered_map<Direction, glm::vec2, EnumHash> {
            {XPOS, glm::vec2(2.f/16.f, 15.f/16.f)},
            {XNEG, glm::vec2(2.f/16.f, 15.f/16.f)},
            {YPOS, glm::vec2(2.f/16.f, 15.f/16.f)}, // 2, 15
            {YNEG, glm::vec2(2.f/16.f, 15.f/16.f)},
            {ZPOS, glm::vec2(2.f/16.f, 15.f/16.f)},
            {ZNEG, glm::vec2(2.f/16.f, 15.f/16.f)}
                    }},
    {STONE, std::unordered_map<Direction, glm::vec2, EnumHash> {
            {XPOS, glm::vec2(1.f/16.f, 15.f/16.f)},
            {XNEG, glm::vec2(1.f/16.f, 15.f/16.f)},
            {YPOS, glm::vec2(1.f/16.f, 15.f/16.f)},
            {YNEG, glm::vec2(1.f/16.f, 15.f/16.f)},
            {ZPOS, glm::vec2(1.f/16.f, 15.f/16.f)},
            {ZNEG, glm::vec2(1.f/16.f, 15.f/16.f)}
                    }},
    {WATER, std::unordered_map<Direction, glm::vec2, EnumHash> {
            {XPOS, glm::vec2(13.f/16.f, 3.f/16.f)},
            {XNEG, glm::vec2(13.f/16.f, 3.f/16.f)},
            {YPOS, glm::vec2(13.f/16.f, 3.f/16.f)},
            {YNEG, glm::vec2(13.f/16.f, 3.f/16.f)},
            {ZPOS, glm::vec2(13.f/16.f, 3.f/16.f)},
            {ZNEG, glm::vec2(13.f/16.f, 3.f/16.f)}
                    }},
    {ICE, std::unordered_map<Direction, glm::vec2, EnumHash> {
            {XPOS, glm::vec2(3.f/16.f, 11.f/16.f)},
            {XNEG, glm::vec2(3.f/16.f, 11.f/16.f)},
            {YPOS, glm::vec2(3.f/16.f, 11.f/16.f)},
            {YNEG, glm::vec2(3.f/16.f, 11.f/16.f)},
            {ZPOS, glm::vec2(3.f/16.f, 11.f/16.f)},
            {ZNEG, glm::vec2(3.f/16.f, 11.f/16.f)}
                    }},
    {SNOW, std::unordered_map<Direction, glm::vec2, EnumHash> {
            {XPOS, glm::vec2(2.f/16.f, 11.f/16.f)},
            {XNEG, glm::vec2(2.f/16.f, 11.f/16.f)},
            {YPOS, glm::vec2(2.f/16.f, 11.f/16.f)},
            {YNEG, glm::vec2(2.f/16.f, 11.f/16.f)},
            {ZPOS, glm::vec2(2.f/16.f, 11.f/16.f)},
            {ZNEG, glm::vec2(2.f/16.f, 11.f/16.f)}
                    }},
    {LAVA, std::unordered_map<Direction, glm::vec2, EnumHash> {
            {XPOS, glm::vec2(13.f/16.f, 1.f/16.f)},
            {XNEG, glm::vec2(13.f/16.f, 1.f/16.f)},
            {YPOS, glm::vec2(13.f/16.f, 1.f/16.f)},
            {YNEG, glm::vec2(13.f/16.f, 1.f/16.f)},
            {ZPOS, glm::vec2(13.f/16.f, 1.f/16.f)},
            {ZNEG, glm::vec2(13.f/16.f, 1.f/16.f)}
                    }},
    {BEDROCK, std::unordered_map<Direction, glm::vec2, EnumHash> {
            {XPOS, glm::vec2(1.f/16.f, 14.f/16.f)},
            {XNEG, glm::vec2(1.f/16.f, 14.f/16.f)},
            {YPOS, glm::vec2(1.f/16.f, 14.f/16.f)},
            {YNEG, glm::vec2(1.f/16.f, 14.f/16.f)},
            {ZPOS, glm::vec2(1.f/16.f, 14.f/16.f)},
            {ZNEG, glm::vec2(1.f/16.f, 14.f/16.f)}
                    }}
};

std::unordered_map<int, glm::vec2, EnumHash> iquadToUvOffset {
    {0, glm::vec2(0, 0)},
    {1, glm::vec2(1, 0)},
    {2, glm::vec2(1, 1)},
    {3, glm::vec2(0, 1)}
};

void Chunk::createInterleavedData()
{
    m_PosColNor.clear();
    m_idxOpaqueSize = 0;

    // see comment in Terrain::blockTypeWorker()
    while(!this->neighborsInitialized())
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }

    for(int ix = 0; ix < 16; ++ix)
    {
        for(int iz = 0; iz < 16; ++iz)
        {
            for(int iy = 0; iy < 256; ++iy)
            {
                BlockType blockType = getBlockAt(ix, iy, iz);
                if(blockType == EMPTY || isTransparent(blockType)) continue;
                glm::vec4 curCol = getColByBlockType(blockType);

                for(auto &curNeighbor : Chunk::neighborsInfo)
                {
                    glm::ivec3 neighborPos = glm::ivec3(ix, iy, iz) + curNeighbor.offset;
                    BlockType neighborBlockType = getBlockAt(neighborPos.x, neighborPos.y, neighborPos.z);
                    if(neighborBlockType != EMPTY && !isTransparent(neighborBlockType)) continue;

                    if(iy > 0 && iy <= 100)
                    {
                        qDebug() << neighborBlockType << ": " << minX << ", " << minZ << ",  :" << neighborPos.x << ", " << neighborPos.y << ", " << neighborPos.z << "\n";
                    }
                    // we append 3 attributes for each vertex
                    for(int iquad = 0; iquad < 4; ++iquad)
                    {
                        // POS
                        m_PosColNor.push_back(glm::vec4(glm::ivec3(ix + minX, iy, iz + minZ) + curNeighbor.quadPosList[iquad], 1.f));

                        //
                        Direction currDir = curNeighbor.dir;
                        curCol[0] = blockFaceUvs[blockType][currDir].x + iquadToUvOffset[iquad].x * 1.f / 16.f;
                        curCol[1] = blockFaceUvs[blockType][currDir].y + iquadToUvOffset[iquad].y * 1.f / 16.f;
                        // set animateable flag from blocktype
                        if (blockType == LAVA) {
                            curCol[2] = 1.f;
                        } else {
                            curCol[2] = 0.f;
                        }

                        m_PosColNor.push_back(curCol);

                        // NORMAL
                        m_PosColNor.push_back(glm::vec4(curNeighbor.normal, .0f));
                    }

                    // a quad is composed of 2 triangles
                    m_idxOpaqueSize += 6;
                }
            }
        }
    }
    return;
}

void Chunk::fillVboByInterData(const std::vector<glm::vec4> &iPosColNor)
{
    m_count = m_idxOpaqueSize;

    if(m_idxGenerated)
    {
        mp_context->glDeleteBuffers(1, &m_bufIdx);
    }

    generateIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_idxOpaqueSize * sizeof(GLuint), chunkMeshIdx.data(), GL_STATIC_DRAW);

    if(m_allGenerated)
    {
        mp_context->glDeleteBuffers(1, &m_bufAll);
    }

    generateAll();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufAll);
    mp_context->glBufferData(GL_ARRAY_BUFFER, iPosColNor.size() * sizeof(glm::vec4), iPosColNor.data(), GL_STATIC_DRAW);
}

void Chunk::createInterleavedTransData()
{
    m_transPosColNor.clear();
    m_idxTransSize = 0;
    for(int ix = 0; ix < 16; ++ix)
    {
        for(int iy = 0; iy < 256; ++iy)
        {
            for(int iz = 0; iz < 16; ++iz)
            {
                BlockType blockType = getBlockAt(ix, iy, iz);
                if(blockType == EMPTY || !isTransparent(getBlockAt(ix, iy, iz))) continue;
                glm::vec4 curCol = getColByBlockType(blockType);

                for(auto &curNeighbor : Chunk::neighborsInfo)
                {
                    glm::ivec3 neighborPos = glm::ivec3(ix, iy, iz) + curNeighbor.offset;

                    // see comment in Terrain::blockTypeWorker(), this function is always after createInterleavedData()
//                    if(neighborPos.x < 0 || neighborPos.x > 15 || neighborPos.z < 0 || neighborPos.z > 15)
//                    {
//                        while(!m_neighbors[curNeighbor.dir]->isInitialized())
//                        {
//                            std::this_thread::sleep_for(std::chrono::milliseconds(5));
//                        }
//                    }
                    BlockType neighborBlockType = getBlockAt(neighborPos.x, neighborPos.y, neighborPos.z);
                    if(neighborBlockType != EMPTY) continue;

                    // border of the chunk
                    if(neighborPos.x < 0 || neighborPos.x > 15 || neighborPos.z < 0 || neighborPos.z > 15)
                    {
                        // block like water should not have border
                        if(!haveBorder(blockType))
                        {
                            continue;
                        }
                    }

                    // we append 3 attributes for each vertex
                    for(int iquad = 0; iquad < 4; ++iquad)
                    {
                        // POS
                        m_transPosColNor.push_back(glm::vec4(glm::ivec3(ix + minX, iy, iz + minZ) + curNeighbor.quadPosList[iquad], 1.f));

                        Direction currDir = curNeighbor.dir;
                        curCol[0] = blockFaceUvs[blockType][currDir].x + iquadToUvOffset[iquad].x * 1.f / 16.f;
                        curCol[1] = blockFaceUvs[blockType][currDir].y + iquadToUvOffset[iquad].y * 1.f / 16.f;
                        curCol[2] = 1.f; // animateable flag set on


                        // COL
                        m_transPosColNor.push_back(curCol);

                        // NORMAL
                        m_transPosColNor.push_back(glm::vec4(curNeighbor.normal, .0f));
                    }

                    // a quad is composed of 2 triangles
                    m_idxTransSize += 6;
                }
            }
        }
    }

    return;
}

void Chunk::fillVboByInterTransData(const std::vector<glm::vec4> &iPosColNor)
{
    m_transCount = m_idxTransSize;

    if(m_transIdxGenerated)
    {
        mp_context->glDeleteBuffers(1, &m_bufTransIdx);
    }
    generateTransIdx();
    mp_context->glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_bufTransIdx);
    mp_context->glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_idxTransSize * sizeof(GLuint), chunkMeshIdx.data(), GL_STATIC_DRAW);

    if(m_transGenerated)
    {
        mp_context->glDeleteBuffers(1, &m_bufTrans);
    }
    generateTrans();
    mp_context->glBindBuffer(GL_ARRAY_BUFFER, m_bufTrans);
    mp_context->glBufferData(GL_ARRAY_BUFFER, iPosColNor.size() * sizeof(glm::vec4), iPosColNor.data(), GL_STATIC_DRAW);
}

void Chunk::setReadyToVBO(bool ready)
{
    this->m_ready2VBO = ready;
    return;
}

void Chunk::setReadyToDraw(bool ready)
{
    this->m_ready2Draw = ready;
    return;
}

bool Chunk::readyToVBO()
{
    return m_ready2VBO;
}

bool Chunk::readyToDraw()
{
    return m_ready2Draw;
}

void Chunk::destroyVBOdata()
{
    Drawable::destroyVBOdata();
}

float Chunk::getGrassLandHeight(glm::vec2 xz)
{
    float h = 0;

    float amp = 0.5;
    float freq = 128;
    for(int i = 0; i < 4; ++i) {
        glm::vec2 offset = glm::vec2(Noise::fbmNoise2D(xz / 256.f), Noise::fbmNoise2D(xz / 300.f) + 1000.f);
        float h1 = Noise::perlinNoise2D((xz + offset * 75.f) / freq);
        //        float h1 = WorleyNoise(xz / freq);
        //        h1 = 1. - abs(h1);
        //        h1 = pow(h1, 1.5);
        h += h1 * amp;

        amp *= 0.5;
        freq *= 0.5;
    }

    //h = glm::smoothstep(0.1f, 0.25f, h) * 0.9 + (0.1 * h);
    h = floor(GRASS_MIN + glm::abs(h) * (GRASS_MAX - GRASS_MIN));

    return h;
}

float Chunk::getMountainHeight(glm::vec2 xz)
{
    float h = 0;

    float amp = 0.5;
    float freq = 128;
    for(int i = 0; i < 4; ++i) {
        float temp  =Noise::fbmNoise2D(xz / 300.f);
        glm::vec2 offset = glm::vec2(Noise::fbmNoise2D(xz / 256.f), Noise::fbmNoise2D(xz / 300.f) + 1000.f);
        float h1 = Noise::perlinNoise2D((xz + offset * 75.f) / freq);
        h += h1 * amp;

        amp *= 0.5;
        freq *= 0.5;
    }

    h = glm::smoothstep(0.f, 0.3f,glm::abs(h)) * 0.9 + (0.1 * glm::abs(h));
    h = glm::floor(MOUNTAIN_MIN + h * (MOUNTAIN_MAX - MOUNTAIN_MIN));

    return h;
}

float Chunk::getHeight(glm::vec2 xz)
{
    float perlin = 0.5 * (Noise::perlinNoise2D(xz/1024.f) + 1.f);
    perlin = glm::smoothstep(0.4f, 0.6f, perlin);

    float grassHeight = getGrassLandHeight(xz);
    float mountainHeight = getMountainHeight(xz);
    float ret = glm::mix(mountainHeight, grassHeight, perlin);
    return ret;
}

// fill the chunk according to procedural height
void Chunk::fillColumn()
{
    bBox.pMin = glm::vec3(this->minX, FLT_MAX ,this->minZ);
    bBox.pMax = glm::vec3(this->minX + 16, -FLT_MAX, this->minZ + 16);
    for(int ix = 0; ix < 16; ++ix)
    {
        for(int iz = 0; iz < 16; ++iz)
        {
            int height = (int)getHeight(glm::vec2(minX + ix, minZ + iz));

            for (int iy = 0; iy <= height; iy++) {
                BlockType block = EMPTY;

                if(iy <= STONE_LEVEL)
                {
                    block = STONE;
                }

                else if(iy > 180)
                {
                    if(iy == height)
                    {
                        block = SNOW;
                    }
                    else
                    {
                        block = STONE;
                    }
                }

                else if(iy > STONE_LEVEL)
                {
                    if(iy == height)
                    {
                        block = GRASS;
                    }
                    else
                    {
                        block = DIRT;
                    }
                }
                setBlockAt(ix, iy, iz, block);
            }


            if(height < SEA_LEVEL)
            {
                for(int waterh = height + 1; waterh <= SEA_LEVEL; ++waterh)
                {
                    setBlockAt(ix, waterh, iz, WATER);
                }
            }
            height = glm::max(height, SEA_LEVEL);
            bBox.pMin.y = 0;
            bBox.pMax.y = glm::max(bBox.pMax.y, float(height + 1));
        }
    }
    m_initialized = true;
    return;
}

void Chunk::updateNeighbors()
{
    this->setReadyToVBO(false);
    this->setReadyToDraw(false);
    this->createVBOdata();

    for(auto nInfo : this->neighborsInfo)
    {
        if(this->m_neighbors.find(nInfo.dir) == this->m_neighbors.end()) continue;

        Chunk *tempNeighbor = this->m_neighbors.find(nInfo.dir)->second;
        if(tempNeighbor != nullptr)
        {
            tempNeighbor->setReadyToVBO(false);
            tempNeighbor->setReadyToDraw(false);
            tempNeighbor->createVBOdata();
        }
    }
}

glm::vec4 getColByBlockType(BlockType _type)
{
    glm::vec4 resColor(0,0,0,1);
    if (_type != EMPTY) {
        switch (_type) {
        case GRASS:
            resColor = glm::vec4(95.f, 159.f, 53.f, 255.f) / 255.f;
            break;
        case DIRT:
            resColor = glm::vec4(121.f, 85.f, 58.f, 255.f) / 255.f;
            break;
        case STONE:
            resColor = glm::vec4(0.5f, 0.5, 0.5, .0f);
            break;
        case SNOW:
            resColor = glm::vec4(1.f, 1.f, 1.f ,1.f);
            break;
        case WATER:
            resColor = glm::vec4(51.f, 153.f, 255.f, 200.f) / 255.f;
        default:
            break;
        }
    }
    return resColor;
}

bool isTransparent(BlockType _type)
{
    if(_type == WATER)
    {
        return true;
    }

    return false;
}

bool haveBorder(BlockType _type)
{
    if(_type == WATER)
    {
        return false;
    }

    return true;
}

glm::ivec2 Chunk::getMinCoords()
{
    return glm::ivec2(minX, minZ);
}
