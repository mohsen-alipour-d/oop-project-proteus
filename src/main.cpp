#include "ui/ProteusApplication.h"

int main(int argc, char* argv[])
{
    (void)argc;
    (void)argv;

    // =====================================================================
    // SECTIONS 1 TO 4 - FRONTEND
    // Main menu, project canvas, component library and component editing are
    // initialized by ProteusApplication.
    // =====================================================================
    ProteusApplication application;
    if (!application.initialize())
    {
        return 1;
    }

    // =====================================================================
    // SECTIONS 5, 6, 7, 9, 10 AND 11 - BACKEND
    // ProteusApplication owns one BackendAdapter. The adapter connects the UI
    // to Circuit, wiring/nets, advanced components and MCU, measurements,
    // Save/Load + Undo/Redo, and DRC. There is only one application entry
    // point, so merging branches cannot create a second-main conflict.
    // =====================================================================

    // =====================================================================
    // INTEGRATED EVENT AND SIMULATION LOOP
    // UI actions are forwarded to the backend and every accepted edit is
    // recorded as one history snapshot.
    // =====================================================================
    application.run();
    return 0;
}
