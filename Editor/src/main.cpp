#include <Application.hpp>

// If it's a Distribution build, tell the linker to use the Windows subsystem instead of Console
#ifdef WTHR_DIST
#pragma comment(linker, "/SUBSYSTEM:windows /ENTRY:mainCRTStartup")
#endif

int main(int argc, char** argv)
{
    Application app;
    if (!app.Init())    
        return -1;

    app.Run();
    return 0;
}