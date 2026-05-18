#ifndef __CAMERA_HPP__
#define __CAMERA_HPP__

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "app.hpp"

class Camera {
protected:
    glm::vec3 m_eye { 0, 0, 0 };
    glm::vec3 m_center { 0, 0, 1 };

    mutable glm::mat4 m_lookat;
    mutable bool m_requiresUpdate = true;

    void recalculate () const;

public:
    Camera ();

    void setCenter (const glm::vec3& position);
    void setEye (const glm::vec3& eye);

    decltype(m_eye) getCenter () const {
        return m_center;
    }
    decltype(m_center) getEye () const {
        return m_eye;
    }

    void move (const glm::vec3& vec);

    const glm::mat4& getMatrix () const;
};

class ControlledCamera : public Camera {
private:
    int m_px;
    int m_py;

    float m_sensitivity = 0.5f;
    float m_pitch = 0.f;
    float m_yaw = 0.f;

    Input *m_input;

public:
    ControlledCamera (const std::unique_ptr<Input>& input);

    /* Update the camera using the app inputs */
    bool update (float speed);
};

#endif /* #define __CAMERA_HPP__ */
