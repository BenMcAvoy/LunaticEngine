#pragma once

#include "model/base.h"
#include "render/vertex.h"

namespace Lunatic {
    class Sprite : public Instance, public Renderable {
    public:
        explicit Sprite(const std::string_view name);
        std::string getClassName() const override;
        void draw() override;

        int getVAO() {
            if (!dataInitialized_)
                throw std::runtime_error("Sprite static data not initialized");
			return static_cast<int>(vao_);
		}

    private:
        // All data below is static and shared across all instances of Sprite
        static inline bool dataInitialized_ = false;
        static inline GLuint vao_ = 0;
        static inline GLuint vbo_ = 0;
        static inline GLuint ebo_ = 0;
    };
} // namespace Lunatic
