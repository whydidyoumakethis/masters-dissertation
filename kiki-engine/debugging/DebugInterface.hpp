#ifndef KIKI_DEBUGGING_DEBUGINTERFACE
#define KIKI_DEBUGGING_DEBUGINTERFACE

#include "windows/EntityViewer.hpp"
#include "DebugCamera.hpp"
#include "renderer/RenderManager.hpp"

namespace Kiki {
    class DebugInterface {
        public:
        static DebugInterface& get();

        void initialise();
        void draw();
        void shutdown();

        private:
        DebugInterface() = default;
        ~DebugInterface() = default;
        DebugInterface(const DebugInterface&) = delete;
        DebugInterface& operator=(const DebugInterface&) = delete;

        bool initialised = false;
        bool enabled = false;
        bool debugCamAllowed = true;

        bool entityViewerVisible;
        debug::EntityViewer& entityViewer = debug::EntityViewer::get();
        DebugCamera& debugCam = DebugCamera::get();

        RenderManager& renderManager = RenderManager::get();
    };
}

#endif