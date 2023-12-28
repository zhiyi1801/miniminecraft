#include "player.h"
#include <QString>



Player::Player(glm::vec3 pos, const Terrain &terrain)
    : Entity(pos), m_velocity(0,0,0), m_acceleration(0,0,0),
      m_camera(pos + glm::vec3(0, 1.5f, 0)), mcr_terrain(terrain),
      mcr_camera(m_camera),flightMode(true)
{}

Player::~Player()
{}

void Player::tick(float dT, InputBundle &input) {
    processInputs(input);
    computePhysics(dT, mcr_terrain);
    m_camera.updateProjPlanes();
}

void Player::processInputs(InputBundle &inputs) {
    // TODO: Update the Player's velocity and acceleration based on the
    // state of the inputs.

    // Movement

    if (inputs.wPressed) {
        m_acceleration += m_forward;
    }
    if (inputs.aPressed) {
        m_acceleration -= m_right;
    }
    if (inputs.sPressed) {
        m_acceleration -= m_forward;
    }
    if (inputs.dPressed) {
        m_acceleration += m_right;
    }

    if (flightMode){
        if (inputs.ePressed) {
            m_acceleration += m_up;
        }
        if (inputs.qPressed) {
            m_acceleration -= m_up;
        }
    }

    if ( !flightMode && inputs.spacePressed){
        m_acceleration.y += 40.0f;
    }

    if (inputs.fPressed) {
        flightMode = !flightMode;
        m_acceleration = glm::vec3(0.0f);
        m_velocity= glm::vec3(0.0f);
    }
}

void Player::computePhysics(float dT, const Terrain &terrain) {

    float min_velocity = 0.01f;

    // apply some acceleration in the negative velocity direction
    // to simulate the drag and friction

    if (glm::length(m_velocity) > min_velocity){
        if(flightMode){
            m_velocity *= 0.7f;
        }else{
            m_velocity.x *= 0.7f;
            m_velocity.z *= 0.7f;
        }

    }



    // add some gravity
    if (!flightMode)
        m_acceleration.y += -10.0f;

    // clamp acceleration
    m_acceleration = glm::clamp(m_acceleration, -10.0f,10.0f);

    //dT = glm::clamp(dT, 0, )

    m_velocity += m_acceleration * dT;

    dT = glm::clamp(dT, 0.f, 1.f);

    m_velocity = glm::clamp(m_velocity, -10.0f,10.0f);

    // detect the collision when the flightMode is off

    if (!flightMode){
        // get the vertices of the position of the player is currently in
        // the vertices should be integers, by floor and ceiling the postion of the player
        std::vector<glm::vec3> vertices = getVoxelVertices(m_position);
        std::vector<glm::vec3> camera_vertices = getVoxelVertices(m_camera.mcr_position);
        vertices.insert(vertices.end(), camera_vertices.begin(), camera_vertices.end());

        // now perform collision detection by casting a ray from the vertices and see if it hits the terrain
        for (int i = 0; i < int(vertices.size()); i++) {
            glm::vec3 strideLength = m_velocity * dT;
            // check the length of the step
            glm::vec3 direction = strideLength;
            if (glm::length(strideLength) > 1.0f) {
                direction = glm::normalize(strideLength);
            }
            glm::vec3 oneStep = direction;
            glm::vec3 ray = vertices[i];

            do{
                ray = vertices[i] + direction;
                if (terrain.getBlockAt(ray.x, vertices[i].y, vertices[i].z) != EMPTY) {
                    m_velocity.x = 0;
                }
                if (terrain.getBlockAt(vertices[i].x, ray.y, vertices[i].z) != EMPTY) {
                    m_velocity.y = 0;
                }
                if (terrain.getBlockAt(vertices[i].x, vertices[i].y, ray.z) != EMPTY) {
                    m_velocity.z = 0;
                }
                // scale direction by one unit
                direction += oneStep;
            }while(glm::length(direction)<glm::length(strideLength));

        }
    }

    if (flightMode){
        moveAlongVector(m_velocity * dT);
    }else{
        float y = m_velocity.y;
        m_velocity.y = 0;
        moveAlongVector(m_velocity * dT);
        m_velocity.y = y;

        glm::vec3 yVelocity = glm::vec3(0, m_velocity.y, 0);
        moveAlongVector(yVelocity * dT * 0.1f);
    }




    if (glm::length(m_velocity) < min_velocity) {
        m_velocity = glm::vec3(0);
    }

    // zero out the acceleration
    if (flightMode){
        m_acceleration = m_acceleration * 0.9f;
    }else{
        // zero out x and z acceleration
        m_acceleration.x = m_acceleration.x*0.9f;
        m_acceleration.z = m_acceleration.z*0.9f;
    }


}

std::vector<glm::vec3> Player::getVoxelVertices(glm::vec3 pos){
    std::vector<glm::vec3> vertices;
    vertices.push_back(glm::vec3(floor(pos.x), floor(pos.y), floor(pos.z)));
    vertices.push_back(glm::vec3(floor(pos.x), floor(pos.y), ceil(pos.z)));
    vertices.push_back(glm::vec3(floor(pos.x), ceil(pos.y), floor(pos.z)));
    vertices.push_back(glm::vec3(floor(pos.x), ceil(pos.y), ceil(pos.z)));
    vertices.push_back(glm::vec3(ceil(pos.x), floor(pos.y), floor(pos.z)));
    vertices.push_back(glm::vec3(ceil(pos.x), floor(pos.y), ceil(pos.z)));
    vertices.push_back(glm::vec3(ceil(pos.x), ceil(pos.y), floor(pos.z)));
    vertices.push_back(glm::vec3(ceil(pos.x), ceil(pos.y), ceil(pos.z)));
    return vertices;
}

void Player::setCameraWidthHeight(unsigned int w, unsigned int h) {
    m_camera.setWidthHeight(w, h);
}

void Player::moveAlongVector(glm::vec3 dir) {
    Entity::moveAlongVector(dir);
    m_camera.moveAlongVector(dir);
}
void Player::moveForwardLocal(float amount) {
    Entity::moveForwardLocal(amount);
    m_camera.moveForwardLocal(amount);
}
void Player::moveRightLocal(float amount) {
    Entity::moveRightLocal(amount);
    m_camera.moveRightLocal(amount);
}
void Player::moveUpLocal(float amount) {
    Entity::moveUpLocal(amount);
    m_camera.moveUpLocal(amount);
}
void Player::moveForwardGlobal(float amount) {
    Entity::moveForwardGlobal(amount);
    m_camera.moveForwardGlobal(amount);
}
void Player::moveRightGlobal(float amount) {
    Entity::moveRightGlobal(amount);
    m_camera.moveRightGlobal(amount);
}
void Player::moveUpGlobal(float amount) {
    Entity::moveUpGlobal(amount);
    m_camera.moveUpGlobal(amount);
}
void Player::rotateOnForwardLocal(float degrees) {
    Entity::rotateOnForwardLocal(degrees);
    m_camera.rotateOnForwardLocal(degrees);
}
void Player::rotateOnRightLocal(float degrees) {
    Entity::rotateOnRightLocal(degrees);
    m_camera.rotateOnRightLocal(degrees);
}
void Player::rotateOnUpLocal(float degrees) {
    Entity::rotateOnUpLocal(degrees);
    m_camera.rotateOnUpLocal(degrees);
}
void Player::rotateOnForwardGlobal(float degrees) {
    Entity::rotateOnForwardGlobal(degrees);
    m_camera.rotateOnForwardGlobal(degrees);
}
void Player::rotateOnRightGlobal(float degrees) {
    Entity::rotateOnRightGlobal(degrees);
    m_camera.rotateOnRightGlobal(degrees);
}
void Player::rotateOnUpGlobal(float degrees) {
    Entity::rotateOnUpGlobal(degrees);
    m_camera.rotateOnUpGlobal(degrees);
}

QString Player::posAsQString() const {
    std::string str("( " + std::to_string(m_position.x) + ", " + std::to_string(m_position.y) + ", " + std::to_string(m_position.z) + ")");
    return QString::fromStdString(str);
}
QString Player::velAsQString() const {
    std::string str("( " + std::to_string(m_velocity.x) + ", " + std::to_string(m_velocity.y) + ", " + std::to_string(m_velocity.z) + ")");
    return QString::fromStdString(str);
}
QString Player::accAsQString() const {
    std::string str("( " + std::to_string(m_acceleration.x) + ", " + std::to_string(m_acceleration.y) + ", " + std::to_string(m_acceleration.z) + ")");
    return QString::fromStdString(str);
}
QString Player::lookAsQString() const {
    std::string str("( " + std::to_string(m_forward.x) + ", " + std::to_string(m_forward.y) + ", " + std::to_string(m_forward.z) + ")");
    return QString::fromStdString(str);
}

glm::vec3 Player::getPos()const
{
    return m_position;
}

//***********************************************

bool gridMarch(glm::vec3 rayOrigin, glm::vec3 rayDirection, const Terrain &terrain, float *out_dist, glm::ivec3 *out_blockHit) {
    float maxLen = glm::length(rayDirection); // Farthest we search
    glm::ivec3 currCell = glm::ivec3(glm::floor(rayOrigin));
    rayDirection = glm::normalize(rayDirection); // Now all t values represent world dist.

    float curr_t = 0.f;
    while(curr_t < maxLen) {
        float min_t = glm::sqrt(3.f);
        float interfaceAxis = -1; // Track axis for which t is smallest
        for(int i = 0; i < 3; ++i) { // Iterate over the three axes
            if(rayDirection[i] != 0) { // Is ray parallel to axis i?
                float offset = glm::max(0.f, glm::sign(rayDirection[i])); // See slide 5
                // If the player is *exactly* on an interface then
                // they'll never move if they're looking in a negative direction
                if(currCell[i] == rayOrigin[i] && offset == 0.f) {
                    offset = -1.f;
                }
                int nextIntercept = currCell[i] + offset;
                float axis_t = (nextIntercept - rayOrigin[i]) / rayDirection[i];
                axis_t = glm::min(axis_t, maxLen); // Clamp to max len to avoid super out of bounds errors
                if(axis_t < min_t) {
                    min_t = axis_t;
                    interfaceAxis = i;
                }
            }
        }
        if(interfaceAxis == -1) {
            throw std::out_of_range("interfaceAxis was -1 after the for loop in gridMarch!");
        }
        curr_t += min_t; // min_t is declared in slide 7 algorithm
        rayOrigin += rayDirection * min_t;
        glm::ivec3 offset = glm::ivec3(0,0,0);
        // Sets it to 0 if sign is +, -1 if sign is -
        offset[interfaceAxis] = glm::min(0.f, glm::sign(rayDirection[interfaceAxis]));
        currCell = glm::ivec3(glm::floor(rayOrigin)) + offset;
        // If currCell contains something other than EMPTY, return
        // curr_t
        BlockType cellType = terrain.getBlockAt(currCell.x, currCell.y, currCell.z);
        if(cellType != EMPTY) {
            *out_blockHit = currCell;
            *out_dist = glm::min(maxLen, curr_t);
            return true;
        }
    }
    *out_dist = glm::min(maxLen, curr_t);
    return false;
}

void Player::addBlock() const{
    float currDist;
    glm::ivec3 outBlock;
    if(gridMarch(m_camera.mcr_position, this->m_forward * 3.0f, mcr_terrain, &currDist, &outBlock)){
        BlockType type = mcr_terrain.getBlockAt(outBlock.x, outBlock.y, outBlock.z);
        glm::vec3 intersection = m_camera.mcr_position + this->m_forward * currDist;
        glm::vec3 offset = glm::vec3(intersection.x - outBlock.x, intersection.y - outBlock.y, intersection.z - outBlock.z);
        Chunk *c = nullptr;
        if (fabs(offset.z) < 0.001f){
            mcr_terrain.setBlockAt(outBlock.x, outBlock.y, outBlock.z - 1, type);
            c = mcr_terrain.getChunkAt(outBlock.x, outBlock.z - 1).get();
        } else if (fabs(offset.z - 1.f) < 0.001f){
            mcr_terrain.setBlockAt(outBlock.x, outBlock.y, outBlock.z + 1, type);
            c = mcr_terrain.getChunkAt(outBlock.x, outBlock.z + 1).get();
        } else if (fabs(offset.x) < 0.001f) {
            mcr_terrain.setBlockAt(outBlock.x - 1, outBlock.y, outBlock.z, type);
            c = mcr_terrain.getChunkAt(outBlock.x - 1, outBlock.z).get();
        }else if (fabs(offset.x - 1.f) < 0.001f) {
            mcr_terrain.setBlockAt(outBlock.x + 1, outBlock.y, outBlock.z, type);
            c = mcr_terrain.getChunkAt(outBlock.x + 1, outBlock.z).get();
        } else if (fabs(offset.y) < 0.001f) {
            mcr_terrain.setBlockAt(outBlock.x, outBlock.y - 1, outBlock.z, type);
            c = mcr_terrain.getChunkAt(outBlock.x, outBlock.z).get();
        } else if (fabs(offset.y - 1.f) < 0.001f){
            mcr_terrain.setBlockAt(outBlock.x, outBlock.y + 1, outBlock.z, type);
            c = mcr_terrain.getChunkAt(outBlock.x, outBlock.z).get();
        }
        if (c){
            c->updateNeighbors();
        }
    }
}

void Player::removeBlock() const{
    float currDist;
    glm::ivec3 outBlock;
    if(gridMarch(m_camera.mcr_position, this->m_forward * 3.0f, mcr_terrain, &currDist, &outBlock)){
        mcr_terrain.setBlockAt(outBlock.x, outBlock.y, outBlock.z, EMPTY);
        Chunk *c = mcr_terrain.getChunkAt(outBlock.x, outBlock.z).get();
        if (c){
            c->updateNeighbors();
        }
    }
}

void Player::updateProjPlanes()
{
    this->m_camera.updateProjPlanes();
}

void Player::updateOrthoPlanes()
{
    this->m_camera.updateOrthoPlanes();
}
