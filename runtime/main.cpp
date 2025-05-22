#include "engine.h"

int main(void) {
    Lunatic::Engine engine(1280, 720, "Lunatic Runtime");
    LUNA_ASSERT(&engine.GetInstance() == &engine);

    engine.run();

    return 0;
}
