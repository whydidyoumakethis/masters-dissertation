#pragma once
#include <kiki.h>
#include"../Events.h"
class TimeLimitSystem : public System {
public:
    Phase GetPhase() const override { return Phase::PreUpdate; }
    void OnUpdate(float dt) override {
		//spdlog::info("Elapsed time: {:.2f} seconds", Timer::get().Elapsed());
        auto& registry = World::Get().Registry();
        auto timerView = registry.view<InterfaceComponent>();
        auto elapsedTime = Timer::get().Elapsed();
        if (timerTextEntity != entt::null && registry.valid(timerTextEntity)
            && registry.all_of<TextComponent>(timerTextEntity)) {

            auto& textComp = registry.get<TextComponent>(timerTextEntity);
            std::u32string timeStr = formatTime(elapsedTime);
            if (textComp.text != timeStr) {
                textComp.text = timeStr;
                textComp.dirty = true;
            }
        }
    };

	void OnStart() override {
		Timer::get().Reset();
        auto objects = World::Get().Query<CharacterComponent>();

        for (auto [entity, chara] : objects.each()) {
			timeLimits = chara.timeLimits;
            labels = chara.labels;
			break; // just take the first one we find, assuming there's only one player character
        }
		createTimerUI();
        createTimeLimitPanel();
		MessageCenter::Subscribe<TimerTriggerEvent, &TimeLimitSystem::OnTimeLimitReached>(this);
	}
    void createTimerUI() {
        auto& registry = World::Get().Registry();
        auto timerUIEntity = registry.create();
		timerTextEntity = timerUIEntity;

        registry.emplace<InterfaceComponent>(timerUIEntity, ScaleVec2D(0, 10, 0, 10), ScaleVec2D(0, 150, 0, 50));
        registry.emplace<BackgroundComponent>(timerUIEntity, glm::vec3(0.1f, 0.1f, 0.1f), 0.3f);
        registry.emplace<TextComponent>(timerUIEntity, "font", U"00:00.00", 24, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f);

        InterfaceAnimationComponent animComp;
        animComp.targetPosition = ScaleVec2D(0, 10, 0, 10);
        animComp.targetSize = ScaleVec2D(0, 150, 0, 50);
        animComp.targetBackgroundColour = glm::vec3(0.1f, 0.1f, 0.1f);
        animComp.targetBackgroundTransparency = 0.3f;
        animComp.targetTextColour = glm::vec3(1.0f, 1.0f, 1.0f);
        animComp.targetTextTransparency = 0.0f;
        animComp.targetTextSize = 32;
        animComp.time = 0.5;
        animComp.loop = true;
        animComp.reverse = true;
        animComp.interpolation = InterfaceInterpolationType::EASE_IN_OUT;

        registry.emplace<InterfaceAnimationComponent>(timerUIEntity, animComp);

    }
    void createTimeLimitPanel() {
        auto& registry = World::Get().Registry();


		// top right panel with time limits and ability labels
        auto panelBg = registry.create();
        registry.emplace<InterfaceComponent>(panelBg,
            ScaleVec2D(1.0f, -210, 0.0f, 10),
            ScaleVec2D(0.0f, 200, 0.0f, 30 + (int)timeLimits.size() * 30));
        registry.emplace<BackgroundComponent>(panelBg,
            glm::vec3(0.15f, 0.18f, 0.22f), 0.45f);

        auto titleEntity = registry.create();
        registry.emplace<InterfaceComponent>(titleEntity,
            ScaleVec2D(1.0f, -208, 0.0f, 14),
            ScaleVec2D(0.0f, 196, 0.0f, 24));
        registry.emplace<TextComponent>(titleEntity,
            "font", U"time limit    ability", 13,
            glm::vec3(0.75f, 0.85f, 0.95f), 0.0f,
            Kiki::HorizontalAlignment::LEFT,
            Kiki::VerticalAlignment::CENTRE);

        for (int i = 0; i < (int)timeLimits.size() && i < (int)labels.size(); i++) {
            auto rowEntity = registry.create();
			rows.push_back(rowEntity);
			isDone.push_back(false);
			float yPos = 42.0f + i * 30.0f;  // 30f per row

            registry.emplace<InterfaceComponent>(rowEntity,
                ScaleVec2D(1.0f, -204, 0.0f, yPos),
                ScaleVec2D(0.0f, 192, 0.0f, 26));
            registry.emplace<TextComponent>(rowEntity,
                "font",
                formatTimeLimitText(timeLimits[i], labels[i]),
                14,
                glm::vec3(0.88f, 0.88f, 0.88f), 0.0f,
                Kiki::HorizontalAlignment::LEFT,
                Kiki::VerticalAlignment::CENTRE);


        }
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
    std::u32string formatTimeLimitText(float seconds, const std::u32string& label) {
        int mins = static_cast<int>(seconds) / 60;
        int secs = static_cast<int>(seconds) % 60;
        char buf[12];
        snprintf(buf, sizeof(buf), "%d:%02d  ", mins, secs);

        std::u32string result;
        for (char c : std::string(buf)) result.push_back(static_cast<char32_t>(c));
        result += U"         " + label;
        return result;
    }
    void OnTimeLimitReached(TimerTriggerEvent e) {
        auto& registry = World::Get().Registry();

        for (size_t i = 0; i < rows.size(); ++i) {
            auto rowEntity = rows[i];
            if (rowEntity == entt::null || !registry.valid(rowEntity)) continue;


            if (e.elapsedTime   > timeLimits[i] || isDone[i]) continue;

            if (rows[i] != entt::null && registry.valid(rows[i])
                && registry.all_of<TextComponent>(rows[i])) {

                auto& textComp = registry.get<TextComponent>(rows[i]);
                textComp.colour = glm::vec3(0.35f, 0.65f, 0.35f);
                textComp.dirty = true;
                isDone[i] = true;
                //std::u32string completed = textComp.text + U"✓";
                //textComp.text = completed;
                if (registry.all_of<InterfaceComponent>(rowEntity)) {
                    auto& rowIC = registry.get<InterfaceComponent>(rowEntity);

                    auto strikeEntity = registry.create();

                    ScaleVec2D strikePos = rowIC.position;
                    strikePos.y += 11.0f;

                    registry.emplace<InterfaceComponent>(strikeEntity,
                        strikePos,
                        ScaleVec2D(rowIC.size.scaleX, rowIC.size.x, 0.0f, 2.0f)
                    );
                    registry.emplace<BackgroundComponent>(strikeEntity,
                        glm::vec3(0.5f, 0.5f, 0.5f), 0.0f
                    );
                }

                
            }

            break;
        }
    }
private:
    entt::entity  timerTextEntity = entt::null;
	std::vector<Entity> rows = {};
	std::vector<bool> isDone = {};
    // temp solution
    std::vector<float> timeLimits = { };
    std::vector<std::u32string> labels = {};
	//std::vector<std::vector<unsigned int>> timeLimitPowers = { {0,1,2}, {0,2,3} };
};