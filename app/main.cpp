#include "AppOptions.h"
#include "ViewerApp.h"

#include <iostream>

int main(int argc, char** argv) {
    const auto options = coral::app::parseOptions(argc, argv);
    if (!options.ok()) {
        std::cerr << options.error() << std::endl;
        return 1;
    }
    return coral::app::runViewerApp(options.value());
}