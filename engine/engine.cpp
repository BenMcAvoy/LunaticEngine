#include "engine.h"

#include "debug/console.h"
#include "utils.h"

using namespace Lunatic;

Engine* Engine::s_instance = nullptr;

Engine::Engine(uint32_t width, uint32_t height, std::string_view title)
    : m_width(width), m_height(height), m_title(title) {
    LUNA_ASSERT(s_instance == nullptr);
    s_instance = this;
}
Engine::~Engine() {
    LUNA_ASSERT(s_instance == this);
    s_instance = nullptr;
}
Engine& Engine::GetInstance() {
    LUNA_ASSERT(s_instance != nullptr);
    return *s_instance;
}

void badFunction() {
    glDrawArrays(GL_TRIANGLES, 0, 3);
}

void Engine::run() {
    LUNA_ASSERT(s_instance == this);

    Debug::SetupConsole();

    LUNA_ASSERT(glfwInit(), "Failed to initialize GLFW");

#ifdef LUNATIC_DEBUG
    glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);  // Required on macOS
#endif

    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    LUNA_ASSERT(m_window != nullptr, "Failed to create GLFW window");
    glfwMakeContextCurrent(m_window);

    // Load OpenGL functions using glad
    LUNA_ASSERT(gladLoadGLLoader((GLADloadproc)glfwGetProcAddress), "Failed to initialize OpenGL context");
    LUNA_ASSERT(glGetError() == GL_NO_ERROR, "Failed to initialize OpenGL context");

#ifdef LUNATIC_DEBUG
    int flags;
    glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
    if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
        glEnable(GL_DEBUG_OUTPUT);
        glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);  // Optional but useful for breakpoints
        glDebugMessageCallback(CB_OnDebugMessage, nullptr);
        glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
    }
#endif

    // Set userdata
    glfwSetWindowUserPointer(m_window, this);

    // Set callbacks
    glfwSetKeyCallback(m_window, CB_OnKeyboard);
    glfwSetMouseButtonCallback(m_window, CB_OnMouseButton);
    glfwSetCursorPosCallback(m_window, CB_OnMouseMove);
    glfwSetScrollCallback(m_window, CB_OnMouseScroll);
    glfwSetWindowSizeCallback(m_window, CB_OnWindowResize);
    glfwSetErrorCallback(CB_OnError);

    // Set the viewport
    glViewport(0, 0, m_width, m_height);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // Enable Docking
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::Begin("Hello, Lunatic!");

        if (ImGui::Button("Debug"))
            LUNADEBUG("Debug button pressed");
        if (ImGui::Button("Info"))
            LUNAINFO("Info button pressed");
        if (ImGui::Button("Warn"))
            LUNAWARN("Warn button pressed");
        if (ImGui::Button("Error"))
            LUNERROR("Error button pressed");
        if (ImGui::Button("Critical"))
            LUNCRITICAL("Critical button pressed");
        if (ImGui::Button("Bad Function"))
            badFunction();

        ImGui::End();

        Debug::Console::GetInstance().renderWindow();

        // Rendering
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}

void Engine::CB_OnKeyboard(GLFWwindow* window, int key, int scancode, int action, int mods) {
    auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    LUNA_ASSERT(engine != nullptr);

    if (action == GLFW_PRESS) {
        if (key == GLFW_KEY_ESCAPE) {
            glfwSetWindowShouldClose(window, true);
        }
    }

    if (key >= 0 && key < GLFW_KEY_LAST) {
        engine->m_keys[key] = (action != GLFW_RELEASE);
    }
}
void Engine::CB_OnMouseButton(GLFWwindow* window, int button, int action, int mods) {
    auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    LUNA_ASSERT(engine != nullptr);

    if (button >= 0 && button < GLFW_MOUSE_BUTTON_LAST) {
        engine->m_mouseButtons[button] = (action != GLFW_RELEASE);
    }
}
void Engine::CB_OnMouseMove(GLFWwindow* window, double xpos, double ypos) {
    auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    LUNA_ASSERT(engine != nullptr);

    engine->m_mouseX = xpos;
    engine->m_mouseY = ypos;
    engine->m_scrollX = 0.0;
    engine->m_scrollY = 0.0;
}
void Engine::CB_OnMouseScroll(GLFWwindow* window, double xoffset, double yoffset) {
    auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    LUNA_ASSERT(engine != nullptr);

    engine->m_scrollX = xoffset;
    engine->m_scrollY = yoffset;
}
void Engine::CB_OnWindowResize(GLFWwindow* window, int width, int height) {
    auto engine = static_cast<Engine*>(glfwGetWindowUserPointer(window));
    LUNA_ASSERT(engine != nullptr);

    engine->m_width = width;
    engine->m_height = height;
    glViewport(0, 0, width, height);
}
void Engine::CB_OnError(int error, const char* description) {
    spdlog::error("GLFW Error ({}) : {}", error, description);
}
void Engine::CB_OnDebugMessage(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    switch (severity) {
        case GL_DEBUG_SEVERITY_HIGH:
            LUNAERROR("OpenGL Debug Message ({}) : {}", id, message);
            break;
        case GL_DEBUG_SEVERITY_MEDIUM:
            LUNAWARN("OpenGL Debug Message ({}) : {}", id, message);
            break;
        case GL_DEBUG_SEVERITY_LOW:
            LUNAINFO("OpenGL Debug Message ({}) : {}", id, message);
            break;
        default:
            LUNADEBUG("OpenGL Debug Message ({}) : {}", id, message);
            break;
    }

    spdlog::default_logger_raw()->flush();

    if (severity == GL_DEBUG_SEVERITY_HIGH) {
#ifdef __GNUC__
        __builtin_trap();
#elif defined(_MSC_VER)
        __debugbreak();
#else
        __asm__ volatile("int3");
#endif
    }
}