#pragma once

#include <Canis/Entity.hpp>

#include <SuperPupUtilities/StateMachine.hpp>

#include <string>

namespace AICombat
{
    class MageStateMachine;

    class MageIdleState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "MageIdleState";

        explicit MageIdleState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class MageChaseState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "MageChaseState";
        float moveSpeed = 4.0f;

        explicit MageChaseState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class MageShotState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "MageShotState";
        float shotTime = 1.00f;
        float attackRange = 12.25f;
        float duration = 0.0f;

        explicit MageShotState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
        void Exit() override;
    };

    class MageStateMachine : public SuperPupUtilities::StateMachine
    {
    public:
        static constexpr const char* ScriptName = "AICombat::MageStateMachine";

        std::string targetTag = "";
        float detectionRange = 25.0f;
        Canis::Vector3 bodyColliderSize = Canis::Vector3(1.0f);
        int maxHealth = 20;
        bool logStateChanges = true;
        Canis::AudioAssetHandle hitSfxPath1 = { .path = "assets/audio/sfx/hit_1.ogg" };
        Canis::AudioAssetHandle hitSfxPath2 = { .path = "assets/audio/sfx/hit_2.ogg" };
        float hitSfxVolume = 1.0f;
        Canis::SceneAssetHandle deathEffectPrefab = { .path = "assets/prefabs/brawler_death_particles.scene" };

        explicit MageStateMachine(Canis::Entity& _entity);

        MageIdleState idleState;
        MageChaseState chaseState;
        MageShotState shotState;

        void Create() override;
        void Ready() override;
        void Destroy() override;
        void Update(float _dt) override;

        Canis::Entity* FindClosestTarget() const;
        float DistanceTo(const Canis::Entity& _other) const;
        void FaceTarget(const Canis::Entity& _target);
        void MoveTowards(const Canis::Entity& _target, float _speed, float _dt);
        void ChangeState(const std::string& _stateName);
        const std::string& GetCurrentStateName() const;
        float GetStateTime() const;
        float GetAttackRange() const;
        int GetCurrentHealth() const;

        void ResetShot();
        void ShootShot();
        void TakeDamage(int _damage);
        bool IsAlive() const;

    private:
        void PlayHitSfx();
        void SpawnDeathEffect();

        int m_currentHealth = 0;
        float m_stateTime = 0.0f;
        Canis::Vector4 m_baseColor = Canis::Vector4(1.0f);
        bool m_hasBaseColor = false;
        bool m_useFirstHitSfx = true;
    };

    void RegisterMageStateMachineScript(Canis::App& _app);
    void UnRegisterMageStateMachineScript(Canis::App& _app);
}
