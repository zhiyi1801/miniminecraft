#include "entity.h"

Entity::Entity()
    :  Entity(glm::vec3(0,0,0))
{}

Entity::Entity(glm::vec3 pos)
    : m_forward(0,0,-1), m_right(1,0,0), m_up(0,1,0), m_position(pos), mcr_position(m_position)
{}

Entity::Entity(const Entity &e)
    : m_forward(e.m_forward), m_right(e.m_right), m_up(e.m_up), m_position(e.m_position), mcr_position(m_position)
{}

Entity::~Entity()
{}


void Entity::moveAlongVector(glm::vec3 dir) {
    m_position += dir;
}

void Entity::moveForwardLocal(float amount) {
    m_position += amount * m_forward;
}

void Entity::moveRightLocal(float amount) {
    m_position += amount * m_right;
}

void Entity::moveUpLocal(float amount) {
    m_position += amount * m_up;
}

void Entity::moveForwardGlobal(float amount) {
    m_position += glm::vec3(0, 0, amount);
}

void Entity::moveRightGlobal(float amount) {
    m_position += glm::vec3(amount, 0, 0);
}

void Entity::moveUpGlobal(float amount) {
    m_position += glm::vec3(0, amount, 0);
}

void Entity::rotateOnForwardLocal(float degrees) {
    float rad = glm::radians(degrees);
    glm::mat4 rot(glm::rotate(glm::mat4(), rad, m_forward));
    m_right = glm::vec3(rot * glm::vec4(m_right, 0.f));
    m_up = glm::vec3(rot * glm::vec4(m_up, 0.f));
}

void Entity::rotateOnRightLocal(float degrees) {
    float rad = glm::radians(degrees);
    glm::mat4 rot(glm::rotate(glm::mat4(), rad, m_right));
    m_forward = glm::vec3(rot * glm::vec4(m_forward, 0.f));
    m_up = glm::vec3(rot * glm::vec4(m_up, 0.f));
}

void Entity::rotateOnUpLocal(float degrees) {
    float rad = glm::radians(degrees);
    glm::mat4 rot(glm::rotate(glm::mat4(), rad, m_up));
    m_right = glm::vec3(rot * glm::vec4(m_right, 0.f));
    m_forward = glm::vec3(rot * glm::vec4(m_forward, 0.f));
}


void Entity::rotateOnForwardGlobal(float degrees) {
    float rad = glm::radians(degrees);
    m_forward = glm::vec3(glm::rotate(glm::mat4(), rad, glm::vec3(0,0,1)) * glm::vec4(m_forward, 0.f));
    m_right = glm::vec3(glm::rotate(glm::mat4(), rad, glm::vec3(0,0,1)) * glm::vec4(m_right, 0.f));
    m_up = glm::vec3(glm::rotate(glm::mat4(), rad, glm::vec3(0,0,1)) * glm::vec4(m_up, 0.f));
}

void Entity::rotateOnRightGlobal(float degrees) {
    float rad = glm::radians(degrees);
    m_forward = glm::vec3(glm::rotate(glm::mat4(), rad, glm::vec3(1,0,0)) * glm::vec4(m_forward, 0.f));
    m_right = glm::vec3(glm::rotate(glm::mat4(), rad, glm::vec3(1,0,0)) * glm::vec4(m_right, 0.f));
    m_up = glm::vec3(glm::rotate(glm::mat4(), rad, glm::vec3(1,0,0)) * glm::vec4(m_up, 0.f));
}

void Entity::rotateOnUpGlobal(float degrees) {
    float rad = glm::radians(degrees);
    m_forward = glm::vec3(glm::rotate(glm::mat4(), rad, glm::vec3(0,1,0)) * glm::vec4(m_forward, 0.f));
    m_right = glm::vec3(glm::rotate(glm::mat4(), rad, glm::vec3(0,1,0)) * glm::vec4(m_right, 0.f));
    m_up = glm::vec3(glm::rotate(glm::mat4(), rad, glm::vec3(0,1,0)) * glm::vec4(m_up, 0.f));
}

void Entity::initiateAxes()
{
    m_forward = glm::vec3(0.f, 0.f, -1.f);
    m_up = glm::vec3(0.f, 1.f, 0.f);
    m_right = glm::vec3(1.f, 0.f, 0.f);
}

void Entity::setForward(const glm::vec3 &_forward)
{
    glm::vec3 forward = _forward;
    forward = glm::normalize(forward);

    glm::vec3 axis = glm::cross(this->m_forward, forward);

    float cosTheta = 0;
    float thetaRad = 0;
    glm::mat4 rotateMat = glm::mat4();

    if(glm::length2(axis) != 0)
    {
        axis = glm::normalize(axis);
        cosTheta = glm::dot(glm::normalize(this->m_forward), forward);
        cosTheta = glm::clamp(cosTheta, -1.f, 1.f);
        thetaRad = glm::acos(cosTheta);
        rotateMat = glm::rotate(glm::mat4(), thetaRad, axis);
    }

    m_forward = forward;
    m_right = glm::vec3(rotateMat * glm::vec4(m_right, 0.f));
    m_up = glm::vec3(rotateMat * glm::vec4(m_up, 0.f));

    m_right = glm::cross(m_forward, m_up);
    m_up = glm::cross(m_right, m_forward);

    m_forward = glm::normalize(m_forward);
    m_right = glm::normalize(m_right);
    m_up = glm::normalize(m_up);
}

glm::vec3 Entity::getPos()const
{
    return this->m_position;
}

void Entity::setPos(const glm::vec3 &_pos)
{
    this->m_position = _pos;
}
