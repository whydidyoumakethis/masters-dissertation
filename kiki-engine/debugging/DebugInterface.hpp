#ifndef KIKI_DEBUGGING_DEBUGINTERFACE
#define KIKI_DEBUGGING_DEBUGINTERFACE

#include "windows/EntityViewer.hpp"

namespace Kiki {
    class DebugInterface {
        public:
        static DebugInterface& get();

        void initialise();
        void update(float dt);
        void shutdown();

        private:
        DebugInterface() = default;
        ~DebugInterface() = default;
        DebugInterface(const DebugInterface&) = delete;
        DebugInterface& operator=(const DebugInterface&) = delete;

        bool initialised = false;

        bool entityViewerVisible;
        debug::EntityViewer& entityViewer = debug::EntityViewer::get();
    };
}

#endif