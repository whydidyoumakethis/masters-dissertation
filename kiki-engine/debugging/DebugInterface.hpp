#ifndef KIKI_DEBUGGING_DEBUGINTERFACE
#define KIKI_DEBUGGING_DEBUGINTERFACE

#include "windows/EntityViewer.hpp"
#include "DebugCamera.hpp"

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
        bool enabled = false;
        bool debugCamAllowed = true;

        bool entityViewerVisible, logVisible;
        debug::EntityViewer& entityViewer = debug::EntityViewer::get();
        DebugCamera& debugCam = DebugCamera::get();
    };
}

#endif