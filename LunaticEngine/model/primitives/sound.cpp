#include "pch.h"

#include "sound.h"

#include "core/engine.h"

using namespace Lunatic;

SoundEmitter::SoundEmitter(std::string_view name) : Instance(name), sound_() {
	typeInfo = rttr::type::get<SoundEmitter>();

	if (!maInitialized_) {
		if (ma_engine_init(NULL, &engine_) != MA_SUCCESS) {
			spdlog::error("Failed to initialize miniaudio engine");
		}
		else {
			maInitialized_ = true;
			spdlog::info("Miniaudio engine initialized");
		}
	}
}

SoundEmitter::~SoundEmitter() {
    if (soundInitialized_) {
        ma_sound_stop(&sound_);
        ma_sound_uninit(&sound_);
        soundInitialized_ = false;
        isPlaying_ = false;
    }
}

void SoundEmitter::setSoundPath(std::string_view path) {
    if (!maInitialized_) {
        spdlog::error("Miniaudio engine not initialized, cannot set sound path");
        return;
    }

    if (soundInitialized_) {
        ma_sound_stop(&sound_);
        ma_sound_uninit(&sound_);
        soundInitialized_ = false;
        isPlaying_ = false;
    }

    soundPath_ = path;

    std::string pathStr(path);
    if (ma_sound_init_from_file(&engine_, pathStr.c_str(), MA_SOUND_FLAG_DECODE, NULL, NULL, &sound_) != MA_SUCCESS) {
        spdlog::error("Failed to load sound from path: {}", path);
        soundPath_.clear();
    } else {
        spdlog::debug("Sound loaded from path: {}", path);
        soundInitialized_ = true;
        ma_sound_set_looping(&sound_, isLooping_ ? MA_TRUE : MA_FALSE);
    }
}

void SoundEmitter::play() {
    if (!maInitialized_) {
        spdlog::error("Miniaudio engine not initialized, cannot play sound");
        return;
    }

    if (soundPath_.empty() || !soundInitialized_) {
        spdlog::warn("No sound path set, cannot play sound");
        return;
    }

	isPlaying_ = true;
    if (ma_sound_start(&sound_) != MA_SUCCESS) {
        spdlog::error("Failed to play sound: {}", soundPath_);
    }
}

void SoundEmitter::update() {
    if (soundInitialized_) {
        isPlaying_ = ma_sound_is_playing(&sound_);
    } else {
        isPlaying_ = false;
    }
}

void SoundEmitter::setLooping(bool loop) {
    isLooping_ = loop;
    if (soundInitialized_) {
        ma_sound_set_looping(&sound_, isLooping_ ? MA_TRUE : MA_FALSE);
    }
}
