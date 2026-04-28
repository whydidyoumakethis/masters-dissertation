#pragma once
#include <kiki.h>
class TimeLimitSystem : public System {
public:
    Phase GetPhase() const override { return Phase::PreUpdate; }
    void OnUpdate(float dt) override {
		//spdlog::info("Elapsed time: {:.2f} seconds", Timer::get().Elapsed());
        //auto& registry = World::Get().Registry();
        //auto timerView = registry.view<InterfaceComponent>();

        //for (auto [entity, interfaceComp] : timerView.each()) {
        //    auto elapsedTime = Timer::get().Elapsed();

        //    auto textView = registry.view<TextComponent, InterfaceComponent>();
        //    for (auto [textEntity, textComp, textInterface] : textView.each()) {
        //        if (textInterface.parent == entity) {
        //            std::u32string timeStr = formatTime(elapsedTime);

        //            if (textComp.text != timeStr) {
        //                textComp.text = timeStr;
        //                textComp.dirty = true;
        //            }
        //            break;
        //        }
        //    }
        //}
    };

	void OnStart() override {
		Timer::get().Reset();
		//createTimerUI();
	}
    //void createTimerUI() {
    //    auto& registry = World::Get().Registry();
    //    auto timerUIEntity = registry.create();


    //    InterfaceComponent containerInterface;
    //    containerInterface.position = { 10, 10, 0.0f, 0.0f, 10, 10 };
    //    containerInterface.size = { 150, 50, 0.0f, 0.0f, 150, 50 };
    //    containerInterface.zindex = 1000;
    //    containerInterface.dirty = true;
    //    containerInterface.parent = entt::null;
    //    registry.emplace<InterfaceComponent>(timerUIEntity, containerInterface);

    //    BackgroundComponent bgComp;
    //    bgComp.colour = glm::vec3(0.1f, 0.1f, 0.1f);
    //    bgComp.transparency = 0.3f;
    //    registry.emplace<BackgroundComponent>(timerUIEntity, bgComp);

    //    auto timerTextEntity = registry.create();

    //    InterfaceComponent textInterface;
    //    textInterface.position = { 0, 0, 0.0f, 0.0f, 0, 0 };
    //    textInterface.size = { 150, 50, 1.0f, 1.0f, 0, 0 };
    //    textInterface.zindex = 1001;
    //    textInterface.dirty = true;
    //    textInterface.parent = timerUIEntity;
    //    registry.emplace<InterfaceComponent>(timerTextEntity, textInterface);

    //    TextComponent textComp;
    //    textComp.text = U"00:00.00";
    //    textComp.font = "font_name";
    //    textComp.size = 24;
    //    textComp.colour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    //    textComp.horizontalAlignment = HorizontalAlignment::CENTRE;
    //    textComp.verticalAlignment = VerticalAlignment::CENTRE;
    //    textComp.dirty = true;
    //    registry.emplace<TextComponent>(timerTextEntity, textComp);

    //}
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
};