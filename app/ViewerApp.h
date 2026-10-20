#pragma once
#include "AppOptions.h"

namespace coral::app {

// Composition root: wires device, protocol, imaging, filters and UI; runs the UI loop.
int runViewerApp(const AppOptions& options);

}  // namespace coral::app