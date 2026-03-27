#include <Machines/Furnace.hpp>

#include <UI/GameUIController.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/InputManager.hpp>

ScriptConf furnaceConf = {};

void RegisterFurnaceScript(App& _app)
{
    DEFAULT_CONFIG(furnaceConf, Furnace);
    REGISTER_PROPERTY(furnaceConf, Furnace, goldOreCount);
    REGISTER_PROPERTY(furnaceConf, Furnace, uraniumFuelCount);
    REGISTER_PROPERTY(furnaceConf, Furnace, timeLeft);
    REGISTER_PROPERTY(furnaceConf, Furnace, processingTime);
    REGISTER_PROPERTY(furnaceConf, Furnace, dropPrefab);

    furnaceConf.DEFAULT_DRAW_INSPECTOR(Furnace);

    _app.RegisterScript(furnaceConf);
}

DEFAULT_UNREGISTER_SCRIPT(furnaceConf, Furnace)

void Furnace::Create() {}

void Furnace::Ready() {}

void Furnace::Destroy() {}

void Furnace::Update(float _dt) {
    if (!entity.HasComponent<Transform>())
        return;
    
    if (timeLeft == 0.0f)
        return;
    
    timeLeft -= _dt;

    if (timeLeft <= 0.0f){
        timeLeft = 0.0f;
        goldOreCount--;

        Vector3 spawnOffset = entity.GetComponent<Transform>().GetGlobalPosition() + Vector3(0.0f, 0.0f, 1.0f);

        for (Entity *root : entity.scene.Instantiate(dropPrefab))
        {
            if (root != nullptr && root->HasComponent<Transform>())
                root->GetComponent<Transform>().position += spawnOffset;
        }

        TryStartProcessing();
    }
}

void Furnace::TryStartProcessing()
{
    if (timeLeft > 0.0f)
        return;

    if (goldOreCount <= 0 || uraniumFuelCount <= 0)
        return;

    uraniumFuelCount--;
    timeLeft = processingTime;
}

std::string Furnace::GetMessage(const InteractionContext &_context)
{
    (void)_context;
    std::string message = "Left Click to open Furnace.";

    if (timeLeft > 0.0f)
        message = "Processing\nOre " + std::to_string(goldOreCount) + " Fuel " + std::to_string(uraniumFuelCount) + "\n" + message;
    else
        message = "Ore " + std::to_string(goldOreCount) + " Fuel " + std::to_string(uraniumFuelCount) + "\n" + message;

    return message;
}

bool Furnace::HandleInteraction(const InteractionContext &_context)
{
    InputManager& input = entity.scene.GetInputManager();
    if (!input.LeftClickReleased())
        return false;

    if (_context.interactingEntity == nullptr)
        return false;

    if (Canis::Entity* hudCanvas = entity.scene.FindEntityWithName("HUD_Canvas"))
    {
        if (GameUIController* gameUI = hudCanvas->GetScript<GameUIController>())
        {
            gameUI->OpenFurnace(entity, *_context.interactingEntity);
            return true;
        }
    }

    return false;
}
