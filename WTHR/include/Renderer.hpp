#pragma once
#include <pch.hpp>
#include <Framebuffer.hpp>
#include <entt/entt.hpp>
#include <ScriptEditor.hpp>
//#include <PhysicsWorld.hpp>

class Scene;
class Shader;


class Renderer
{
public:
    Renderer();
    ~Renderer()
    {

    }

    void Init();
    void Clear(const glm::vec3& color = { 0.1f, 0.1f, 0.1f });
    void RenderScene(Scene&,Shader& );
    void RenderGizmo(Scene& scene, Shader& shader);
    glm::mat4 BuildModelMatrix(const Transform& transform);

    void RenderPicking(Scene&,int,int);
    void HandlePickingClick(Scene& scene, double mouseX, double mouseY,entt::entity&);

    void setSize(int x, int y) { width = x; height = y; }

    ImGuizmo::OPERATION gizmoType;
    ScriptEditor m_Editor;
private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_ShaderProgram = 0;
    Framebuffer m_ObjectPicking;

    float width, height;
    Shader pickingShader;
};
