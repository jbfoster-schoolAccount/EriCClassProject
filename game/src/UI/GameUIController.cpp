#include <UI/GameUIController.hpp>

#include <Machines/Furnace.hpp>
#include <SuperPupUtilities/Inventory.hpp>

#include <Canis/App.hpp>
#include <Canis/AssetManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/InputManager.hpp>
#include <Canis/Window.hpp>

#include <cmath>

namespace
{
    constexpr const char* kInventoryPayloadType = "inventory_item";
    constexpr const char* kUraniumItemName = "Uranium";
    constexpr const char* kGoldOreItemName = "Gold Ore";
    constexpr float kModalDepth = 0.002f;
    constexpr float kPanelDepth = 0.0019f;
    constexpr float kWidgetDepth = 0.0018f;

    Canis::RectTransform& SetupRect(
        Canis::Entity& _entity,
        Canis::Entity* _parent,
        const Canis::Vector2& _position,
        const Canis::Vector2& _size,
        const Canis::Vector2& _anchorMin,
        const Canis::Vector2& _anchorMax,
        const Canis::Vector2& _pivot,
        float _depth)
    {
        Canis::RectTransform& rect = _entity.GetComponent<Canis::RectTransform>();
        rect.anchorMin = _anchorMin;
        rect.anchorMax = _anchorMax;
        rect.pivot = _pivot;
        rect.position = _position;
        rect.size = _size;
        rect.scale = Canis::Vector2(1.0f);
        rect.depth = _depth;
        rect.rotation = 0.0f;
        rect.originOffset = Canis::Vector2(0.0f);
        rect.rotationOriginOffset = Canis::Vector2(0.0f);
        rect.SetParent(_parent);
        return rect;
    }

    Canis::Entity* CreatePanel(
        Canis::Entity& _owner,
        const std::string& _name,
        Canis::Entity* _parent,
        const Canis::Vector2& _position,
        const Canis::Vector2& _size,
        const Canis::Color& _color,
        float _depth,
        const Canis::Vector2& _anchorMin = Canis::Vector2(0.5f, 0.5f),
        const Canis::Vector2& _anchorMax = Canis::Vector2(0.5f, 0.5f),
        const Canis::Vector2& _pivot = Canis::Vector2(0.5f, 0.5f))
    {
        Canis::Entity* entity = _owner.scene.CreateEntity(_name);
        if (entity == nullptr)
            return nullptr;

        SetupRect(*entity, _parent, _position, _size, _anchorMin, _anchorMax, _pivot, _depth);

        Canis::Sprite2D& sprite = entity->GetComponent<Canis::Sprite2D>();
        sprite.textureHandle = Canis::AssetManager::GetTextureHandle("assets/defaults/textures/square.png");
        sprite.color = _color;
        return entity;
    }

    Canis::Entity* CreateLabel(
        Canis::Entity& _owner,
        const std::string& _name,
        Canis::Entity* _parent,
        const Canis::Vector2& _position,
        const Canis::Vector2& _size,
        const std::string& _text,
        unsigned int _fontSize,
        unsigned int _alignment,
        float _depth)
    {
        Canis::Entity* entity = _owner.scene.CreateEntity(_name);
        if (entity == nullptr)
            return nullptr;

        SetupRect(*entity, _parent, _position, _size, Canis::Vector2(0.5f, 0.5f), Canis::Vector2(0.5f, 0.5f), Canis::Vector2(0.5f, 0.5f), _depth);

        Canis::Text& text = entity->GetComponent<Canis::Text>();
        text.assetId = Canis::AssetManager::LoadText("assets/fonts/Antonio-Bold.ttf", _fontSize);
        text.alignment = _alignment;
        text.color = Canis::Color(1.0f);
        text.SetText(_text);
        return entity;
    }

    Canis::Entity* CreateButton(
        Canis::Entity& _owner,
        const std::string& _name,
        Canis::Entity* _parent,
        const Canis::Vector2& _position,
        const Canis::Vector2& _size,
        const std::string& _label,
        const std::string& _actionName)
    {
        Canis::Entity* buttonEntity = CreatePanel(
            _owner,
            _name,
            _parent,
            _position,
            _size,
            Canis::Color(0.14f, 0.18f, 0.24f, 0.98f),
            kWidgetDepth);
        if (buttonEntity == nullptr)
            return nullptr;

        Canis::UIButton& button = buttonEntity->GetComponent<Canis::UIButton>();
        button.targetEntity = &_owner;
        button.targetScript = GameUIController::ScriptName;
        button.actionName = _actionName;
        button.baseColor = Canis::Color(0.14f, 0.18f, 0.24f, 0.98f);
        button.hoverColor = Canis::Color(0.19f, 0.25f, 0.34f, 1.0f);
        button.pressedColor = Canis::Color(0.10f, 0.14f, 0.18f, 1.0f);

        (void)CreateLabel(
            _owner,
            _name + "_Label",
            buttonEntity,
            Canis::Vector2(0.0f, 0.0f),
            _size - Canis::Vector2(16.0f, 16.0f),
            _label,
            28u,
            Canis::TextAlignment::CENTER,
            kWidgetDepth - 0.0001f);

        return buttonEntity;
    }

    Canis::Entity* CreateDropSlot(
        Canis::Entity& _owner,
        const std::string& _name,
        Canis::Entity* _parent,
        const Canis::Vector2& _position,
        const std::string& _actionName)
    {
        Canis::Entity* slotEntity = CreatePanel(
            _owner,
            _name,
            _parent,
            _position,
            Canis::Vector2(140.0f, 140.0f),
            Canis::Color(0.10f, 0.12f, 0.15f, 0.96f),
            kWidgetDepth);
        if (slotEntity == nullptr)
            return nullptr;

        Canis::UIDropTarget& dropTarget = slotEntity->GetComponent<Canis::UIDropTarget>();
        dropTarget.targetEntity = &_owner;
        dropTarget.targetScript = GameUIController::ScriptName;
        dropTarget.actionName = _actionName;
        dropTarget.acceptedPayloadType = kInventoryPayloadType;
        dropTarget.baseColor = Canis::Color(0.10f, 0.12f, 0.15f, 0.96f);
        dropTarget.hoverColor = Canis::Color(0.22f, 0.30f, 0.18f, 1.0f);

        return slotEntity;
    }

    GameUIController::InventoryEntryUI CreateInventoryEntry(
        Canis::Entity& _owner,
        Canis::Entity* _parent,
        const std::string& _itemName,
        const Canis::Vector2& _position,
        const Canis::Color& _color)
    {
        GameUIController::InventoryEntryUI entry = {};
        entry.itemName = _itemName;
        entry.root = CreatePanel(
            _owner,
            "RuntimeUI_" + _itemName + "_Tile",
            _parent,
            _position,
            Canis::Vector2(180.0f, 68.0f),
            _color,
            kWidgetDepth);
        if (entry.root == nullptr)
            return entry;

        Canis::UIDragSource& dragSource = entry.root->GetComponent<Canis::UIDragSource>();
        dragSource.payloadType = kInventoryPayloadType;
        dragSource.payloadValue = _itemName;

        entry.label = CreateLabel(
            _owner,
            "RuntimeUI_" + _itemName + "_Label",
            entry.root,
            Canis::Vector2(0.0f, 10.0f),
            Canis::Vector2(148.0f, 28.0f),
            _itemName,
            24u,
            Canis::TextAlignment::CENTER,
            kWidgetDepth - 0.0001f);

        entry.count = CreateLabel(
            _owner,
            "RuntimeUI_" + _itemName + "_Count",
            entry.root,
            Canis::Vector2(0.0f, -16.0f),
            Canis::Vector2(148.0f, 24.0f),
            "0",
            20u,
            Canis::TextAlignment::CENTER,
            kWidgetDepth - 0.0001f);

        return entry;
    }
}

ScriptConf gameUIControllerConf = {};

void RegisterGameUIControllerScript(Canis::App& _app)
{
    DEFAULT_CONFIG_AND_REQUIRED(gameUIControllerConf, GameUIController, Canis::RectTransform, Canis::Canvas);
    RegisterUIAction(gameUIControllerConf, "resume_pause", &GameUIController::OnResumeClicked);
    RegisterUIAction(gameUIControllerConf, "quit_game", &GameUIController::OnQuitClicked);
    RegisterUIAction(gameUIControllerConf, "close_furnace", &GameUIController::OnCloseFurnaceClicked);
    RegisterUIAction(gameUIControllerConf, "drop_furnace_fuel", &GameUIController::OnFurnaceFuelDropped);
    RegisterUIAction(gameUIControllerConf, "drop_furnace_ore", &GameUIController::OnFurnaceOreDropped);

    gameUIControllerConf.DEFAULT_DRAW_INSPECTOR(GameUIController);

    _app.RegisterScript(gameUIControllerConf);
}

DEFAULT_UNREGISTER_SCRIPT(gameUIControllerConf, GameUIController)

void GameUIController::Create()
{
    DestroyRuntimeChildren();
    BuildPauseMenu();
    BuildFurnacePopup();
    SetModalPage(ModalPage::None);
}

void GameUIController::Ready() {}

void GameUIController::Destroy()
{
    DestroyRuntimeChildren();
}

void GameUIController::Update(float _dt)
{
    (void)_dt;

    if (m_playerEntity == nullptr || !m_playerEntity->active)
        m_playerEntity = entity.scene.GetEntityWithTag("Player");

    Canis::InputManager& input = entity.scene.GetInputManager();
    if (input.JustPressedKey(Canis::Key::ESCAPE))
    {
        if (m_modalPage == ModalPage::PauseMenu || m_modalPage == ModalPage::Furnace)
        {
            SetModalPage(ModalPage::None);
            m_furnaceEntity = nullptr;
        }
        else
        {
            SetModalPage(ModalPage::PauseMenu);
        }
    }

    if (m_modalPage == ModalPage::Furnace)
        RefreshFurnacePopup();
}

void GameUIController::OpenFurnace(Canis::Entity &_furnaceEntity, Canis::Entity &_playerEntity)
{
    m_playerEntity = &_playerEntity;
    m_furnaceEntity = &_furnaceEntity;
    RefreshFurnacePopup();
    SetModalPage(ModalPage::Furnace);
}

void GameUIController::OnResumeClicked(const Canis::UIActionContext &_context)
{
    (void)_context;
    SetModalPage(ModalPage::None);
}

void GameUIController::OnQuitClicked(const Canis::UIActionContext &_context)
{
    (void)_context;
    entity.scene.GetWindow().RequestClose();
}

void GameUIController::OnCloseFurnaceClicked(const Canis::UIActionContext &_context)
{
    (void)_context;
    m_furnaceEntity = nullptr;
    SetModalPage(ModalPage::None);
}

void GameUIController::OnFurnaceFuelDropped(const Canis::UIActionContext &_context)
{
    if (_context.payloadValue != kUraniumItemName || m_playerEntity == nullptr || m_furnaceEntity == nullptr)
        return;

    SuperPupUtilities::Inventory* inventory = m_playerEntity->GetScript<SuperPupUtilities::Inventory>();
    Furnace* furnace = m_furnaceEntity->GetScript<Furnace>();
    if (inventory == nullptr || furnace == nullptr)
        return;

    if (inventory->Remove(kUraniumItemName, 1))
    {
        furnace->uraniumFuelCount++;
        furnace->TryStartProcessing();
        RefreshFurnacePopup();
    }
}

void GameUIController::OnFurnaceOreDropped(const Canis::UIActionContext &_context)
{
    if (_context.payloadValue != kGoldOreItemName || m_playerEntity == nullptr || m_furnaceEntity == nullptr)
        return;

    SuperPupUtilities::Inventory* inventory = m_playerEntity->GetScript<SuperPupUtilities::Inventory>();
    Furnace* furnace = m_furnaceEntity->GetScript<Furnace>();
    if (inventory == nullptr || furnace == nullptr)
        return;

    if (inventory->Remove(kGoldOreItemName, 1))
    {
        furnace->goldOreCount++;
        furnace->TryStartProcessing();
        RefreshFurnacePopup();
    }
}

void GameUIController::DestroyRuntimeChildren()
{
    if (!entity.HasComponent<Canis::RectTransform>())
        return;

    std::vector<Canis::Entity*> children = entity.GetComponent<Canis::RectTransform>().children;
    for (Canis::Entity* child : children)
    {
        if (child != nullptr && child->name.rfind("RuntimeUI_", 0) == 0)
            child->Destroy();
    }

    m_pauseRoot = nullptr;
    m_furnaceRoot = nullptr;
    m_pauseTitleText = nullptr;
    m_resumeButton = nullptr;
    m_quitButton = nullptr;
    m_furnaceTitleText = nullptr;
    m_furnaceStatusText = nullptr;
    m_furnaceFuelCountText = nullptr;
    m_furnaceOreCountText = nullptr;
    m_furnaceCloseButton = nullptr;
    m_furnaceFuelSlot = nullptr;
    m_furnaceOreSlot = nullptr;
    m_inventoryEntries.clear();
}

void GameUIController::BuildPauseMenu()
{
    m_pauseRoot = CreatePanel(
        entity,
        "RuntimeUI_PauseRoot",
        &entity,
        Canis::Vector2(0.0f),
        Canis::Vector2(0.0f),
        Canis::Color(0.03f, 0.05f, 0.07f, 0.68f),
        kModalDepth,
        Canis::Vector2(0.0f, 0.0f),
        Canis::Vector2(1.0f, 1.0f),
        Canis::Vector2(0.5f, 0.5f));

    Canis::Entity* panel = CreatePanel(
        entity,
        "RuntimeUI_PausePanel",
        m_pauseRoot,
        Canis::Vector2(0.0f, 0.0f),
        Canis::Vector2(360.0f, 220.0f),
        Canis::Color(0.06f, 0.08f, 0.11f, 0.97f),
        kPanelDepth);

    m_pauseTitleText = CreateLabel(
        entity,
        "RuntimeUI_PauseTitle",
        panel,
        Canis::Vector2(0.0f, 66.0f),
        Canis::Vector2(280.0f, 40.0f),
        "Paused",
        38u,
        Canis::TextAlignment::CENTER,
        kPanelDepth - 0.0001f);

    m_resumeButton = CreateButton(
        entity,
        "RuntimeUI_ResumeButton",
        panel,
        Canis::Vector2(0.0f, 6.0f),
        Canis::Vector2(220.0f, 52.0f),
        "Resume",
        "resume_pause");

    m_quitButton = CreateButton(
        entity,
        "RuntimeUI_QuitButton",
        panel,
        Canis::Vector2(0.0f, -60.0f),
        Canis::Vector2(220.0f, 52.0f),
        "Quit Game",
        "quit_game");

    if (m_pauseRoot != nullptr)
        m_pauseRoot->active = false;
}

void GameUIController::BuildFurnacePopup()
{
    m_furnaceRoot = CreatePanel(
        entity,
        "RuntimeUI_FurnaceRoot",
        &entity,
        Canis::Vector2(0.0f),
        Canis::Vector2(0.0f),
        Canis::Color(0.03f, 0.05f, 0.07f, 0.68f),
        kModalDepth,
        Canis::Vector2(0.0f, 0.0f),
        Canis::Vector2(1.0f, 1.0f),
        Canis::Vector2(0.5f, 0.5f));

    Canis::Entity* panel = CreatePanel(
        entity,
        "RuntimeUI_FurnacePanel",
        m_furnaceRoot,
        Canis::Vector2(0.0f, 0.0f),
        Canis::Vector2(580.0f, 360.0f),
        Canis::Color(0.06f, 0.08f, 0.11f, 0.98f),
        kPanelDepth);

    m_furnaceTitleText = CreateLabel(
        entity,
        "RuntimeUI_FurnaceTitle",
        panel,
        Canis::Vector2(0.0f, 142.0f),
        Canis::Vector2(320.0f, 34.0f),
        "Furnace",
        36u,
        Canis::TextAlignment::CENTER,
        kPanelDepth - 0.0001f);

    m_furnaceCloseButton = CreateButton(
        entity,
        "RuntimeUI_FurnaceClose",
        panel,
        Canis::Vector2(236.0f, 142.0f),
        Canis::Vector2(72.0f, 42.0f),
        "Close",
        "close_furnace");

    (void)CreateLabel(entity, "RuntimeUI_FuelLabel", panel, Canis::Vector2(-130.0f, 88.0f), Canis::Vector2(180.0f, 24.0f), "Fuel Slot", 24u, Canis::TextAlignment::CENTER, kPanelDepth - 0.0001f);
    (void)CreateLabel(entity, "RuntimeUI_OreLabel", panel, Canis::Vector2(130.0f, 88.0f), Canis::Vector2(180.0f, 24.0f), "Ore Slot", 24u, Canis::TextAlignment::CENTER, kPanelDepth - 0.0001f);

    m_furnaceFuelSlot = CreateDropSlot(entity, "RuntimeUI_FuelSlot", panel, Canis::Vector2(-130.0f, 10.0f), "drop_furnace_fuel");
    m_furnaceOreSlot = CreateDropSlot(entity, "RuntimeUI_OreSlot", panel, Canis::Vector2(130.0f, 10.0f), "drop_furnace_ore");

    m_furnaceFuelCountText = CreateLabel(
        entity,
        "RuntimeUI_FuelCount",
        m_furnaceFuelSlot,
        Canis::Vector2(0.0f, -44.0f),
        Canis::Vector2(120.0f, 24.0f),
        "Fuel: 0",
        22u,
        Canis::TextAlignment::CENTER,
        kWidgetDepth - 0.0001f);

    m_furnaceOreCountText = CreateLabel(
        entity,
        "RuntimeUI_OreCount",
        m_furnaceOreSlot,
        Canis::Vector2(0.0f, -44.0f),
        Canis::Vector2(120.0f, 24.0f),
        "Ore: 0",
        22u,
        Canis::TextAlignment::CENTER,
        kWidgetDepth - 0.0001f);

    m_furnaceStatusText = CreateLabel(
        entity,
        "RuntimeUI_FurnaceStatus",
        panel,
        Canis::Vector2(0.0f, -88.0f),
        Canis::Vector2(420.0f, 48.0f),
        "Idle",
        22u,
        Canis::TextAlignment::CENTER,
        kPanelDepth - 0.0001f);

    (void)CreateLabel(entity, "RuntimeUI_InventoryLabel", panel, Canis::Vector2(0.0f, -136.0f), Canis::Vector2(260.0f, 24.0f), "Drag Items Into Slots", 22u, Canis::TextAlignment::CENTER, kPanelDepth - 0.0001f);

    m_inventoryEntries.push_back(CreateInventoryEntry(entity, panel, kUraniumItemName, Canis::Vector2(-110.0f, -198.0f), Canis::Color(0.20f, 0.50f, 0.18f, 1.0f)));
    m_inventoryEntries.push_back(CreateInventoryEntry(entity, panel, kGoldOreItemName, Canis::Vector2(110.0f, -198.0f), Canis::Color(0.58f, 0.46f, 0.10f, 1.0f)));

    if (m_furnaceRoot != nullptr)
        m_furnaceRoot->active = false;
}

void GameUIController::SetModalPage(ModalPage _page)
{
    const bool wasModalOpen = (m_modalPage != ModalPage::None);
    m_modalPage = _page;

    if (m_pauseRoot != nullptr)
        m_pauseRoot->active = (_page == ModalPage::PauseMenu);
    if (m_furnaceRoot != nullptr)
        m_furnaceRoot->active = (_page == ModalPage::Furnace);

    const bool isModalOpen = (_page != ModalPage::None);
    entity.scene.SetPaused(isModalOpen);

    if (isModalOpen)
    {
        entity.scene.GetWindow().LockMouse(false);
    }
    else if (wasModalOpen)
    {
        entity.scene.GetWindow().LockMouse(true);
    }
}

void GameUIController::RefreshFurnacePopup()
{
    SuperPupUtilities::Inventory* inventory = (m_playerEntity == nullptr) ? nullptr : m_playerEntity->GetScript<SuperPupUtilities::Inventory>();
    Furnace* furnace = (m_furnaceEntity == nullptr) ? nullptr : m_furnaceEntity->GetScript<Furnace>();

    if (inventory == nullptr || furnace == nullptr)
    {
        if (m_modalPage == ModalPage::Furnace)
            SetModalPage(ModalPage::None);
        return;
    }

    if (m_furnaceFuelCountText != nullptr && m_furnaceFuelCountText->HasComponent<Canis::Text>())
        m_furnaceFuelCountText->GetComponent<Canis::Text>().SetText("Fuel: " + std::to_string(furnace->uraniumFuelCount));

    if (m_furnaceOreCountText != nullptr && m_furnaceOreCountText->HasComponent<Canis::Text>())
        m_furnaceOreCountText->GetComponent<Canis::Text>().SetText("Ore: " + std::to_string(furnace->goldOreCount));

    if (m_furnaceStatusText != nullptr && m_furnaceStatusText->HasComponent<Canis::Text>())
    {
        Canis::Text& statusText = m_furnaceStatusText->GetComponent<Canis::Text>();
        if (furnace->timeLeft > 0.0f)
            statusText.SetText("Smelting... " + std::to_string(static_cast<int>(std::ceil(furnace->timeLeft))) + "s");
        else if (furnace->goldOreCount > 0 && furnace->uraniumFuelCount <= 0)
            statusText.SetText("Need uranium fuel");
        else
            statusText.SetText("Idle");
    }

    for (InventoryEntryUI& entry : m_inventoryEntries)
    {
        if (entry.root == nullptr)
            continue;

        const int count = inventory->GetCount(entry.itemName);
        entry.root->active = count > 0;

        if (entry.count != nullptr && entry.count->HasComponent<Canis::Text>())
            entry.count->GetComponent<Canis::Text>().SetText("x" + std::to_string(count));
    }
}
