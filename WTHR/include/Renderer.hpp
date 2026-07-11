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
    void SetSunDirection(glm::vec3 dir) { m_SunDirection = dir; }
    void SetSunDiffuse(glm::vec3 diff) { m_SunDiffuse = diff; }
    void SetSunAmbient(glm::vec3 amb) { m_SunAmbient = amb; }
    glm::vec3 GetSunDirection() { return m_SunDirection; }
    glm::vec3 GetSunDiffuse() {return m_SunDiffuse;}
    glm::vec3 GetSunAmbient() {return m_SunAmbient;}
private:
    GLuint m_VAO = 0;
    GLuint m_VBO = 0;
    GLuint m_ShaderProgram = 0;
    Framebuffer m_ObjectPicking;
    glm::vec3 m_SunDirection = glm::normalize(glm::vec3{ -0.5f, -1.0f, -0.3f });
    glm::vec3 m_SunAmbient = { 0.15f, 0.18f, 0.22f };
    glm::vec3 m_SunDiffuse = { 1.0f,  0.95f, 0.85f };
    glm::vec3 m_SunSpecular = { 1.0f,  0.95f, 0.85f };

    float width, height;
    Shader pickingShader;
};
