#pragma once

#include "pch.h"

#include "model/base.h"

namespace Lunatic {
	class SoundEmitter : public Instance, public Updateable {
	public:
		explicit SoundEmitter(std::string_view name);
			virtual ~SoundEmitter() override;

		virtual std::string_view getClassName() const override {
			return "SoundEmitter";
		}

		void setSoundPath(std::string_view path);
		std::string_view getSoundPath() const { return soundPath_; }

		void setLooping(bool loop);
		bool isLooping() const { return isLooping_; }

		bool isPlaying() const { return isPlaying_; }

		void play();

		void update() override;

		RTTR_ENABLE(Instance);
		RTTR_REGISTRATION_FRIEND;
	private:
		static inline bool maInitialized_ = false;
		static inline ma_engine engine_;

		ma_sound sound_;
		bool soundInitialized_ = false;
		bool isLooping_ = false;
		bool isPlaying_ = false;

		std::string soundPath_;
	};
} // namespace Lunatic
