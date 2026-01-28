#include "src/Application/Application.h"

int main() {
    Application app;
    
    if (!app.Initialize(3000, 2000, "Symulacja Dymu")) {
        return -1;
    }
    
    app.Run();
    app.Clean();
    
    return 0;
}

