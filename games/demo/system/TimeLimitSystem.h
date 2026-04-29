#pragma once
#include <kiki.h>
class TimeLimitSystem : public System {
public:
    Phase GetPhase() const override { return Phase::PreUpdate; }
    void OnUpdate(float dt) override {
		//spdlog::info("Elapsed time: {:.2f} seconds", Timer::get().Elapsed());
        auto& registry = World::Get().Registry();
        auto timerView = registry.view<InterfaceComponent>();

        for (auto [entity, interfaceComp] : timerView.each()) {
            auto elapsedTime = Timer::get().Elapsed();

            auto textView = registry.view<TextComponent>();
            for (auto [textEntity, textComp] : textView.each()) {
                    std::u32string timeStr = formatTime(elapsedTime);

                    if (textComp.text != timeStr) {
                        textComp.text = timeStr;
                        textComp.dirty = true;
                    }
                
            }
        }
    };

	void OnStart() override {
		Timer::get().Reset();
		createTimerUI();
	}
    void createTimerUI() {
        auto& registry = World::Get().Registry();
        auto timerUIEntity = registry.create();

        registry.emplace<InterfaceComponent>(timerUIEntity, ScaleVec2D(0, 10, 0, 10), ScaleVec2D(0, 150, 0, 50));
        registry.emplace<BackgroundComponent>(timerUIEntity, glm::vec3(0.1f, 0.1f, 0.1f), 0.3f);
        registry.emplace<TextComponent>(timerUIEntity, "font", U"00:00.00", 24, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f);

    }
    std::u32string formatTime(float seconds) {
        int minutes = static_cast<int>(seconds) / 60;
        int secs = static_cast<int>(seconds) % 60;
        int hundredths = static_cast<int>((seconds - static_cast<int>(seconds)) * 100);

        // MM:SS.CC
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%02d:%02d.%02d", minutes, secs, hundredths);

        // UTF-32
        std::string str(buffer);
        std::u32string result;
        for (char c : str) {
            result.push_back(static_cast<char32_t>(c));
        }

        return result;
    }
private:
    // temp solution
    std::vector<float> timeLimits = { 90.f,60.f,50.f,20.f };
	std::vector<std::vector<unsigned int>> timeLimitPowers = { {0,1,2}, {0,2,3} };
};