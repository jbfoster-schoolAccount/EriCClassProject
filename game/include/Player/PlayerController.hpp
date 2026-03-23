#pragma once

#include <Canis/Entity.hpp>

class PlayerController : public Canis::ScriptableEntity
{
private:

public:
    static constexpr const char* ScriptName = "PlayerController";

    float walkingSpeed = 1000.0f;
    float sprintingSpeed = 3000.0f;
    float jumpImpulse = 7.5f;
    float groundCheckDistance = 0.75f;
    Canis::Mask groundCollisionMask = Canis::Rigidbody::DefaultLayer;
    float pickupRadius = 1.15f;

    bool grounded = false;

    PlayerController(Canis::Entity &_entity) : Canis::ScriptableEntity(_entity) {}

    void Create();
    void Ready();
    void Destroy();
    void Update(float _dt);
};

extern void RegisterPlayerControllerScript(Canis::App& _app);
extern void UnRegisterPlayerControllerScript(Canis::App& _app);