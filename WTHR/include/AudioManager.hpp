#include <miniaudio.h>

// AudioManager.h
class AudioManager {
public:
    void Initialize();
    void PlaySound(const std::string& path);
    void Update(entt::registry& registry);

private:
    // Keep miniaudio completely out of the header file!
    // This forward declaration keeps your build times fast.
    struct AudioEngineImpl;
    std::unique_ptr<AudioEngineImpl> m_engine;
};