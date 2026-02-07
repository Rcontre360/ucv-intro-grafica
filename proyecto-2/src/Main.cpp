#include "state/C3DViewer.h"
#include <iostream>

int main() 
{
    C3DViewer app;
    if (!app.setup()) 
    {
        fprintf(stderr, "Failed to setup C3DViewer\n");
        return -1;
    }

    app.mainLoop();

    return 0;
}
