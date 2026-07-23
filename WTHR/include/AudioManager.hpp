#pragma once

#include <entt/entity/registry.hpp>
#include <memory>
#include <string>

class AudioManager {
public:
    AudioManager();
    ~AudioManager(); // MUST be declared here, defined in .cpp!

    void LoadSound(const std::string& path);

    // Prevent accidental copying (unique_ptr is non-copyable)
    AudioManager(const AudioManager&) = delete;
    AudioManager& operator=(const AudioManager&) = delete;

    // Allow moving if needed
    AudioManager(AudioManager&&) noexcept = default;
    AudioManager& operator=(AudioManager&&) noexcept = default;

    void Initialize() {}
    void PlaySound(const std::string& path);
    void Update(entt::registry& registry);

private:
    struct AudioEngineImpl;
    struct SoundWrapper;
    std::unique_ptr<AudioEngineImpl> m_Engine;
    std::unordered_map<uint32_t, SoundWrapper> m_Sounds;
    
};