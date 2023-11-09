#include <RootTask.h>

#include <rio.h>

int main()
{
    // Initialize RIO with root task
    if (!rio::Initialize<RootTask>())
        return -1;

    // Main loop
    rio::EnterMainLoop();

    // Exit RIO
    rio::Exit();

    return 0;
}
