#pragma once

struct CameraPathPointComponent {
    int pathIndex = -1;
    float durationToNext = 3.0f;

    CameraPathPointComponent() = default;
    explicit CameraPathPointComponent(int index, float duration = 3.0f)
        : pathIndex(index), durationToNext(duration) {}
};
