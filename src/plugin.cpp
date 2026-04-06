#include "log.h"
#include <future>
#include <SimpleIni.h>

static std::vector<RE::TESObjectARMO*> g_armors;
static std::atomic<bool> g_dialogueActive{ false };
static float g_chanceToActivate = 20.0f;
static bool g_onlyFemale = true;
static bool g_unequipSlotOnAhegaoStart = true;

class DialogueEventSink :
	public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
	static DialogueEventSink* GetSingleton()
	{
		static DialogueEventSink instance;
		return &instance;
	}

	RE::TESObjectARMO* currentArmor = nullptr;
	RE::TESObjectARMO* lastArmorUnequipped = nullptr;

	void AhegaoWhenNPCNotTalking(RE::ActorHandle npcHandle)
	{
		if (!g_armors.empty())
		{
			std::random_device rd;
			std::mt19937 gen(rd());
			std::uniform_int_distribution<size_t> dist(0, g_armors.size() - 1);
			currentArmor = g_armors[dist(gen)];
		}

		std::thread([this, npcHandle]() {
			// Initial delay of 1 second before starting to poll
			std::this_thread::sleep_for(std::chrono::seconds(1));

			std::mt19937 gen(std::random_device{}());
			std::uniform_real_distribution<float> dist(0.0f, 100.0f);

			// Up to 60 seconds max
			constexpr int kMaxAttempts = 600;
			bool lastIsTalking = false;
			bool firstPoll = true;

			for (int i = 0; i < kMaxAttempts; ++i) {
				if (!g_dialogueActive.load(std::memory_order_relaxed)) {
					break;
				}

				auto promise = std::make_shared<std::promise<bool>>();
				auto future = promise->get_future();

				auto* taskInterface = SKSE::GetTaskInterface();
				if (!taskInterface) break;

				// Read if talking to player safely on the game thread
				taskInterface->AddTask([npcHandle, promise]() { //&promise
					auto* npc = npcHandle.get().get();
					if (!npc) {
						promise->set_value(false);
						return;
					}

					bool isTalking = false;

					auto& runtimeData = npc->GetActorRuntimeData();
					if (runtimeData.voiceTimer > 0.0f) {
						isTalking = true;
					}
					//SKSE::log::info("isTalking: {}", isTalking ? "true" : "false");

					promise->set_value(isTalking);
					});

				if (future.wait_for(std::chrono::seconds(5)) == std::future_status::timeout) {
					break; // bail out
				}
				const bool isTalking = future.get();

				// Only act if state changed (or first poll)
				if (firstPoll || isTalking != lastIsTalking) {
					if (isTalking) {
						// NPC started talking -> hide ahegao
						taskInterface->AddTask([this, npcHandle]() {
							auto* npc = npcHandle.get().get();
							if (npc) EndAhegao(npc, false);
							});
					}
					else {
						const float roll = dist(gen);
						// NPC stopped talking -> show ahegao if chance allows
						taskInterface->AddTask([this, npcHandle, roll]() {
							auto* npc = npcHandle.get().get();
							if (npc && roll < g_chanceToActivate)
							{
								StartAhegao(npc);
							}
							});
					}
					lastIsTalking = isTalking;
					firstPoll = false;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(500));
			}

			}).detach();
	}

	void StartAhegao(RE::Actor* npc)
	{
		if (!npc) return;

		if (!currentArmor) return;

		auto* equipManager = RE::ActorEquipManager::GetSingleton();
		if (!equipManager) return;

		auto* equipSlot = currentArmor->GetEquipSlot();
		auto armorSlot = static_cast<RE::BGSBipedObjectForm::BipedObjectSlot>(currentArmor->GetSlotMask());
		auto* currentlyWorn = npc->GetWornArmor(armorSlot);

		if(currentlyWorn)
		{
			if (g_unequipSlotOnAhegaoStart)
			{
				lastArmorUnequipped = currentlyWorn;
				equipManager->UnequipObject(npc, currentlyWorn, nullptr, 1, equipSlot, false, true, false, true, nullptr);
			}
			else
			{
				return;
			}
		}

		StartAhegaoEquipTongue(npc, equipManager, equipSlot);
		StartAhegaoMfg(npc);
	}

	void StartAhegaoEquipTongue(RE::Actor* npc, auto* equipManager, auto* equipSlot)
	{
		npc->AddObjectToContainer(currentArmor, nullptr, 1, nullptr);
		equipManager->EquipObject(npc, currentArmor, nullptr, 1, equipSlot,
			false, true, false, true);
	}

	void StartAhegaoMfg(RE::Actor* npc)
	{
		using MfgFn = void (DialogueEventSink::*)(RE::Actor*);
		static const MfgFn variants[] = {
			&DialogueEventSink::StartAhegaoMfg1,
			&DialogueEventSink::StartAhegaoMfg2,
			&DialogueEventSink::StartAhegaoMfg3,
			&DialogueEventSink::StartAhegaoMfg4,
			&DialogueEventSink::StartAhegaoMfg5,
			&DialogueEventSink::StartAhegaoMfg6,
		};

		std::mt19937 gen(std::random_device{}());
		std::uniform_int_distribution<size_t> dist(0, std::size(variants) - 1);
		(this->*variants[dist(gen)])(npc);
	}

	void StartAhegaoMfg1(RE::Actor* npc)
	{
		ExecuteConsoleCommand("mfg expression 1 100", npc);
		ExecuteConsoleCommand("mfg phoneme 12 100", npc);
		ExecuteConsoleCommand("mfg phoneme 11 100", npc);

		ExecuteConsoleCommand("mfg modifier 11 90", npc);
		ExecuteConsoleCommand("mfg modifier 0 40", npc);
		ExecuteConsoleCommand("mfg modifier 1 50", npc);
	}

	void StartAhegaoMfg2(RE::Actor* npc)
	{
		ExecuteConsoleCommand("mfg expression 1 100", npc);
		ExecuteConsoleCommand("mfg phoneme 1 100", npc);
		ExecuteConsoleCommand("mfg phoneme 3 100", npc);

		ExecuteConsoleCommand("mfg modifier 6 100", npc);
		ExecuteConsoleCommand("mfg modifier 7 100 ", npc);
		ExecuteConsoleCommand("mfg modifier 11 100", npc);
	}

	void StartAhegaoMfg3(RE::Actor* npc)
	{
		ExecuteConsoleCommand("mfg expression 6 100", npc);
		ExecuteConsoleCommand("mfg phoneme 0 100", npc);
		ExecuteConsoleCommand("mfg phoneme 6 100", npc);

		ExecuteConsoleCommand("mfg modifier 6 100", npc);
		ExecuteConsoleCommand("mfg modifier 7 100 ", npc);
		ExecuteConsoleCommand("mfg modifier 11 100", npc);
	}

	void StartAhegaoMfg4(RE::Actor* npc)
	{
		ExecuteConsoleCommand("mfg expression 8 100", npc);
		ExecuteConsoleCommand("mfg phoneme 11 100", npc);
		ExecuteConsoleCommand("mfg phoneme 10 100", npc);

		ExecuteConsoleCommand("mfg modifier 6 100", npc);
		ExecuteConsoleCommand("mfg modifier 7 100 ", npc);
		ExecuteConsoleCommand("mfg modifier 11 100", npc);
	}

	void StartAhegaoMfg5(RE::Actor* npc)
	{
		ExecuteConsoleCommand("mfg expression 9 100", npc);
		ExecuteConsoleCommand("mfg phoneme 12 100", npc);
		ExecuteConsoleCommand("mfg phoneme 13 100", npc);

		ExecuteConsoleCommand("mfg modifier 6 100", npc);
		ExecuteConsoleCommand("mfg modifier 7 100 ", npc);
		ExecuteConsoleCommand("mfg modifier 11 100", npc);
	}

	void StartAhegaoMfg6(RE::Actor* npc)
	{
		ExecuteConsoleCommand("mfg expression 16 100", npc);
		ExecuteConsoleCommand("mfg phoneme 5 100", npc);
		ExecuteConsoleCommand("mfg phoneme 14 100", npc);

		ExecuteConsoleCommand("mfg modifier 6 100", npc);
		ExecuteConsoleCommand("mfg modifier 7 100 ", npc);
		ExecuteConsoleCommand("mfg modifier 11 100", npc);
	}

	void EndAhegao(RE::Actor* npc, bool clearGlobalTongue)
	{
		if (!npc) return;

		EndAhegaoUnequipTongue(npc, clearGlobalTongue);
		EndAhegaoMfg(npc);
	}

	void EndAhegaoUnequipTongue(RE::Actor* npc, bool clearGlobalTongue)
	{
		if (currentArmor)
		{
			auto* equipManager = RE::ActorEquipManager::GetSingleton();
			if (equipManager) {
				equipManager->UnequipObject(npc, currentArmor, nullptr, 1, nullptr,
					true, true, true, true);
			}
			npc->RemoveItem(currentArmor, 1, RE::ITEM_REMOVE_REASON::kRemove, nullptr, nullptr);

			if (lastArmorUnequipped && equipManager)
			{
				if (std::find(g_armors.begin(), g_armors.end(), lastArmorUnequipped) == g_armors.end())
				{
					auto* equipSlot = currentArmor->GetEquipSlot();
					equipManager->EquipObject(npc, lastArmorUnequipped, nullptr, 1, equipSlot,
						false, true, false, true);
				}

				lastArmorUnequipped = nullptr;
			}

			if (clearGlobalTongue)
			{
				currentArmor = nullptr;
			}
		}
	}

	void EndAhegaoMfg(RE::Actor* npc)
	{
		ExecuteConsoleCommand("mfg expression 1 0", npc);
		ExecuteConsoleCommand("mfg expression 6 0", npc);
		ExecuteConsoleCommand("mfg expression 8 0", npc);
		ExecuteConsoleCommand("mfg expression 9 0", npc);
		ExecuteConsoleCommand("mfg expression 16 0", npc);

		ExecuteConsoleCommand("mfg phoneme 0 0", npc);
		ExecuteConsoleCommand("mfg phoneme 1 0", npc);
		ExecuteConsoleCommand("mfg phoneme 3 0", npc);
		ExecuteConsoleCommand("mfg phoneme 5 0", npc);
		ExecuteConsoleCommand("mfg phoneme 6 0", npc);
		ExecuteConsoleCommand("mfg phoneme 10 0", npc);
		ExecuteConsoleCommand("mfg phoneme 11 0", npc);
		ExecuteConsoleCommand("mfg phoneme 12 0", npc);
		ExecuteConsoleCommand("mfg phoneme 13 0", npc);
		ExecuteConsoleCommand("mfg phoneme 14 0", npc);

		ExecuteConsoleCommand("mfg modifier 0 0", npc);
		ExecuteConsoleCommand("mfg modifier 1 0", npc);
		ExecuteConsoleCommand("mfg modifier 6 0", npc);
		ExecuteConsoleCommand("mfg modifier 7 0", npc);
		ExecuteConsoleCommand("mfg modifier 11 0", npc);
	}

	void ExecuteConsoleCommand(const std::string& command, RE::TESObjectREFR* targetRef = nullptr)
	{
		const auto scriptFactory = RE::IFormFactory::GetConcreteFormFactoryByType<RE::Script>();
		const auto script = scriptFactory ? scriptFactory->Create() : nullptr;
		if (script) {
			script->SetCommand(command);
			script->CompileAndRun(targetRef);
			delete script;
		}
	}

	// ---------------------------------------------------------------
	// Event handler
	// ---------------------------------------------------------------
	RE::BSEventNotifyControl ProcessEvent(
		const RE::MenuOpenCloseEvent* a_event,
		RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override
	{
		if (!a_event || a_event->menuName != RE::DialogueMenu::MENU_NAME)
		{
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto topicManager = RE::MenuTopicManager::GetSingleton();
		if (!topicManager)
		{
			return RE::BSEventNotifyControl::kContinue;
		}

		const auto speaker = topicManager->speaker.get();
		if (!speaker)
		{
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* npc = speaker->As<RE::Actor>();
		if (!npc)
		{
			return RE::BSEventNotifyControl::kContinue;
		}

		auto* base = npc->GetActorBase();
		if (!base || (g_onlyFemale && base->GetSex() != RE::SEX::kFemale) || npc->IsChild()) {
			return RE::BSEventNotifyControl::kContinue;
		}

		if (a_event->opening)
		{
			g_dialogueActive.store(true, std::memory_order_relaxed);
			AhegaoWhenNPCNotTalking(npc->GetHandle());
		}
		else
		{
			g_dialogueActive.store(false, std::memory_order_relaxed);
			EndAhegao(npc, true);
		}

		return RE::BSEventNotifyControl::kContinue;
	}
};

void LoadSettings()
{
	CSimpleIniA ini;
	ini.SetUnicode();

	const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
	const std::string path = std::string("Data/SKSE/Plugins/") + std::string(plugin->GetName()) + ".ini";
	ini.LoadFile(path.c_str());

	g_chanceToActivate = static_cast<float>(ini.GetDoubleValue("General", "fChanceToActivate", 20.0));
	g_onlyFemale = static_cast<bool>(ini.GetBoolValue("General", "bOnlyFemale", true));
	g_unequipSlotOnAhegaoStart = static_cast<bool>(ini.GetBoolValue("General", "bUnequipSlotOnAhegaoStart", true));

	//SKSE::log::info("Conf : {}", g_onlyFemale);
}

// ---------------------------------------------------------------
// Called when all game data is loaded - safe to register sinks
// ---------------------------------------------------------------
void OnDataLoaded()
{
	const auto ui = RE::UI::GetSingleton();
	if (ui) {
		ui->AddEventSink<RE::MenuOpenCloseEvent>(DialogueEventSink::GetSingleton());
		SKSE::log::info("Registered MenuOpenCloseEvent sink.");
	}
	else {
		SKSE::log::error("Could not get RE::UI singleton!");
	}

	auto* dataHandler = RE::TESDataHandler::GetSingleton();
	if (!dataHandler) {
		SKSE::log::error("DataHandler not found !");
		return;
	}

	struct ArmorEntry {
		RE::FormID formID;
		const char* pluginName;
	};

	std::vector<ArmorEntry> armorList = {
		{0x000D6C, "Tongues.esp"}, // Tongue 1 - lingaa1armor
		{0x000D6D, "Tongues.esp"}, // Tongue 2 - lingaa2armor
		{0x000D6E, "Tongues.esp"}, // Tongue 3 - lingaa3armor -> X
		{0x000D6F, "Tongues.esp"}, // Tongue 4 - lingaa4armor -> X
		{0x000D70, "Tongues.esp"}, // Tongue 5 - lingaa5armor
		{0x000D71, "Tongues.esp"}, // Tongue 6 - lingaa6armor -> X
		{0x000D72, "Tongues.esp"}, // Tongue 7 - lingaa7armor
		{0x000D73, "Tongues.esp"}, // Tongue 8 - lingaa8armor
		{0x000D74, "Tongues.esp"}, // Tongue 9 - lingaa9armor
		{0x000D75, "Tongues.esp"}, // Tongue 10 - lingaa10armor
	};

	g_armors.clear();

	for (const auto& entry : armorList) {
		auto* armor = dataHandler->LookupForm<RE::TESObjectARMO>(entry.formID, entry.pluginName);
		if (armor) {
			g_armors.push_back(armor);
			SKSE::log::info("Tongue loaded : {} ({:x} in {})", armor->GetName(), entry.formID, entry.pluginName);
		}
		else {
			SKSE::log::error("Tongue not found : {:x} in {}", entry.formID, entry.pluginName);
		}
	}

	if (g_armors.empty()) {
		SKSE::log::error("No Tongue loaded, the mod won't work.");
	}
}

// ---------------------------------------------------------------
// SKSE messaging handler
// ---------------------------------------------------------------
void MessageHandler(SKSE::MessagingInterface::Message* a_msg)
{
	switch (a_msg->type) {
	case SKSE::MessagingInterface::kDataLoaded:
		OnDataLoaded();
		break;
	case SKSE::MessagingInterface::kPostLoad:
		break;
	case SKSE::MessagingInterface::kPreLoadGame:
		break;
	case SKSE::MessagingInterface::kPostLoadGame:
		break;
	case SKSE::MessagingInterface::kNewGame:
		break;
	}
}

// ---------------------------------------------------------------
// Plugin entry point
// ---------------------------------------------------------------
SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
	SKSE::Init(skse);
	SetupLog();
	LoadSettings();

	auto messaging = SKSE::GetMessagingInterface();
	if (!messaging->RegisterListener("SKSE", MessageHandler)) {
		return false;
	}

	return true;
}