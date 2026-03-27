#pragma once

#include <Canis/Entity.hpp>

class GameUIController : public Canis::ScriptableEntity
{
public:
    struct InventoryEntryUI
    {
        std::string itemName = "";
        Canis::Entity* root = nullptr;
        Canis::Entity* label = nullptr;
        Canis::Entity* count = nullptr;
    };

private:
    enum class ModalPage
    {
        None = 0,
        PauseMenu,
        Furnace,
    };

    ModalPage m_modalPage = ModalPage::None;
    Canis::Entity* m_playerEntity = nullptr;
    Canis::Entity* m_furnaceEntity = nullptr;

    Canis::Entity* m_pauseRoot = nullptr;
    Canis::Entity* m_furnaceRoot = nullptr;

    Canis::Entity* m_pauseTitleText = nullptr;
    Canis::Entity* m_resumeButton = nullptr;
    Canis::Entity* m_quitButton = nullptr;

    Canis::Entity* m_furnaceTitleText = nullptr;
    Canis::Entity* m_furnaceStatusText = nullptr;
    Canis::Entity* m_furnaceFuelCountText = nullptr;
    Canis::Entity* m_furnaceOreCountText = nullptr;
    Canis::Entity* m_furnaceCloseButton = nullptr;
    Canis::Entity* m_furnaceFuelSlot = nullptr;
    Canis::Entity* m_furnaceOreSlot = nullptr;

    std::vector<InventoryEntryUI> m_inventoryEntries = {};

    void DestroyRuntimeChildren();
    void BuildPauseMenu();
    void BuildFurnacePopup();
    void SetModalPage(ModalPage _page);
    void RefreshFurnacePopup();

public:
    static constexpr const char* ScriptName = "GameUIController";

    explicit GameUIController(Canis::Entity &_entity) : Canis::ScriptableEntity(_entity) {}

    void Create() override;
    void Ready() override;
    void Destroy() override;
    void Update(float _dt) override;
    bool UpdateWhenPaused() const override { return true; }

    void OpenFurnace(Canis::Entity &_furnaceEntity, Canis::Entity &_playerEntity);

    void OnResumeClicked(const Canis::UIActionContext &_context);
    void OnQuitClicked(const Canis::UIActionContext &_context);
    void OnCloseFurnaceClicked(const Canis::UIActionContext &_context);
    void OnFurnaceFuelDropped(const Canis::UIActionContext &_context);
    void OnFurnaceOreDropped(const Canis::UIActionContext &_context);
};

extern void RegisterGameUIControllerScript(Canis::App& _app);
extern void UnRegisterGameUIControllerScript(Canis::App& _app);
