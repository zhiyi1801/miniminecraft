#ifndef SUN_H
#define SUN_H

#include "glm_includes.h"
#include "player.h"
#include <QElapsedTimer>
#include "smartpointerhelp.h"

class Sun
{
private:
    // position reletive to player
    const unsigned int CYCLE_TIME = 500000;
    const glm::vec3 SUMMIT = glm::mat3(glm::rotate(glm::mat4(), float(30.f/360.f*_2_PI), glm::vec3(0,0,1.f))) * glm::vec3(0, 1, 0);
    glm::vec3 m_zeroPos;
    Player *m_player;
    glm::vec3 m_rotateAxis;
    QElapsedTimer m_timer;
    int m_timeOffset;
    const int SUN_PLANE_HEIGHT = 138;

public:
    Camera m_cam;

    Sun(const glm::vec3 &_pos, Player *_player);
    int getCycleTime();

    int tick();
};

#endif // SUN_H
