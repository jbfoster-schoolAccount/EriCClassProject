#pragma once

#include <Canis/Entity.hpp>

#include <SuperPupUtilities/StateMachine.hpp>

#include <string>

namespace AICombat
{
    class HealStateMachine;

    class HealIdleState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "HealIdleState";

        explicit HealIdleState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class HealChaseState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "HealChaseState";
        float moveSpeed = 4.0f;

        explicit HealChaseState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
    };

    class HealShotState : public SuperPupUtilities::State
    {
    public:
        static constexpr const char* Name = "HealShotState";
        float shotTime = 1.00f;
        float attackRange = 12.25f;
        float duration = 0.0f;

        explicit HealShotState(SuperPupUtilities::StateMachine& _stateMachine);
        void Enter() override;
        void Update(float _dt) override;
        void Exit() override;
    };

    class HealStateMachine : public SuperPupUtilities::StateMachine
    {
    public:
        static constexpr const char* ScriptName = "AICombat::HealStateMachine";

        std::string targetTag = "";
        float detectionRange = 25.0f;
        Canis::Vector3 bodyColliderSize = Canis::Vector3(1.0f);
        int maxHealth = 15;
        bool logStateChanges = true;
        Canis::AudioAssetHandle hitSfxPath1 = { .path = "assets/audio/sfx/hit_1.ogg" };
        Canis::AudioAssetHandle hitSfxPath2 = { .path = "assets/audio/sfx/hit_2.ogg" };
        Canis::AudioAssetHandle healSfx = { .path = "assets/audio/sfx/heal.wav" };
        float hitSfxVolume = 1.0f;
        Canis::SceneAssetHandle deathEffectPrefab = { .path = "assets/prefabs/brawler_death_particles.scene" };

        explicit HealStateMachine(Canis::Entity& _entity);

        HealIdleState idleState;
        HealChaseState chaseState;
        HealShotState healState;

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
        void PlayATKSfx();
        void SpawnDeathEffect();

        int m_currentHealth = 0;
        float m_stateTime = 0.0f;
        Canis::Vector4 m_baseColor = Canis::Vector4(1.0f);
        bool m_hasBaseColor = false;
        bool m_useFirstHitSfx = true;
    };

    void RegisterHealStateMachineScript(Canis::App& _app);
    void UnRegisterHealStateMachineScript(Canis::App& _app);
}
