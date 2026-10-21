#include "sf33rd/Source/Game/main.h"

#if defined(TARGET_PS2)
void main() {
    AcrMain();
}
#else
#if defined(TARGET_WEB)
#include "port/web/web_trace.h"
#endif

int main() {
#if defined(TARGET_WEB)
    web_trace("main entry");
#endif
    AcrMain();
#if defined(TARGET_WEB)
    web_trace("main exit");
#endif
    return 0;
}
#endif
