#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace graphics {

    // Default camera values
    const float YAW         = -90.0f;
    const float PITCH       =  0.0f;
    const float SPEED       =  2.5f;
    const float SENSITIVITY =  0.1f;
    const float ZOOM        =  45.0f;


    // An abstract camera class that processes input and calculates the corresponding Euler Angles, Vectors and Matrices for use in OpenGL
    class Camera
    {
    public:
        enum class Movement {
            FORWARD_BACK,
            UP_DOWN,
            LEFT_RIGHT,
        };

        // Constructor with vectors
        Camera(glm::vec3 position = glm::vec3(0.0f, 1.0f, 2.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = YAW, float pitch = PITCH) : m_front(glm::vec3(0.0f, 0.0f, -1.0f)), m_movement_speed(SPEED), m_sensitivity(SENSITIVITY), m_zoom(ZOOM)
        {
            m_position = position;
            m_world_up = up;
            m_yaw = yaw;
            m_pitch = pitch;
            updateCameraVectors();
        }
        // Constructor with scalar values
        Camera(float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch) : m_front(glm::vec3(0.0f, 0.0f, -1.0f)), m_movement_speed(SPEED), m_sensitivity(SENSITIVITY), m_zoom(ZOOM)
        {
            m_position = glm::vec3(posX, posY, posZ);
            m_world_up = glm::vec3(upX, upY, upZ);
            m_yaw = yaw;
            m_pitch = pitch;
            updateCameraVectors();
        }

        inline void beginFrame (const float dt) { m_delta_time = dt; }

        // Returns the view matrix calculated using Euler Angles and the LookAt Matrix
        glm::mat4 view()
        {
            return glm::lookAt(m_position, m_position + m_front, m_up);
        }

        // Processes input received from any keyboard-like input system. Accepts input parameter in the form of camera defined ENUM (to abstract it from windowing systems)
        void move (Movement direction, float amount)
        {
            float velocity = amount * m_delta_time;
            switch (direction) {
                case Movement::FORWARD_BACK:
                    m_position -= m_front * velocity;
                    break;
                case Movement::LEFT_RIGHT: 
                    m_position += m_right * velocity;
                    break;
                case Movement::UP_DOWN: 
                    m_position += m_up * velocity;
                    break;
                default:
                    break;
            };
        }

        void pan (const glm::vec3& direction) {
            m_position += direction * m_delta_time;
        }

        // Processes input received from a mouse input system. Expects the offset value in both the x and y direction.
        void orient (float x_offset, float y_offset, bool constrain_pitch = true)
        {
            x_offset *= m_sensitivity;
            y_offset *= m_sensitivity;

            m_yaw   += x_offset;
            m_pitch += y_offset;

            // Make sure that when pitch is out of bounds, screen doesn't get flipped
            if (constrain_pitch)
            {
                if (m_pitch > 89.0f) {
                    m_pitch = 89.0f;
                }
                if (m_pitch < -89.0f {
                    m_pitch = -89.0f;
                })
            }

            // Update m_front, m_right and m_up Vectors using the updated Euler angles
            updateCameraVectors();
        }

        // Processes input received from a mouse scroll-wheel event. Only requires input on the vertical wheel-axis
        void zoom (float y_offset)
        {
            if (m_zoom >= 1.0f && m_zoom <= 45.0f)
                m_zoom -= y_offset;
            if (m_zoom <= 1.0f)
                m_zoom = 1.0f;
            if (m_zoom >= 45.0f)
                m_zoom = 45.0f;
        }

    private:
        // Calculates the front vector from the Camera's (updated) Euler Angles
        void updateCameraVectors()
        {
            // Calculate the new m_front vector
            glm::vec3 front;
            front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
            front.y = sin(glm::radians(m_pitch));
            front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
            m_front = glm::normalize(front);
            // Also re-calculate the m_right and m_up vector
            m_right = glm::normalize(glm::cross(m_front, m_world_up));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
            m_up    = glm::normalize(glm::cross(m_right, m_front));
        }

        // Camera Attributes
        glm::vec3 m_position;
        glm::vec3 m_front;
        glm::vec3 m_up;
        glm::vec3 m_right;
        glm::vec3 m_world_up;
        // Euler Angles
        float m_yaw;
        float m_pitch;
        // Camera options
        float m_movement_speed;
        float m_sensitivity; // Mouse/joystick sensitivity
        float m_zoom;

        float m_delta_time;
    };

} // graphics::
