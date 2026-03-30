#ifndef KIKI_DEBUGGING_ENTITYVIEWER
#define KIKI_DEBUGGING_ENTITYVIEWER

namespace debug {
    class EntityViewer {
        public:
        static void draw(bool* visible);

        private:
        static float constexpr iconSize = 32.0f;
    };
}

#endif