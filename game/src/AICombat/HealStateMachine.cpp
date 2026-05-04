#include <AICombat/BigBrawlerStateMachine.hpp>
#include <AICombat/BrawlerStateMachine.hpp>
#include <AICombat/MageStateMachine.hpp>
#include <AICombat/HealStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace AICombat
{
    namespace
    {
        ScriptConf healStateMachineConf = {};
    }

    HealIdleState::HealIdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HealIdleState::Enter()
    {
        if (HealStateMachine* healStateMachine = dynamic_cast<HealStateMachine*>(m_stateMachine))
            healStateMachine->ResetShot();
    }

    void HealIdleState::Update(float)
    {
        if (HealStateMachine* healStateMachine = dynamic_cast<HealStateMachine*>(m_stateMachine))
        {
            if (healStateMachine->FindClosestTarget() != nullptr)
                healStateMachine->ChangeState(HealChaseState::Name);
        }
    }

    HealChaseState::HealChaseState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HealChaseState::Enter()
    {
        if (HealStateMachine* healStateMachine = dynamic_cast<HealStateMachine*>(m_stateMachine))
            healStateMachine->ResetShot();
    }

    void HealChaseState::Update(float _dt)
    {
        HealStateMachine* healStateMachine = dynamic_cast<HealStateMachine*>(m_stateMachine);
        if (healStateMachine == nullptr)
            return;

        Canis::Entity* target = healStateMachine->FindClosestTarget();

        if (target == nullptr)
        {
            healStateMachine->ChangeState(HealIdleState::Name);
            return;
        }

        healStateMachine->FaceTarget(*target);

        if (healStateMachine->DistanceTo(*target) <= healStateMachine->GetAttackRange())
        {
            healStateMachine->ChangeState(HealShotState::Name);
            return;
        }

        healStateMachine->MoveTowards(*target, moveSpeed, _dt);
    }

    HealShotState::HealShotState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HealShotState::Enter()
    {
        if (HealStateMachine* healStatMachine = dynamic_cast<HealStateMachine*>(m_stateMachine))
            healStatMachine->ResetShot();
    }

    void HealShotState::Update(float)
    {
        HealStateMachine* healStatMachine = dynamic_cast<HealStateMachine*>(m_stateMachine);
        if (healStatMachine == nullptr)
            return;

        if (Canis::Entity* target = healStatMachine->FindClosestTarget())
            healStatMachine->FaceTarget(*target);

        duration = duration + 0.001f;

        if (duration >= shotTime)
        {
            healStatMachine->ShootShot();
            duration = 0.0f;
        }

        if (healStatMachine->GetStateTime() < duration)
            return;

        if (healStatMachine->FindClosestTarget() != nullptr)
            healStatMachine->ChangeState(HealChaseState::Name);
        else
            healStatMachine->ChangeState(HealIdleState::Name);
    }

    void HealShotState::Exit()
    {
        if (HealStateMachine* healStatMachine = dynamic_cast<HealStateMachine*>(m_stateMachine))
            healStatMachine->ResetShot();
    }

    HealStateMachine::HealStateMachine(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity),
        idleState(*this),
        chaseState(*this),
        healState(*this) {}

    void RegisterHealStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(healStateMachineConf, AICombat::HealStateMachine, targetTag);
        REGISTER_PROPERTY(healStateMachineConf, AICombat::HealStateMachine, detectionRange);
        REGISTER_PROPERTY(healStateMachineConf, AICombat::HealStateMachine, bodyColliderSize);
        RegisterAccessorProperty(healStateMachineConf, AICombat::HealStateMachine, chaseState, moveSpeed);
        RegisterAccessorProperty(healStateMachineConf, AICombat::HealStateMachine, healState, shotTime);
        RegisterAccessorProperty(healStateMachineConf, AICombat::HealStateMachine, healState, attackRange);
        RegisterAccessorProperty(healStateMachineConf, AICombat::HealStateMachine, healState, duration);
        REGISTER_PROPERTY(healStateMachineConf, AICombat::HealStateMachine, maxHealth);
        REGISTER_PROPERTY(healStateMachineConf, AICombat::HealStateMachine, logStateChanges);
        REGISTER_PROPERTY(healStateMachineConf, AICombat::HealStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(healStateMachineConf, AICombat::HealStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(healStateMachineConf, AICombat::HealStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(healStateMachineConf, AICombat::HealStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            healStateMachineConf,
            AICombat::HealStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        healStateMachineConf.DEFAULT_DRAW_INSPECTOR(AICombat::HealStateMachine);

        _app.RegisterScript(healStateMachineConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(healStateMachineConf, HealStateMachine)

    void HealStateMachine::Create()
    {
        entity.GetComponent<Canis::Transform>();

        Canis::Rigidbody& rigidbody = entity.GetComponent<Canis::Rigidbody>();
        rigidbody.motionType = Canis::RigidbodyMotionType::KINEMATIC;
        rigidbody.useGravity = false;
        rigidbody.allowSleeping = false;
        rigidbody.linearVelocity = Canis::Vector3(0.0f);
        rigidbody.angularVelocity = Canis::Vector3(0.0f);

        entity.GetComponent<Canis::BoxCollider>().size = bodyColliderSize;

        if (entity.HasComponent<Canis::Material>())
        {
            m_baseColor = entity.GetComponent<Canis::Material>().color;
            m_hasBaseColor = true;
        }
    }

    void HealStateMachine::Ready()
    {
        if (entity.HasComponent<Canis::Material>())
        {
            m_baseColor = entity.GetComponent<Canis::Material>().color;
            m_hasBaseColor = true;
        }

        m_currentHealth = std::max(maxHealth, 1);
        m_stateTime = 0.0f;
        m_useFirstHitSfx = true;

        ClearStates();
        AddState(idleState);
        AddState(chaseState);
        AddState(healState);

        ResetShot();
        ChangeState(HealIdleState::Name);
    }

    void HealStateMachine::Destroy()
    {
        SuperPupUtilities::StateMachine::Destroy();
    }

    void HealStateMachine::Update(float _dt)
    {
        if (!IsAlive())
            return;

        m_stateTime += _dt;
        SuperPupUtilities::StateMachine::Update(_dt);
    }

    Canis::Entity* HealStateMachine::FindClosestTarget() const
    {
        if (targetTag.empty() || !entity.HasComponent<Canis::Transform>())
            return nullptr;

        const Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 origin = transform.GetGlobalPosition();
        Canis::Entity* closestTarget = nullptr;
        float closestDistance = detectionRange;

        for (Canis::Entity* candidate : entity.scene.GetEntitiesWithTag(targetTag))
        {
            bool lowHealth = false;

            if (auto* heal = candidate->GetScript<HealStateMachine>())
                lowHealth = heal->GetCurrentHealth() <= 20;

            if (auto* brawler = candidate->GetScript<BrawlerStateMachine>())
                lowHealth = brawler->GetCurrentHealth() <= 20;

            if (auto* big = candidate->GetScript<BigBrawlerStateMachine>())
                lowHealth = big->GetCurrentHealth() <= 50;

            if (auto* mage = candidate->GetScript<MageStateMachine>())
                lowHealth = mage->GetCurrentHealth() <= 10;

            if (lowHealth)
                continue;

            if (candidate == nullptr || candidate == &entity || !candidate->active)
                continue;

            if (!candidate->HasComponent<Canis::Transform>())
                continue;

            if (const HealStateMachine* other = candidate->GetScript<HealStateMachine>())
            {
                if (!other->IsAlive())
                    continue;
            }

            const Canis::Vector3 candidatePosition = candidate->GetComponent<Canis::Transform>().GetGlobalPosition();
            const float distance = glm::distance(origin, candidatePosition);

            if (distance > detectionRange || distance >= closestDistance)
                continue;

            closestDistance = distance;
            closestTarget = candidate;
        }

        return closestTarget;
    }

    float HealStateMachine::DistanceTo(const Canis::Entity& _other) const
    {
        if (!entity.HasComponent<Canis::Transform>() || !_other.HasComponent<Canis::Transform>())
            return std::numeric_limits<float>::max();

        const Canis::Vector3 selfPosition = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
        const Canis::Vector3 targetPosition = _other.GetComponent<Canis::Transform>().GetGlobalPosition();
        return glm::distance(selfPosition, targetPosition);
    }

    void HealStateMachine::FaceTarget(const Canis::Entity& _target)
    {
        if (!entity.HasComponent<Canis::Transform>() || !_target.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();
        Canis::Vector3 direction = _target.GetComponent<Canis::Transform>().GetGlobalPosition() - selfPosition;
        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f)
            return;

        direction = glm::normalize(direction);
        transform.rotation.y = std::atan2(-direction.x, -direction.z);
    }

    void HealStateMachine::MoveTowards(const Canis::Entity& _target, float _speed, float _dt)
    {
        if (!entity.HasComponent<Canis::Transform>() || !_target.HasComponent<Canis::Transform>())
            return;

        Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 selfPosition = transform.GetGlobalPosition();
        Canis::Vector3 direction = _target.GetComponent<Canis::Transform>().GetGlobalPosition() - selfPosition;
        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f)
            return;

        direction = glm::normalize(direction);
        transform.position += direction * _speed * _dt;
    }

    void HealStateMachine::ShootShot()
    {
        for (Canis::Entity* other : entity.GetComponent<Canis::BoxCollider>().entered)
        {
        BigBrawlerStateMachine* target1 = nullptr;
        BrawlerStateMachine* target2 = nullptr;
        MageStateMachine* target3 = nullptr;
        HealStateMachine* target4 = nullptr;
        if (auto* big = other->GetScript<BigBrawlerStateMachine>())
            target1 = big;
        else if (auto* brawler = other->GetScript<BrawlerStateMachine>())
            target2 = brawler;
        else if (auto* mage = other->GetScript<MageStateMachine>())
            target3 = mage;
        else if (auto* heal = other->GetScript<HealStateMachine>())
            target4 = heal;
        if (target1 != nullptr)
            target1->TakeDamage(-3.0f);
        if (target2 != nullptr)
            target2->TakeDamage(-3.0f);
        if (target3 != nullptr)
            target3->TakeDamage(-3.0f);
        if (target4 != nullptr)
            target4->TakeDamage(-3.0f);
        }
        PlayATKSfx();
    }
    
    void HealStateMachine::ResetShot()
    {
        m_stateTime = 0;
    }

    void HealStateMachine::ChangeState(const std::string& _stateName)
    {
        if (SuperPupUtilities::StateMachine::GetCurrentStateName() == _stateName)
            return;

        if (!SuperPupUtilities::StateMachine::ChangeState(_stateName))
            return;

        m_stateTime = 0.0f;

        if (logStateChanges)
            Canis::Debug::Log("%s -> %s", entity.name.c_str(), _stateName.c_str());
    }

    const std::string& HealStateMachine::GetCurrentStateName() const
    {
        return SuperPupUtilities::StateMachine::GetCurrentStateName();
    }

    float HealStateMachine::GetStateTime() const
    {
        return m_stateTime;
    }

    float HealStateMachine::GetAttackRange() const
    {
        return healState.attackRange;
    }

    int HealStateMachine::GetCurrentHealth() const
    {
        return m_currentHealth;
    }

    void HealStateMachine::TakeDamage(int _damage)
    {
        if (!IsAlive())
            return;

        const int damageToApply = std::max(_damage, 0);
        if (damageToApply <= 0)
            return;

        m_currentHealth = std::max(0, m_currentHealth - damageToApply);
        PlayHitSfx();

        if (m_hasBaseColor && entity.HasComponent<Canis::Material>())
        {
            Canis::Material& material = entity.GetComponent<Canis::Material>();
            const float healthRatio = (maxHealth > 0)
                ? (static_cast<float>(m_currentHealth) / static_cast<float>(maxHealth))
                : 0.0f;

            material.color = Canis::Vector4(
                m_baseColor.x * (0.5f + (0.5f * healthRatio)),
                m_baseColor.y * (0.5f + (0.5f * healthRatio)),
                m_baseColor.z * (0.5f + (0.5f * healthRatio)),
                m_baseColor.w);
        }

        if (m_currentHealth > 0)
            return;

        if (logStateChanges)
            Canis::Debug::Log("%s was defeated.", entity.name.c_str());

        SpawnDeathEffect();
        entity.Destroy();
    }

    void HealStateMachine::PlayHitSfx()
    {
        const Canis::AudioAssetHandle& selectedSfx = m_useFirstHitSfx ? hitSfxPath1 : hitSfxPath2;
        m_useFirstHitSfx = !m_useFirstHitSfx;

        if (selectedSfx.Empty())
            return;

        Canis::AudioManager::PlaySFX(selectedSfx, std::clamp(hitSfxVolume, 0.0f, 1.0f));
    }

    void HealStateMachine::PlayATKSfx()
    {
        const Canis::AudioAssetHandle& selectedSfx = healSfx;

        if (selectedSfx.Empty())
            return;

        Canis::AudioManager::PlaySFX(selectedSfx, 1.0f);
    }

    void HealStateMachine::SpawnDeathEffect()
    {
        if (deathEffectPrefab.Empty() || !entity.HasComponent<Canis::Transform>())
            return;

        const Canis::Transform& sourceTransform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 spawnPosition = sourceTransform.GetGlobalPosition();
        const Canis::Vector3 spawnRotation = sourceTransform.GetGlobalRotation();

        for (Canis::Entity* spawnedEntity : entity.scene.Instantiate(deathEffectPrefab))
        {
            if (spawnedEntity == nullptr || !spawnedEntity->HasComponent<Canis::Transform>())
                continue;

            Canis::Transform& spawnedTransform = spawnedEntity->GetComponent<Canis::Transform>();
            spawnedTransform.position = spawnPosition;
            spawnedTransform.rotation = spawnRotation;
        }
    }

    bool HealStateMachine::IsAlive() const
    {
        return m_currentHealth > 0;
    }
}