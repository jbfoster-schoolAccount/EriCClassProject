#include <Player/PlayerController.hpp>

#include <Canis/App.hpp>
#include <Canis/InputManager.hpp>
#include <Canis/ConfigHelper.hpp>


ScriptConf playerConf = {};

void RegisterPlayerControllerScript(App& _app)
{
    REGISTER_PROPERTY(playerConf, PlayerController, walkingSpeed);
    REGISTER_PROPERTY(playerConf, PlayerController, sprintingSpeed);
    REGISTER_PROPERTY(playerConf, PlayerController, jumpImpulse);
    REGISTER_PROPERTY(playerConf, PlayerController, groundCheckDistance);
    REGISTER_PROPERTY(playerConf, PlayerController, groundCollisionMask);
    REGISTER_PROPERTY(playerConf, PlayerController, pickupRadius);
    
    DEFAULT_CONFIG_AND_REQUIRED(playerConf, PlayerController, Transform, Rigidbody, CapsuleCollider);

    playerConf.DEFAULT_DRAW_INSPECTOR(PlayerController);

    _app.RegisterScript(playerConf);
}

DEFAULT_UNREGISTER_SCRIPT(playerConf, PlayerController)

void PlayerController::Create() {}

void PlayerController::Ready() {}

void PlayerController::Destroy() {}

void PlayerController::Update(float _dt)
{
    if (!entity.HasComponents<Transform, Rigidbody>())
        return;

    Transform& transform = entity.GetComponent<Transform>();
    Rigidbody& rigidbody = entity.GetComponent<Rigidbody>();

    InputManager& input = entity.scene.GetInputManager();
    RaycastHit groundHit = {};
    const Vector3 groundRayOrigin = transform.GetGlobalPosition() + Vector3(0.0f, 0.1f, 0.0f);
    grounded = entity.scene.Raycast(groundRayOrigin, Vector3(0.0f, -1.0f, 0.0f), groundHit, groundCheckDistance, groundCollisionMask);

    Vector3 inputDirection = Vector3(0.0f);

    if (input.GetKey(Key::A) || input.GetKey(Key::LEFT))
        inputDirection.x -= 1.0f;
    if (input.GetKey(Key::D) || input.GetKey(Key::RIGHT))
        inputDirection.x += 1.0f;
    if (input.GetKey(Key::W) || input.GetKey(Key::UP))
        inputDirection.z -= 1.0f;
    if (input.GetKey(Key::S) || input.GetKey(Key::DOWN))
        inputDirection.z += 1.0f;
    
    bool sprint = input.GetKey(Key::LSHIFT);

    Vector3 movement = inputDirection;

    if (grounded && input.JustPressedKey(Key::SPACE))
        rigidbody.AddForce(Vector3(0.0f, jumpImpulse, 0.0f), Rigidbody3DForceMode::IMPULSE);

    if (movement != Vector3(0.0f))
    {
        movement = glm::normalize(movement);
        float speed = sprint ? sprintingSpeed : walkingSpeed;
        rigidbody.AddForce(movement * speed * _dt, Rigidbody3DForceMode::FORCE);
    }
}