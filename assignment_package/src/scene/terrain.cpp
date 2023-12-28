#include "terrain.h"
#include "cube.h"
#include <stdexcept>
#include <iostream>
#include <QElapsedTimer>
#include <chrono>

Terrain::Terrain(OpenGLContext *context)
    : m_chunks(), m_generatedTerrain(), /*m_geomCube(context),*/ mp_context(context)
{}

Terrain::~Terrain() {
    //m_geomCube.destroyVBOdata();
    for(auto &chunkPair : m_chunks)
    {
        chunkPair.second->destroyVBOdata();
    }
}

// Combine two 32-bit ints into one 64-bit int
// where the upper 32 bits are X and the lower 32 bits are Z
int64_t toKey(int x, int z) {
    int64_t xz = 0xffffffffffffffff;
    int64_t x64 = x;
    int64_t z64 = z;

    // Set all lower 32 bits to 1 so we can & with Z later
    xz = (xz & (x64 << 32)) | 0x00000000ffffffff;

    // Set all upper 32 bits to 1 so we can & with XZ
    z64 = z64 | 0xffffffff00000000;

    // Combine
    xz = xz & z64;
    return xz;
}

glm::ivec2 toCoords(int64_t k) {
    // Z is lower 32 bits
    int64_t z = k & 0x00000000ffffffff;
    // If the most significant bit of Z is 1, then it's a negative number
    // so we have to set all the upper 32 bits to 1.
    // Note the 8    V
    if(z & 0x0000000080000000) {
        z = z | 0xffffffff00000000;
    }
    int64_t x = (k >> 32);

    return glm::ivec2(x, z);
}

// Surround calls to this with try-catch if you don't know whether
// the coordinates at x, y, z have a corresponding Chunk
BlockType Terrain::getBlockAt(int x, int y, int z) const
{
    if(hasChunkAt(x, z)) {
        // Just disallow action below or above min/max height,
        // but don't crash the game over it.
        if(y < 0 || y >= 256) {
            return EMPTY;
        }
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        return c->getBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                             static_cast<unsigned int>(y),
                             static_cast<unsigned int>(z - chunkOrigin.y));
    }
    else {
        //zthrow std::out_of_range("Coordinates " + std::to_string(x) +
        //                        " " + std::to_string(y) + " " +
        //                        std::to_string(z) + " have no Chunk!");
        return EMPTY;
    }
}

BlockType Terrain::getBlockAt(glm::vec3 p) const {
    return getBlockAt(p.x, p.y, p.z);
}

bool Terrain::hasChunkAt(int x, int z) const {
    // Map x and z to their nearest Chunk corner
    // By flooring x and z, then multiplying by 16,
    // we clamp (x, z) to its nearest Chunk-space corner,
    // then scale back to a world-space location.
    // Note that floor() lets us handle negative numbers
    // correctly, as floor(-1 / 16.f) gives us -1, as
    // opposed to (int)(-1 / 16.f) giving us 0 (incorrect!).
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.find(toKey(16 * xFloor, 16 * zFloor)) != m_chunks.end();
}


uPtr<Chunk>& Terrain::getChunkAt(int x, int z) {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks[toKey(16 * xFloor, 16 * zFloor)];
}


const uPtr<Chunk>& Terrain::getChunkAt(int x, int z) const {
    int xFloor = static_cast<int>(glm::floor(x / 16.f));
    int zFloor = static_cast<int>(glm::floor(z / 16.f));
    return m_chunks.at(toKey(16 * xFloor, 16 * zFloor));
}

void Terrain::setBlockAt(int x, int y, int z, BlockType t) const
{
    if(hasChunkAt(x, z)) {
        const uPtr<Chunk> &c = getChunkAt(x, z);
        glm::vec2 chunkOrigin = glm::vec2(floor(x / 16.f) * 16, floor(z / 16.f) * 16);
        c->setBlockAt(static_cast<unsigned int>(x - chunkOrigin.x),
                      static_cast<unsigned int>(y),
                      static_cast<unsigned int>(z - chunkOrigin.y),
                      t);
    }
    else {
        throw std::out_of_range("Coordinates " + std::to_string(x) +
                                " " + std::to_string(y) + " " +
                                std::to_string(z) + " have no Chunk!");
    }
}

Chunk* Terrain::instantiateChunkAt(int x, int z) {
    uPtr<Chunk> chunk = mkU<Chunk>(mp_context, x, z);
    Chunk *cPtr = chunk.get();
    m_chunks[toKey(x, z)] = move(chunk);
    // Set the neighbor pointers of itself and its neighbors
    if(hasChunkAt(x, z + 16)) {
        auto &chunkNorth = m_chunks[toKey(x, z + 16)];
        cPtr->linkNeighbor(chunkNorth, ZPOS);
    }
    if(hasChunkAt(x, z - 16)) {
        auto &chunkSouth = m_chunks[toKey(x, z - 16)];
        cPtr->linkNeighbor(chunkSouth, ZNEG);
    }
    if(hasChunkAt(x + 16, z)) {
        auto &chunkEast = m_chunks[toKey(x + 16, z)];
        cPtr->linkNeighbor(chunkEast, XPOS);
    }
    if(hasChunkAt(x - 16, z)) {
        auto &chunkWest = m_chunks[toKey(x - 16, z)];
        cPtr->linkNeighbor(chunkWest, XNEG);
    }

    for(int i = 0; i < 16; ++i)
    {
        for(int j = 0; j < 16; ++j)
        {
            fillColumn(x + i, z + j);
        }
    }
    cPtr->createVBOdata();
    return cPtr;
}

// TODO: When you make Chunk inherit from Drawable, change this code so
// it draws each Chunk with the given ShaderProgram, remembering to set the
// model matrix to the proper X and Z translation!
void Terrain::draw(int minX, int maxX, int minZ, int maxZ,
                   ShaderProgram *shaderProgram,
                   const std::vector<std::vector<glm::vec4>> &frustrumPlanesInworld)
{
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16)
        {
            m_chunkMutex.lock();
            //if the chunk does not exist, nop
            if(hasChunkAt(x,z))
            {
                //if the chunk is not ready to draw, nop
                const uPtr<Chunk> &chunk = getChunkAt(x, z);
                glm::ivec2 testXZ = chunk->getMinCoords();
                if(!chunk->readyToDraw())
                {
                    m_chunkMutex.unlock();
                    continue;
                }

                if(frustrumPlanesInworld.size() != 0)
                {
                    if(!chunk->bBox.ifIntersect(frustrumPlanesInworld))
                    {
                        m_chunkMutex.unlock();
                        continue;
                    }
                }

                shaderProgram->drawInterleaved(*(chunk.get()));
            }
            m_chunkMutex.unlock();
        }
    }
}

void Terrain::drawTrans(int minX, int maxX, int minZ, int maxZ,
                        ShaderProgram *shaderProgram,
                        const std::vector<std::vector<glm::vec4>> &frustrumPlanesInworld)
{
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(int x = minX; x < maxX; x += 16) {
        for(int z = minZ; z < maxZ; z += 16)
        {
            m_chunkMutex.lock();
            if(hasChunkAt(x,z))
            {
                const uPtr<Chunk> &chunk = getChunkAt(x, z);

                if(frustrumPlanesInworld.size() != 0)
                {
                    if(!chunk->bBox.ifIntersect(frustrumPlanesInworld))
                    {
                        m_chunkMutex.unlock();
                        continue;
                    }
                }

                if(chunk->readyToDraw())
                {
                    shaderProgram->drawTransInterleaved(*(chunk.get()));
                }
            }
            m_chunkMutex.unlock();
        }
    }
    glDisable(GL_BLEND);
}

void Terrain::CreateTestScene()
{
    // TODO: DELETE THIS LINE WHEN YOU DELETE m_geomCube!
    //m_geomCube.createVBOdata();

    // Create the Chunks that will
    // store the blocks for our
    // initial world space

    for(int x = 0; x < 256; x += 64) {
        for(int z = 0; z < 256; z += 64) {
            createTerrainGenerationZone(glm::ivec3(x, 0, z));
        }
    }
}

//TODO in this function will use m_chunks which is shared in multi thread
void Terrain::fillColumn(int x, int z)
{
    int height = (int)getHeight(glm::vec2(x, z));

    for (int k = 0; k <= height; k++) {
        BlockType block = EMPTY;

        if(k <= STONE_LEVEL)
        {
            block = STONE;
        }

        else if(k > 180)
        {
            if(k == height)
            {
                block = SNOW;
            }
            else
            {
                block = STONE;
            }
        }

        else if(k > STONE_LEVEL)
        {
            if(k == height)
            {
                block = GRASS;
            }
            else
            {
                block = DIRT;
            }
        }
        setBlockAt(x, k, z, block);
    }


    if(height < SEA_LEVEL)
    {
        for(int waterh = height + 1; waterh <= SEA_LEVEL; ++waterh)
        {
            setBlockAt(x, waterh, z, WATER);
        }
    }
}

float Terrain::getGrassLandHeight(glm::vec2 xz)
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

float Terrain::getMountainHeight(glm::vec2 xz)
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

float getTestHeight(glm::vec2 xz)
{
    float h = 0;

    float amp = 0.5;
    float freq = 128;
    for(int i = 0; i < 4; ++i) {
        glm::vec2 offset = glm::vec2(Noise::fbmNoise2D(xz / 256.f), Noise::fbmNoise2D(xz / 300.f) + 1000.f);
        float h1 = Noise::perlinNoise2D((xz + offset * 75.f) / freq);
        h += h1 * amp;

        amp *= 0.5;
        freq *= 0.5;
    }

    return h;
}

float Terrain::getHeight(glm::vec2 xz)
{
    float perlin = 0.5 * (Noise::perlinNoise2D(xz/1024.f) + 1.f);
    perlin = glm::smoothstep(0.4f, 0.6f, perlin);

    float grassHeight = getGrassLandHeight(xz);
    float mountainHeight = getMountainHeight(xz);
    float ret = glm::mix(mountainHeight, grassHeight, perlin);
    return ret;
}

void Terrain::createTerrainGenerationZone(glm::ivec3 pos)
{
    int startx = glm::floor(pos.x / 64.0f) * 64, startz = glm::floor(pos.z / 64.0f) * 64;

    // if this terrain generation zone already exists, put the data to gpu and return.
    if(m_generatedTerrain.find(toKey(startx, startz)) != m_generatedTerrain.end())
    {
        for(int x = startx; x < startx + 64; x += 16) {
            for(int z = startz; z < startz + 64; z += 16) {
                uPtr<Chunk> &curChunk = getChunkAt(x, z);
                curChunk->createVBOdata();
            }
        }
        return;
    }

    // if the terrain is not initiated
    for(int x = startx; x < startx + 64; x += 16) {
        for(int z = startz; z < startz + 64; z += 16) {
            instantiateChunkAt(x, z);
        }
    }
    m_generatedTerrain.insert(toKey(startx, startz));
    return;
}

void Terrain::createSceneByPosition(glm::ivec3 pos)
{
    int startx = glm::floor(pos.x / 64.0f) * 64, startz = glm::floor(pos.z / 64.0f) * 64;

    for(int ix = -1 * TERRAIN_CREATE_RADIUS; ix <= TERRAIN_CREATE_RADIUS; ++ix)
    {
        for(int iz = -1 * TERRAIN_CREATE_RADIUS; iz <= TERRAIN_CREATE_RADIUS; ++iz)
        {
            createTerrainGenerationZone(glm::ivec3(startx + ix * 64, 0, startz + iz * 64));
            //m_generatedTerrain.insert(toKey(startx + ix * 64, startz + iz * 64));
        }
    }
    return;
}

void Terrain::unloadTerrainGPU(glm::ivec3 pos)
{
    int startx = glm::floor(pos.x / 64.0f) * 64, startz = glm::floor(pos.z / 64.0f) * 64;

    // if the terrain generation zone doesn't exist, nop
    if(m_generatedTerrain.find(toKey(startx, startz)) == m_generatedTerrain.end()) return;

    for(int ix = startx; ix < startx + 64; ix += 16)
    {
        for(int iz = startz; iz < startz + 64; iz += 16)
        {
            if(m_chunks.find(toKey(ix, iz)) != m_chunks.end())
            {
                uPtr<Chunk> &curChunk = m_chunks.find(toKey(ix, iz))->second;
                curChunk->destroyVBOdata();
            }
        }
    }
}

void Terrain::updateVBObyPosition(glm::ivec3 curPos, glm::ivec3 prePos)
{
    int curX = glm::floor(curPos.x / 64.0f) * 64,  curZ = glm::floor(curPos.z / 64.0f) * 64,
        preX = glm::floor(prePos.x / 64.0f) * 64,  preZ = glm::floor(prePos.z / 64.0f) * 64;

    // max use (TERRAIN_CREATE_RADIUS + 1) because the interval is open interval and x, z is on the left low corner
    int curXmin = curX - 64 * TERRAIN_CREATE_RADIUS, curXmax = curX + 64 * (TERRAIN_CREATE_RADIUS + 1),
        curZmin = curZ - 64 * TERRAIN_CREATE_RADIUS, curZmax = curZ + 64 * (TERRAIN_CREATE_RADIUS + 1);

    int preXmin = preX - 64 * TERRAIN_CREATE_RADIUS, preXmax = preX + 64 * (TERRAIN_CREATE_RADIUS + 1),
        preZmin = preZ - 64 * TERRAIN_CREATE_RADIUS, preZmax = preZ + 64 * (TERRAIN_CREATE_RADIUS + 1);

    for(int ix = preXmin; ix < preXmax; ix += 64)
    {
        for(int iz = preZmin; iz < preZmax; iz += 64)
        {
            if(ix < curXmax && ix >= curXmin && iz < curZmax && iz >= curZmin) continue;
            unloadTerrainGPU(glm::ivec3(ix, 0, iz));
        }
    }

    // second generate new terrain
    for(int ix = curXmin; ix < curXmax; ix += 64)
    {
        for(int iz = curZmin; iz < curZmax; iz += 64)
        {
            if(ix < preXmax && ix >= preXmin && iz < preZmax && iz >= preZmin) continue;
            createTerrainGenerationZone(glm::ivec3(ix, 0, iz));
        }
    }

    return;
}

// generate all zones in GENERATE_RADIUS
void Terrain::multiThreadUpdateTerrainGenerateRadius(glm::ivec3 curPos)
{
    //QElapsedTimer timedebuge;//declare a timer
    //timedebuge.start();

    int curX = glm::floor(curPos.x / 64.0f) * 64,  curZ = glm::floor(curPos.z / 64.0f) * 64;

    // max use (TERRAIN_CREATE_RADIUS + 1) because the interval is open interval and x, z is on the left low corner
    int curXmin = curX - 64 * TERRAIN_CREATE_RADIUS, curXmax = curX + 64 * (TERRAIN_CREATE_RADIUS + 1),
        curZmin = curZ - 64 * TERRAIN_CREATE_RADIUS, curZmax = curZ + 64 * (TERRAIN_CREATE_RADIUS + 1);

    // initiate new terrain
    for(int ix = curXmin; ix < curXmax; ix += 64)
    {
        for(int iz = curZmin; iz < curZmax; iz += 64)
        {
            if(m_generatedTerrain.find(toKey(ix, iz)) != m_generatedTerrain.end()) continue;
            m_threadFinishedFlags.push_back(mkS<std::atomic<bool>>(false));
            m_threads.push_back(std::thread(&Terrain::blockTypeWorker, this, ix, iz, m_threadFinishedFlags.back(), m_threadFinishedFlags.size()));
            m_threads.back().detach();
            m_generatedTerrain.insert(toKey(ix, iz));
        }
    }
    //qDebug()<<"first part: "<<timedebuge.elapsed()<<"ms";

    m_chunkNotReadyMutex.lock();
    while(!m_chunksNotReady2VBO.empty())
    {
        auto tempKeyIte = m_chunksNotReady2VBO.begin();
        int64_t tempKey = *tempKeyIte;
        m_chunksNotReady2VBO.erase(tempKeyIte);
        m_tempVBO.insert(tempKey);

        m_threadFinishedFlags.push_back(mkS<std::atomic<bool>>(false));
        m_threads.push_back(std::thread(&Terrain::VBOWorker, this, tempKey, m_threadFinishedFlags.back()));
        m_threads.back().detach();
    }
    m_chunkNotReadyMutex.unlock();

    //this->joinAll();

    int countPerCall = 6;
    m_chunkReadyMutex.lock();
    // every time call this function, only deal with "countPerCall" chunks
    while(!m_chunksReady2VBO.empty() && countPerCall--)
    {
        auto tempKeyIte = m_chunksReady2VBO.begin();
        int64_t tempKey = *tempKeyIte;
        m_chunksReady2VBO.erase(tempKeyIte);

        this->createChunkVBO(tempKey);
    }
    m_chunkReadyMutex.unlock();

    this->clearTerminateDetachThread();

    return;
}

// generate zones in DRAW_RADIUS, generate zoneNum zones each time
void Terrain::multiThreadUpdateTerrainDrawRadius(glm::ivec3 curPos, int zoneNum)
{
    this->clearTerminateDetachThread();
    // first generate the zones in GENERATE_RADIUS
    if(m_threads.size() != 0) return;

    int curX = glm::floor(curPos.x / 64.0f) * 64,  curZ = glm::floor(curPos.z / 64.0f) * 64;

    // max use (TERRAIN_CREATE_RADIUS + 1) because the interval is open interval and x, z is on the left low corner
    int curXmin = curX - 64 * TERRAIN_DRAW_RADIUS, curXmax = curX + 64 * (TERRAIN_DRAW_RADIUS + 1),
        curZmin = curZ - 64 * TERRAIN_DRAW_RADIUS, curZmax = curZ + 64 * (TERRAIN_DRAW_RADIUS + 1);

    // initiate new terrain
    int count = 0;
    for(int ir = 0; ir <= TERRAIN_DRAW_RADIUS + 1; ++ir)
    {
        if(count >= zoneNum) break;
        curXmin = curX - 64 * ir, curXmax = curX + 64 * (ir + 1),
        curZmin = curZ - 64 * ir, curZmax = curZ + 64 * (ir + 1);
        for(int ix = curXmin; ix < curXmax; ix += 64)
        {
            if(count >= zoneNum) break;
            for(int iz = curZmin; iz < curZmax; iz += 64)
            {
                if(count >= zoneNum) break;
                if(m_generatedTerrain.find(toKey(ix, iz)) != m_generatedTerrain.end()) continue;
                m_threadFinishedFlags.push_back(mkS<std::atomic<bool>>(false));
                m_threads.push_back(std::thread(&Terrain::blockTypeWorker, this, ix, iz, m_threadFinishedFlags.back(), m_threadFinishedFlags.size()));
                m_threads.back().detach();
                m_generatedTerrain.insert(toKey(ix, iz));
                ++count;
            }
        }
    }

    m_chunkNotReadyMutex.lock();
    while(!m_chunksNotReady2VBO.empty())
    {
        auto tempKeyIte = m_chunksNotReady2VBO.begin();
        int64_t tempKey = *tempKeyIte;
        m_chunksNotReady2VBO.erase(tempKeyIte);
        m_tempVBO.insert(tempKey);

        m_threadFinishedFlags.push_back(mkS<std::atomic<bool>>(false));
        m_threads.push_back(std::thread(&Terrain::VBOWorker, this, tempKey, m_threadFinishedFlags.back()));
        m_threads.back().detach();
    }
    m_chunkNotReadyMutex.unlock();

    //this->joinAll();

    int countPerCall = 6;
    m_chunkReadyMutex.lock();
    // every time call this function, only deal with "countPerCall" chunks
    while(!m_chunksReady2VBO.empty() && countPerCall--)
    {
        auto tempKeyIte = m_chunksReady2VBO.begin();
        int64_t tempKey = *tempKeyIte;
        m_chunksReady2VBO.erase(tempKeyIte);

        this->createChunkVBO(tempKey);
    }
    m_chunkReadyMutex.unlock();

    return;
}

//initiate terrain at (x, 0, z)
void Terrain::blockTypeWorker(int x, int z, sPtr<std::atomic<bool>> finishedFlag, int test)
{
    int terrainX = glm::floor(x/64.f) * 64, terrainZ = glm::floor(z/64.f) * 64;
    for(int ix = terrainX; ix < terrainX + 64; ix += 16)
    {
        for(int iz = terrainZ; iz < terrainZ + 64; iz += 16)
        {
            Chunk *cPtr = nullptr;
            m_chunkMutex.lock();
            if(!hasChunkAt(ix, iz))
            {
                uPtr<Chunk> chunk = mkU<Chunk>(mp_context, ix, iz);
                cPtr = chunk.get();
                m_chunks[toKey(ix, iz)] = std::move(chunk);
                m_chunkMutex.unlock();

                cPtr->fillColumn();

                m_chunkMutex.lock();
            }
            cPtr = m_chunks[toKey(ix, iz)].get();
            m_chunkMutex.unlock();

            // Set the neighbor pointers of itself and its neighbors
            // ZPOS
            m_chunkMutex.lock();
            if(!hasChunkAt(ix, iz + 16))
            {
                uPtr<Chunk> tempChunk = mkU<Chunk>(mp_context, ix, iz + 16);
                Chunk *tempCPtr = tempChunk.get();
                m_chunks[toKey(ix, iz + 16)] = std::move(tempChunk);
                m_chunkMutex.unlock();

                tempCPtr->fillColumn();
                m_chunkMutex.lock();
            }
            auto &chunkNorth = m_chunks[toKey(ix, iz + 16)];
            cPtr->linkNeighbor(chunkNorth, ZPOS);
            m_chunkMutex.unlock();

            // ZNEG
            m_chunkMutex.lock();
            if(!hasChunkAt(ix, iz - 16)) {
                uPtr<Chunk> tempChunk = mkU<Chunk>(mp_context, ix, iz - 16);
                Chunk *tempCPtr = tempChunk.get();
                m_chunks[toKey(ix, iz - 16)] = std::move(tempChunk);
                m_chunkMutex.unlock();

                tempCPtr->fillColumn();
                m_chunkMutex.lock();
            }
            auto &chunkSouth = m_chunks[toKey(ix, iz - 16)];
            cPtr->linkNeighbor(chunkSouth, ZNEG);
            m_chunkMutex.unlock();

            // XPOS
            m_chunkMutex.lock();
            if(!hasChunkAt(ix + 16, iz)) {
                uPtr<Chunk> tempChunk = mkU<Chunk>(mp_context, ix + 16, iz);
                Chunk *tempCPtr = tempChunk.get();
                m_chunks[toKey(ix + 16, iz)] = std::move(tempChunk);
                m_chunkMutex.unlock();

                tempCPtr->fillColumn();
                m_chunkMutex.lock();

            }
            auto &chunkEast = m_chunks[toKey(ix + 16, iz)];
            cPtr->linkNeighbor(chunkEast, XPOS);
            m_chunkMutex.unlock();

            //XNEG
            m_chunkMutex.lock();
            if(!hasChunkAt(ix - 16, iz)) {
                uPtr<Chunk> tempChunk = mkU<Chunk>(mp_context, ix - 16, iz);
                Chunk *tempCPtr = tempChunk.get();
                m_chunks[toKey(ix - 16, iz)] = std::move(tempChunk);
                m_chunkMutex.unlock();

                tempCPtr->fillColumn();
                m_chunkMutex.lock();
            }
            auto &chunkWest = m_chunks[toKey(ix - 16, iz)];
            cPtr->linkNeighbor(chunkWest, XNEG);
            m_chunkMutex.unlock();

            // if one thread is in fillColumn(release m_chunkMutex) and another goes here, and further more goes to VBOWorker, it will cause data race
            // therefore set a initialize flag for each chunk, when it and its neighbors are initialized, going on
//            while(!cPtr->neighborsInitialized())
//            {
//                qDebug() << "blockTypeWorker sleep";
//                std::this_thread::sleep_for(std::chrono::milliseconds(10));
//            }

            m_chunkNotReadyMutex.lock();
            if(!cPtr->readyToVBO())
            {
                m_chunksNotReady2VBO.insert(toKey(ix, iz));
            }
            m_chunkNotReadyMutex.unlock();
        }
    }

    *finishedFlag = true;
    return;
}

// create interleaved data for chunk at (x,y,z)
void Terrain::VBOWorker(int64_t key, sPtr<std::atomic<bool>> finishedFlag)
{
    m_chunkMutex.lock();
    uPtr<Chunk> &curChunk = this->m_chunks[key];
    m_chunkNotReadyMutex.lock();
    if(curChunk->readyToVBO())
    {
        m_chunkNotReadyMutex.unlock();
        m_chunkMutex.unlock();
        *finishedFlag = true;
        return;
    }
    m_chunkNotReadyMutex.unlock();
    m_chunkMutex.unlock();

    curChunk->createInterleavedData();
    curChunk->createInterleavedTransData();

    // notice that only the main thread can communicate with gpu
    //curChunk->createVBOdata();
    m_chunkReadyMutex.lock();
    curChunk->setReadyToVBO(true);
    m_chunksReady2VBO.insert(key);
    m_chunkReadyMutex.unlock();

    *finishedFlag = true;
    return;
}

void Terrain::createChunkVBO(int64_t key)
{
    m_chunkMutex.lock();
    uPtr<Chunk> &curChunk = this->m_chunks[key];
    m_chunkMutex.unlock();

    assert(curChunk->readyToVBO());
    curChunk->createVBOdata();
}

void Terrain::joinAll()
{
    while(!m_threads.empty())
    {
        if(m_threads.front().joinable())
        {
            m_threads.front().join();
        }
        m_threads.erase(m_threads.begin());
    }
}

void Terrain::joinOne()
{
    if(!m_threads.empty())
    {
        if(m_threads.front().joinable())
        {
            m_threads.front().join();
        }
        m_threads.erase(m_threads.begin());

        for(int i = 0; i < m_threads.size(); ++i)
        {
            if(!m_threads[i].joinable())
            {
                m_threads.erase(m_threads.begin() + i);
            }
        }
    }
}

void Terrain::clearTerminateJoinThread()
{
    for(int i = 0; i < m_threads.size(); ++i)
    {
        if(!m_threads[i].joinable())
        {
            m_threads.erase(m_threads.begin() + i);
            --i;
        }
    }
}

void Terrain::clearTerminateDetachThread()
{
    for(size_t  i = 0; i < m_threads.size(); ++i)
    {
        if(m_threadFinishedFlags[i]->load())
        {
            m_threads.erase(m_threads.begin() + i);
            m_threadFinishedFlags.erase(m_threadFinishedFlags.begin() + i);
            --i;
        }
    }
}
