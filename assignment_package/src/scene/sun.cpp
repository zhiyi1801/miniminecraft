#include "sun.h"


Sun::Sun(const glm::vec3 &_pos, Player *_player):m_zeroPos(_pos), m_player(_player), m_timer(), m_timeOffset(0), m_cam(m_player->getPos() + m_zeroPos)
{
    glm::vec3 playerXZ = m_player->getPos();
    playerXZ[1] = SUN_PLANE_HEIGHT;
    if(glm::length2(glm::cross(glm::normalize(m_zeroPos), SUMMIT)) < 1e-3)
    {
        if(glm::length2(glm::cross(glm::vec3(1.f, 0.f, 0.f), SUMMIT)) >= 0.3)
        {
            m_zeroPos = glm::vec3(1.f, 0.f, 0.f) * glm::length(m_zeroPos);
        }

        else if(glm::length2(glm::cross(glm::vec3(0.f, 1.f, 0.f), SUMMIT)) >= 0.3)
        {
            m_zeroPos = glm::vec3(0.f, 1.f, 0.f) * glm::length(m_zeroPos);
        }

        else if(glm::length2(glm::cross(glm::vec3(0.f, 0.f, 1.f), SUMMIT)) >= 0.3)
        {
            m_zeroPos = glm::vec3(0.f, 0.f, 1.f) * glm::length(m_zeroPos);
        }
        glm::vec3 sunPos = playerXZ;
        sunPos += m_zeroPos;
        m_cam.setPos(sunPos);
    }

    m_cam.initiateAxes();
    m_cam.setForward(playerXZ - m_cam.getPos());
    m_rotateAxis = glm::cross(m_zeroPos, SUMMIT);
    m_rotateAxis = glm::normalize(m_rotateAxis);
    m_timer.start();
}

// return current time
int Sun::tick()
{
    glm::vec3 playerXZ = m_player->getPos();
    playerXZ[1] = SUN_PLANE_HEIGHT;
    unsigned int curTime = m_timer.elapsed() + m_timeOffset /*+ CYCLE_TIME/3*/;
    if(curTime > CYCLE_TIME)
    {
        curTime = curTime % CYCLE_TIME;
        m_timeOffset = curTime;
        m_timer.restart();
    }
//    curTime /= (CYCLE_TIME/600);
//    curTime *= (CYCLE_TIME/600);
    curTime = CYCLE_TIME * 1.f/48;
    //curTime = CYCLE_TIME / 4;
    float rotateRad = (static_cast<double>(curTime)/CYCLE_TIME) * _2_PI;
    glm::mat3 rotateMat = glm::mat3(glm::rotate(glm::mat4(), rotateRad, m_rotateAxis));
    glm::vec3 sunPos = playerXZ;
    sunPos += rotateMat * m_zeroPos;

    m_cam.setPos(sunPos);
    m_cam.initiateAxes();
    m_cam.setForward(playerXZ - m_cam.getPos());

    return curTime;
}

int Sun::getCycleTime()
{
    return CYCLE_TIME;
}
