#include "AudioManager.hpp"
#include <Components.hpp>
#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

// Define the wrappers inside the CPP file
struct AudioManager::SoundWrapper {
    ma_sound sound;

    ~SoundWrapper() {
        ma_sound_uninit(&sound);
    }
};

struct AudioManager::AudioEngineImpl {
    ma_engine engine;
};

// Constructor initializes the audio engine
AudioManager::AudioManager()
    : m_Engine(std::make_unique<AudioEngineImpl>())
{
    ma_result result = ma_engine_init(nullptr, &m_Engine->engine);
    if (result != MA_SUCCESS) {
        // Handle engine initialization error (e.g., spdlog or std::cerr)
    }
}

// Destructor cleans up miniaudio engine
AudioManager::~AudioManager()
{
    if (m_Engine) {
        ma_engine_uninit(&m_Engine->engine);
    }
}

void AudioManager::LoadSound(const std::string& path)
{
    auto newSound = std::make_unique<SoundWrapper>();

    // Initialize miniaudio sound inside our wrapper
    ma_result result = ma_sound_init_from_file(
        &m_Engine->engine,
        path.c_str(),
        0, nullptr, nullptr,
        &newSound->sound
    );

    if (result == MA_SUCCESS) {
        //m_sounds.push_back(std::move(newSound));
    }
}

void AudioManager::PlaySound(const std::string& path)
{
    if (path.empty()) return;

    // Fire-and-forget background sound playback managed by miniaudio
    ma_engine_play_sound(&m_Engine->engine, path.c_str(), nullptr);
}

void AudioManager::Update(entt::registry& registry)
{
    // Iterate over all entities with an AudioComponent
    // (If using 3D spatial audio, replace TransformComponent with your actual transform component name)
    auto view = registry.view<AudioComponent>();

    for (auto entity : view)
    {
        auto& audio = view.get<AudioComponent>(entity);

        // Optional: If you track entity world positions, update spatial sound positions here:
        /*
        if (registry.all_of<TransformComponent>(entity)) {
            auto& transform = registry.get<TransformComponent>(entity);
            // Example: update miniaudio listener or 3D sound position
            // ma_sound_set_position(&audio.soundInstance, transform.position.x, transform.position.y, transform.position.z);
        }
        */

        // 1. Handle Play Preview / Play Command
        if (audio.playRequested) {
            audio.playRequested = false;
            if (!audio.assetPath.empty()) {
                PlaySound(audio.assetPath);
                audio.isPlaying = true;
                audio.isPaused = false;
            }
        }

        // 2. Handle Stop Command
        if (audio.stopRequested) {
            audio.stopRequested = false;
            audio.isPlaying = false;
            audio.isPaused = false;

            // 3. Update spatial / volume parameters if managed via an active sound instance
            // ma_sound_set_volume(&sound, audio.volume);
            // ma_sound_set_pitch(&sound, audio.pitch);
            // ma_sound_set_looping(&sound, audio.isLooping);
        }
    }
}