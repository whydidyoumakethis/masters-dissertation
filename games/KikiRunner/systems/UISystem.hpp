#ifndef KIKIRUNNER_UISYSTEM
#define KIKIRUNNER_UISYSTEM

#include <kiki.h>

enum ScreenType {
	SPLASH,
	LOADING,
	MAIN_MENU,
	LEVEL
};

struct ScreenBase {
	virtual ~ScreenBase() {};
};

struct SplashScreen : ScreenBase {
	entt::entity background = entt::null;
	entt::entity engineLogo = entt::null;
	entt::entity gameLogo = entt::null;

	SplashScreen() = default;
	~SplashScreen() {
		World& world = World::Get();

		world.DestroyEntity(background);
		world.DestroyEntity(engineLogo);
		world.DestroyEntity(gameLogo);
	};
};

class UISystem : public System {
	public:
	Phase GetPhase() const override { return Phase::Input; }

	void OnStart() override {
		createSplashScreen();

		std::thread([this]() {
			fontManager.loadFont(std::filesystem::path(PROJECT_ASSETS_PATH) / "fonts/DynaPuff-Bold.ttf", "dynapuff-bold");
			fontManager.loadFont(std::filesystem::path(PROJECT_ASSETS_PATH) / "fonts/DynaPuff-Regular.ttf", "dynapuff-regular");

			createMainMenu();
			initialised = true;
		}).detach();

		MessageCenter::Subscribe<AnimationEndEvent, &UISystem::OnTriggerEnter>(this);
	}

	void OnUpdate(float dt) override {
		wait -= dt;

		if (wait < 0.0f) {
			nextReady = true;
		}

		if (nextReady && !changingScreen && initialised) {
			changingScreen = true;
			SplashScreen* screen = static_cast<SplashScreen*>(currentScreen);
			InterfaceAnimationComponent animComp;
			animComp.targetPosition = ScaleVec2D(0.25f, 0.0f, 0.0f, 0.0f);
			animComp.targetSize = ScaleVec2D(0.5f, 0.0f, 1.0f, 0.0f);
			animComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			animComp.time = 2.0f;
			animComp.delay = 1.0f;
			animComp.reverse = true;
			animComp.interpolation = InterfaceInterpolationType::EASE_IN;
			registry.emplace<InterfaceAnimationComponent>(screen->engineLogo, animComp);
		}
	}

	void OnTriggerEnter(const AnimationEndEvent& e) {
		switch (currentScreenType) {
		case (ScreenType::SPLASH):
			SplashScreen* screen = static_cast<SplashScreen*>(currentScreen);
			if (e.entity == screen->engineLogo) {
				InterfaceAnimationComponent animComp;
				animComp.targetPosition = ScaleVec2D(0.25f, 0.0f, 0.0f, 0.0f);
				animComp.targetSize = ScaleVec2D(0.5f, 0.0f, 1.0f, 0.0f);
				animComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
				animComp.time = 2.0f;
				animComp.delay = 1.0f;
				animComp.reverse = true;
				animComp.interpolation = InterfaceInterpolationType::EASE_IN;
				registry.emplace<InterfaceAnimationComponent>(screen->gameLogo, animComp);
			} else if (e.entity == screen->gameLogo) {
				InterfaceAnimationComponent animComp;
				animComp.targetPosition = ScaleVec2D(0.0f, 0.0f, 0.0f, 0.0f);
				animComp.targetSize = ScaleVec2D(1.0f, 0.0f, 1.0f, 0.0f);
				animComp.targetBackgroundColour = glm::vec3(0.0f, 0.0f, 0.0f);
				animComp.targetBackgroundTransparency = 1.0f;
				animComp.time = 2.0f;
				animComp.interpolation = InterfaceInterpolationType::EASE_OUT;
				registry.emplace<InterfaceAnimationComponent>(screen->background, animComp);
			} else if (e.entity == screen->background) {
				delete currentScreen;
			}
		}
	}

	private:
	World& world = World::Get();
	entt::registry& registry = world.Registry();
	TextureManager& textureManager = Kiki::TextureManager::get();
	FontManager& fontManager = Kiki::FontManager::get();

	ScreenBase* currentScreen;
	ScreenBase* nextScreen;
	ScreenType currentScreenType;
	ScreenType nextScreenType;
	bool initialised = false;
	bool nextReady = false;
	bool changingScreen = false;

	float wait = 10.0f;

	void createSplashScreen() {
		currentScreen = new SplashScreen();
		currentScreenType = ScreenType::SPLASH;
		SplashScreen* screen = static_cast<SplashScreen*>(currentScreen);

		screen->background = world.CreateEntity();
		registry.emplace<InterfaceComponent>(screen->background, ScaleVec2D(0.0f, 0.0f, 0.0f, 0.0f), ScaleVec2D(1.0f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int) 60);
		registry.emplace<BackgroundComponent>(screen->background, glm::vec3(0.0f, 0.0f, 0.0f), 0.0f);

		textureManager.loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "interface/kiki_logo_transparent.png", "EngineLogoTransparent");
		screen->engineLogo = world.CreateEntity();
		registry.emplace<InterfaceComponent>(screen->engineLogo, ScaleVec2D(0.25f, 0.0f, 0.0f, 0.0f), ScaleVec2D(0.5f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int) 61);
		registry.emplace<InterfaceTextureComponent>(screen->engineLogo, "EngineLogoTransparent", glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
		registry.emplace<AspectRatioComponent>(screen->engineLogo, 1920.0f / 980.0f);
		
		textureManager.loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "interface/game_logo_transparent.png", "GameLogoTransparent");
		screen->gameLogo = world.CreateEntity();
		registry.emplace<InterfaceComponent>(screen->gameLogo, ScaleVec2D(0.25f, 0.0f, 0.0f, 0.0f), ScaleVec2D(0.5f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int)61);
		registry.emplace<InterfaceTextureComponent>(screen->gameLogo, "GameLogoTransparent", glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
		registry.emplace<AspectRatioComponent>(screen->gameLogo, 1920.0f / 980.0f);
	}

	void createLoadingScreen() {

	}

	void createMainMenu() {

	}
};

#endif