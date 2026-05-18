#include <slrdframework/camera.hpp>

#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/trigonometric.hpp>
#include <glm/ext/matrix_transform.hpp>

Camera::Camera () {}

void Camera::recalculate () const {
    glm::vec3 right = glm::cross (m_center, { 0, 1, 0 });
    glm::vec3 up = glm::cross (right, m_center);
    
    m_lookat = glm::lookAt (m_eye, m_eye - m_center, up);
}

const glm::mat4& Camera::getMatrix () const {
    if (m_requiresUpdate) {
        recalculate ();
        m_requiresUpdate = false;
    }

    return m_lookat;
}

void Camera::move (const glm::vec3& vec) {
    m_eye += vec;
    m_requiresUpdate = true;
}

void Camera::setCenter (const glm::vec3& position) {
    m_center = position;
    m_requiresUpdate = true;
}
void Camera::setEye (const glm::vec3& eye) {
    m_eye = eye;
    m_requiresUpdate = true;
}

ControlledCamera::ControlledCamera (const std::unique_ptr<Input>& input) :
        m_input(input.get ()) {
    input->getMouse (m_px, m_py);
}

bool ControlledCamera::update (float speed) {
    bool changed = false;

    int cx = 0, cy = 0;
    m_input->getMouse (cx, cy);

    int dx = cx - m_px;
    int dy = cy - m_py;

    m_px = cx;
    m_py = cy;

    if (dx || dy) {
        m_pitch -= dy * m_sensitivity;
        m_yaw   += dx * m_sensitivity;
            
        m_pitch = glm::clamp (m_pitch, -89.f, 89.f);

        float dpitch = glm::radians (m_pitch);
        float dyaw   = glm::radians (m_yaw);

        m_center.x = glm::cos (dyaw) * glm::cos (dpitch);
        m_center.y = glm::sin (dpitch);
        m_center.z = glm::sin (dyaw) * glm::cos (dpitch);
        m_center = glm::normalize (m_center);

        changed = true;
    }

    /*std::cout << std::format ("Center: {} {} {}\n",*/
    /*        m_center.x, m_center.y, m_center.z);*/
    /*std::cout << std::format ("Eye: {} {} {}\n",*/
    /*        m_eye.x, m_eye.y, m_eye.z);*/
    if (m_input->isKeyPressed (SDL_SCANCODE_W)) {
        move (-m_center * speed);
        changed = true;
    }
    if (m_input->isKeyPressed (SDL_SCANCODE_S)) {
        move (m_center * speed);
        changed = true;
    }
    if (m_input->isKeyPressed (SDL_SCANCODE_A)) {
        glm::vec3 right;
        glm::vec3 up (0, 1, 0);
        right = glm::normalize (glm::cross (up, m_center));

        move (-right * speed);
        changed = true;
    }
    if (m_input->isKeyPressed (SDL_SCANCODE_D)) {
        glm::vec3 right;
        glm::vec3 up (0, 1, 0);
        right = glm::normalize (glm::cross (up, m_center));

        move (right * speed);
        changed = true;
    }
    if (m_input->isKeyPressed (SDL_SCANCODE_Q)) {
        move ({ 0, speed, 0 });
        changed = true;
    }
    if (m_input->isKeyPressed (SDL_SCANCODE_E)) {
        move ({ 0, -speed, 0});
        changed = true;
    }

    if (changed) {
        m_requiresUpdate = true;
    }

    return changed;
}
