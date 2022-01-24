
#ifdef DEBUG_BUILD
#include <string>
#ifdef DEBUG_BUILD
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#else
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_INFO
#endif
#include <spdlog/spdlog.h>

void GLAPIENTRY opengl_messageCallback (GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam)
{
    std::string source_string;
    switch (source) {
        case GL_DEBUG_SOURCE_API:
            source_string = "api";
            break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:
            source_string = "window_system";
            break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER:
            source_string = "shader_compiler";
            break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:
            source_string = "third_party";
            break;
        case GL_DEBUG_SOURCE_APPLICATION:
            source_string = "application";
            break;
        case GL_DEBUG_SOURCE_OTHER:
        default:
            source_string = "UNKNOWN";
            break;
    }

    std::string type_string;
	switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            type_string = "error";
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            type_string = "deprecated_behavior";
            break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            type_string = "undefined_behavior";
            break;
        case GL_DEBUG_TYPE_PORTABILITY:
            type_string = "portability";
            break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            type_string = "performance";
            break;
        case GL_DEBUG_TYPE_MARKER:
            type_string = "type_marker";
            break;
        case GL_DEBUG_TYPE_OTHER:
        default:
            type_string = "other";
            break;
	}

    std::string severity_string = "unknown";
	switch (severity){
        case GL_DEBUG_SEVERITY_LOW:
            severity_string = "low";
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            severity_string = "medium";
            break;
        case GL_DEBUG_SEVERITY_HIGH:
            severity_string = "high";
            break;
        case GL_DEBUG_SEVERITY_NOTIFICATION:
            severity_string = "notification";
            break;
	}

    #define OPENGL_MESSAGE "OpenGL Message (id={:#x}, type={}, source={}, severity={}): {}", id, type_string, source_string, severity_string, message
    if (type == GL_DEBUG_TYPE_ERROR) {
        spdlog::error(OPENGL_MESSAGE);
    } else {
        spdlog::debug(OPENGL_MESSAGE);
    }
}
#endif
