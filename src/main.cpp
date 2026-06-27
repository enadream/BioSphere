#include "core/application.hpp"
#include "game/game_layer.hpp"

int main() {
    Application app(1280, 720, "BioSphere");
    app.PushLayer(new GameLayer());
    app.Run();
    return 0;
}
