#ifndef KIKIRUNNER_UISYSTEM
#define KIKIRUNNER_UISYSTEM

#include <kiki.h>

#include "events/LevelLoadedEvent.hpp"
#include "events/RequestLevelChangeEvent.hpp"

enum class ScreenType {
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
	entt::entity spinner = entt::null;

	SplashScreen() = default;
	~SplashScreen() {
		World& world = World::Get();

		{
			std::lock_guard<std::mutex> lock(Kiki::SceneManager::get().registryMutex);
			world.DestroyEntity(background);
			world.DestroyEntity(engineLogo);
			world.DestroyEntity(gameLogo);
			world.DestroyEntity(spinner);
		}
	};
};

struct LoadingScreen : ScreenBase {
	entt::entity background = entt::null;
	entt::entity spinner = entt::null;

	LoadingScreen() = default;
	~LoadingScreen() {
		World& world = World::Get();

		{
			std::lock_guard<std::mutex> lock(Kiki::SceneManager::get().registryMutex);
			world.DestroyEntity(background);
			world.DestroyEntity(spinner);
		}
	};
};

struct MainMenuScreen : ScreenBase {
	entt::entity middleContainer = entt::null;
	entt::entity gameLogo = entt::null;
	entt::entity background = entt::null;
	entt::entity playButton = entt::null;
	entt::entity playButtonInner = entt::null;;
	entt::entity quitGameButton = entt::null;
	entt::entity quitGameButtonInner = entt::null;

	MainMenuScreen() = default;
	~MainMenuScreen() {
		World& world = World::Get();

		{
			std::lock_guard<std::mutex> lock(Kiki::SceneManager::get().registryMutex);
			world.DestroyEntity(quitGameButtonInner);
			world.DestroyEntity(quitGameButton);
			world.DestroyEntity(playButtonInner);
			world.DestroyEntity(playButton);
			world.DestroyEntity(background);
			world.DestroyEntity(gameLogo);
			world.DestroyEntity(middleContainer);
		}
	};
};

class UISystem : public System {
	public:
	Phase GetPhase() const override { return Phase::Input; }

	void OnStart() override {
		createSplashScreen();

		std::thread([this]() {
			fontManager.loadFont(std::filesystem::path(PROJECT_ASSETS_PATH) / "fonts/Chewy-Regular.ttf", "chewy-regular");

			MessageCenter::Publish(RequestLevelChangeEvent(std::filesystem::path(PROJECT_ASSETS_PATH) / "sponza.glb"));

			createMainMenu();
			initialised = true;
		}).detach();

		MessageCenter::Subscribe<AnimationEndEvent, &UISystem::OnAnimationEnd>(this);
		MessageCenter::Subscribe<LevelLoadedEvent, &UISystem::OnLevelLoaded>(this);
		MessageCenter::Subscribe<ButtonHoverEvent, &UISystem::OnButtonHover>(this);
		MessageCenter::Subscribe<ButtonClickEvent, &UISystem::OnButtonPress>(this);
	}

	void OnUpdate(float dt) override {
		if (nextReady && initialised) {
			nextReady = false;
			SplashScreen* screen = static_cast<SplashScreen*>(currentScreen);
			InterfaceAnimationComponent animComp;
			animComp.copy(screen->engineLogo);
			animComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
			animComp.time = 2.0f;
			animComp.delay = 1.0f;
			animComp.reverse = true;
			animComp.interpolation = InterfaceInterpolationType::EASE_IN;
			{
				std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
				registry.emplace<InterfaceAnimationComponent>(screen->engineLogo, animComp);
				registry.erase<InterfaceAnimationComponent>(screen->spinner);
			}

			InterfaceAnimationComponent spinnerComp;
			spinnerComp.copy(screen->spinner);
			spinnerComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 0.0f);
			spinnerComp.targetRotation += 180.0f;
			spinnerComp.time = 0.5f;
			spinnerComp.interpolation = InterfaceInterpolationType::EASE_OUT;
			{
				std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
				registry.emplace<InterfaceAnimationComponent>(screen->spinner, spinnerComp);
			}
		}
	}

	void OnAnimationEnd(const AnimationEndEvent& e) {
		switch (currentScreenType) {
		case (ScreenType::SPLASH):
			{
				SplashScreen* screen = static_cast<SplashScreen*>(currentScreen);
				if (e.entity == screen->engineLogo) {
					InterfaceAnimationComponent animComp;
					animComp.copy(screen->gameLogo);
					animComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
					animComp.time = 2.0f;
					animComp.delay = 1.0f;
					animComp.reverse = true;
					animComp.interpolation = InterfaceInterpolationType::EASE_IN;
					{
						std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
						registry.emplace<InterfaceAnimationComponent>(screen->gameLogo, animComp);
					}
				}
				else if (e.entity == screen->gameLogo) {
					InterfaceAnimationComponent animComp;
					animComp.copy(screen->background);
					animComp.targetBackgroundTransparency = 1.0f;
					animComp.time = 2.0f;
					animComp.interpolation = InterfaceInterpolationType::EASE_OUT;
					{
						std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
						registry.emplace<InterfaceAnimationComponent>(screen->background, animComp);
					}
				}
				else if (e.entity == screen->background) {
					delete currentScreen;
					currentScreen = std::move(nextScreen);
					currentScreenType = ScreenType::MAIN_MENU;
					nextScreen = new ScreenBase();
				}
			}
			break;
		case (ScreenType::MAIN_MENU):
			{
				if (nextScreenType == ScreenType::LOADING) {
					delete currentScreen;
					currentScreen = std::move(nextScreen);
					currentScreenType = ScreenType::LOADING;
					nextScreen = new ScreenBase();
					LoadingScreen* screen = static_cast<LoadingScreen*>(currentScreen);

					InterfaceAnimationComponent animComp;
					animComp.copy(screen->spinner);
					animComp.targetRotation += 360.0f;
					animComp.loop = true;
					animComp.time = 1.0f;
					{
						std::lock_guard<std::mutex> lock(sceneManager.registryMutex);
						registry.emplace<InterfaceAnimationComponent>(screen->spinner, animComp);
					}
				}
			}
			break;
		}
	}

	void OnLevelLoaded(const LevelLoadedEvent& e) {
		nextReady = true;
	}

	void OnButtonHover(const ButtonHoverEvent& e) {
		if (currentScreenType != ScreenType::SPLASH && currentScreenType != ScreenType::LOADING)
			Kiki::AudioSystem::PlayOneShot("interface/button.wav");
	}

	void OnButtonPress(const ButtonClickEvent& e) {
		if (e.clickType == ClickType::BUTTON_RELEASE) {
			if (currentScreenType == ScreenType::MAIN_MENU) {
				MainMenuScreen* screen = static_cast<MainMenuScreen*>(currentScreen);

				if (e.button == screen->playButton) {
					createLoadingScreen();
				} else if (e.button == screen->quitGameButton) {
					glfwSetWindowShouldClose(renderManager.getWindow(), true);
				}
			}
		}
	}

	private:
	World& world = World::Get();
	entt::registry& registry = world.Registry();
	TextureManager& textureManager = Kiki::TextureManager::get();
	FontManager& fontManager = Kiki::FontManager::get();
	SceneManager& sceneManager = Kiki::SceneManager::get();
	RenderManager& renderManager = Kiki::RenderManager::get();

	ScreenBase* currentScreen;
	ScreenBase* nextScreen;
	ScreenType currentScreenType;
	ScreenType nextScreenType;
	bool initialised = false;
	bool nextReady = false;

	void createSplashScreen() {
		currentScreen = new SplashScreen();
		currentScreenType = ScreenType::SPLASH;
		SplashScreen* screen = static_cast<SplashScreen*>(currentScreen);

		{
			std::lock_guard<std::mutex> lock(sceneManager.registryMutex);

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
			registry.emplace<InterfaceComponent>(screen->gameLogo, ScaleVec2D(0.25f, 0.0f, 0.0f, 0.0f), ScaleVec2D(0.5f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int) 61);
			registry.emplace<InterfaceTextureComponent>(screen->gameLogo, "GameLogoTransparent", glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
			registry.emplace<AspectRatioComponent>(screen->gameLogo, 1920.0f / 980.0f);

			textureManager.loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "interface/spinner.png", "Spinner");
			screen->spinner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->spinner, ScaleVec2D(0.5f, -25.0f, 1.0f, -200.0f), ScaleVec2D(0.0f, 50.0f, 0.0f, 50.0f), entt::null, (unsigned int) 61);
			registry.emplace<InterfaceTextureComponent>(screen->spinner, "Spinner", glm::vec4(1.0f, 1.0f, 1.0f, 0.6f));
			InterfaceAnimationComponent animComp;
			animComp.copy(screen->spinner);
			animComp.targetRotation = 360.0f;
			animComp.loop = true;
			animComp.time = 1.0f;
			registry.emplace<InterfaceAnimationComponent>(screen->spinner, animComp);
		}
	}

	void createLoadingScreen() {
		nextScreen = new LoadingScreen();
		nextScreenType = ScreenType::LOADING;
		LoadingScreen* screen = static_cast<LoadingScreen*>(nextScreen);

		{
			std::lock_guard<std::mutex> lock(sceneManager.registryMutex);

			screen->background = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->background, ScaleVec2D(0.0f, 0.0f, 0.0f, 0.0f), ScaleVec2D(1.0f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int)60);
			registry.emplace<BackgroundComponent>(screen->background, glm::vec3(0.0f, 0.0f, 0.0f), 1.0f);
			InterfaceAnimationComponent backgroundAnim;
			backgroundAnim.copy(screen->background);
			backgroundAnim.targetBackgroundTransparency = 0.0f;
			backgroundAnim.time = 2.0f;
			backgroundAnim.interpolation = InterfaceInterpolationType::EASE_IN;
			registry.emplace<InterfaceAnimationComponent>(screen->background, backgroundAnim);

			textureManager.loadTexture(std::filesystem::path(PROJECT_ASSETS_PATH) / "interface/spinner.png", "Spinner");
			screen->spinner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->spinner, ScaleVec2D(0.5f, -25.0f, 1.0f, -200.0f), ScaleVec2D(0.0f, 50.0f, 0.0f, 50.0f), entt::null, (unsigned int)61);
			registry.emplace<InterfaceTextureComponent>(screen->spinner, "Spinner", glm::vec4(1.0f, 1.0f, 1.0f, 0.0f));
			InterfaceAnimationComponent spinnerComp;
			spinnerComp.copy(screen->spinner);
			spinnerComp.targetTextureColour = glm::vec4(1.0f, 1.0f, 1.0f, 0.6f);
			spinnerComp.targetRotation += 720.0f;
			spinnerComp.time = 2.0f;
			registry.emplace<InterfaceAnimationComponent>(screen->spinner, spinnerComp);
		}
	}

	void createMainMenu() {
		nextScreen = new MainMenuScreen();
		nextScreenType = ScreenType::MAIN_MENU;
		MainMenuScreen* screen = static_cast<MainMenuScreen*>(nextScreen);

		{
			std::lock_guard<std::mutex> lock(sceneManager.registryMutex);

			screen->middleContainer = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->middleContainer, ScaleVec2D(0.3f, 0.0f, 0.0f, 0.0f), ScaleVec2D(0.4f, 0.0f, 1.0f, 0.0f), entt::null, (unsigned int) 40);

			screen->gameLogo = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->gameLogo, ScaleVec2D(-0.1f, 0.0f, 0.1f, 0.0f), ScaleVec2D(1.2f, 0.0f, 0.25f, 0.0f), screen->middleContainer, (unsigned int) 40);
			registry.emplace<InterfaceTextureComponent>(screen->gameLogo, "GameLogoTransparent", glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			registry.emplace<AspectRatioComponent>(screen->gameLogo, 1920.0f / 980.0f);
			InterfaceAnimationComponent logoBounce;
			logoBounce.copy(screen->gameLogo);
			logoBounce.targetPosition = ScaleVec2D(-0.1f, -25.0f, 0.1f, -25.0f);
			logoBounce.targetSize = ScaleVec2D(1.2f, 50.0f, 0.25f, 50.0f);
			logoBounce.time = 5.0f;
			logoBounce.reverse = true;
			logoBounce.loop = true;
			logoBounce.interpolation = InterfaceInterpolationType::EASE_IN_OUT;
			registry.emplace<InterfaceAnimationComponent>(screen->gameLogo, logoBounce);

			screen->background = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->background, ScaleVec2D(0.0f, 0.0f, 0.45f, 0.0f), ScaleVec2D(1.0f, 0.0f, 0.2f, 0.0f), screen->middleContainer, (unsigned int) 40);
			registry.emplace<BackgroundComponent>(screen->background, glm::vec3(0.1f, 0.1f, 0.1f), 0.3f, 40.0f);
			registry.emplace<AspectRatioComponent>(screen->background, 3.0f / 1.8f);

			screen->playButton = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->playButton, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 0.5f, -18.0f), screen->background, (unsigned int) 41);
			registry.emplace<BackgroundComponent>(screen->playButton, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 0.8f, 28.0f);
			registry.emplace<ButtonComponent>(screen->playButton, glm::vec4(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f, 0.2f), glm::vec4(29.0f / 255.0f, 100.0f / 255.0f, 171.0f / 255.0f, 0.2f), glm::vec4(12.0f / 255.0f, 40.0f / 255.0f, 69.0f / 255.0f, 0.2f));

			screen->playButtonInner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->playButtonInner, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 1.0f, -24.0f), screen->playButton, (unsigned int) 42);
			registry.emplace<BackgroundComponent>(screen->playButtonInner, glm::vec3(20.0f / 255.0f, 70.0f / 255.0f, 120.0f / 255.0f), 0.6f, 16.0f);
			registry.emplace<TextComponent>(screen->playButtonInner, "chewy-regular", U"Play Game", 0.6f, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);

			screen->quitGameButton = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->quitGameButton, ScaleVec2D(0.0f, 12.0f, 0.5f, 6.0f), ScaleVec2D(1.0f, -24.0f, 0.5f, -18.0f), screen->background, (unsigned int) 41);
			registry.emplace<BackgroundComponent>(screen->quitGameButton, glm::vec3(170.0f / 255.0f, 27.0f / 255.0f, 27.0f / 255.0f), 0.8f, 28.0f);
			registry.emplace<ButtonComponent>(screen->quitGameButton, glm::vec4(170.0f / 255.0f, 27.0f / 255.0f, 27.0f / 255.0f, 0.2f), glm::vec4(222.0f / 255.0f, 36.0f / 255.0f, 36.0f / 255.0f, 0.2f), glm::vec4(120.0f / 255.0f, 19.0f / 255.0f, 19.0f / 255.0f, 0.2f));

			screen->quitGameButtonInner = world.CreateEntity();
			registry.emplace<InterfaceComponent>(screen->quitGameButtonInner, ScaleVec2D(0.0f, 12.0f, 0.0f, 12.0f), ScaleVec2D(1.0f, -24.0f, 1.0f, -24.0f), screen->quitGameButton, (unsigned int)42);
			registry.emplace<BackgroundComponent>(screen->quitGameButtonInner, glm::vec3(170.0f / 255.0f, 27.0f / 255.0f, 27.0f / 255.0f), 0.4f, 16.0f);
			registry.emplace<TextComponent>(screen->quitGameButtonInner, "chewy-regular", U"Quit", 0.5f, glm::vec3(1.0f, 1.0f, 1.0f), 0.0f, Kiki::HorizontalAlignment::CENTRE, Kiki::VerticalAlignment::CENTRE, true);
		}
	}
};

#endif