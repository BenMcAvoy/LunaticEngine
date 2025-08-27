#pragma once

#include "model/base.h"
#include "render/vertex.h"
#include "render/texture.h"

#include "core/utils.h"

namespace Lunatic {
    class Sprite : public Instance, public Renderable {
    public:
        explicit Sprite(std::string_view name);
        std::string_view getClassName() const override {
			return "Sprite";
        }

		bool setTexture(std::string_view path);
		void clearTexture() { texture_ = nullptr; }

        void draw(Shader& shader) override;

    protected:
        void reflect() override;

    private:
        // All data below is static and shared across all instances of Sprite
        static inline bool dataInitialized_ = false;
        static inline GLuint vao_ = 0;
        static inline GLuint vbo_ = 0;
        static inline GLuint ebo_ = 0;
		static inline Utils::HeteroStringMap<std::shared_ptr<Texture>> textures_;

		std::string currentTexturePath_;
		std::shared_ptr<Texture> texture_ = nullptr;
    };
} // namespace Lunatic
