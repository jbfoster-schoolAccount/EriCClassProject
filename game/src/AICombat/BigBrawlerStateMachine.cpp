#include <AICombat/BigBrawlerStateMachine.hpp>

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
        ScriptConf bigBrawlerStateMachineConf = {};
    }

    IdleState::IdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void IdleState::Enter()
    {
        if (BigBrawlerStateMachine* bigBrawlerStatMachine = dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine))
            bigBrawlerStatMachine->ResetHammerPose();
    }

    void IdleState::Update(float)
    {
        if (BigBrawlerStateMachine* bigBrawlerStatMachine = dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine))
        {
            if (bigBrawlerStatMachine->FindClosestTarget() != nullptr)
                bigBrawlerStatMachine->ChangeState(ChaseState::Name);
        }
    }

    ChaseState::ChaseState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void ChaseState::Enter()
    {
        if (BigBrawlerStateMachine* bigBrawlerStatMachine = dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine))
            bigBrawlerStatMachine->ResetHammerPose();
    }

    void ChaseState::Update(float _dt)
    {
        BigBrawlerStateMachine* bigBrawlerStatMachine = dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine);
        if (bigBrawlerStatMachine == nullptr)
            return;

        Canis::Entity* target = bigBrawlerStatMachine->FindClosestTarget();

        if (target == nullptr)
        {
            bigBrawlerStatMachine->ChangeState(IdleState::Name);
            return;
        }

        bigBrawlerStatMachine->FaceTarget(*target);

        if (bigBrawlerStatMachine->DistanceTo(*target) <= bigBrawlerStatMachine->GetAttackRange())
        {
            bigBrawlerStatMachine->ChangeState(HammerTimeState::Name);
            return;
        }

        bigBrawlerStatMachine->MoveTowards(*target, moveSpeed, _dt);
    }

    HammerTimeState::HammerTimeState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void HammerTimeState::Enter()
    {
        if (BigBrawlerStateMachine* bigBrawlerStatMachine = dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine))
            bigBrawlerStatMachine->SetHammerSwing(0.0f);
    }

    void HammerTimeState::Update(float)
    {
        BigBrawlerStateMachine* bigBrawlerStatMachine = dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine);
        if (bigBrawlerStatMachine == nullptr)
            return;

        if (Canis::Entity* target = bigBrawlerStatMachine->FindClosestTarget())
            bigBrawlerStatMachine->FaceTarget(*target);

        const float duration = std::max(attackDuration, 0.001f);
        bigBrawlerStatMachine->SetHammerSwing(bigBrawlerStatMachine->GetStateTime() / duration);

        if (bigBrawlerStatMachine->GetStateTime() < duration)
            return;

        if (bigBrawlerStatMachine->FindClosestTarget() != nullptr)
            bigBrawlerStatMachine->ChangeState(ChaseState::Name);
        else
            bigBrawlerStatMachine->ChangeState(IdleState::Name);
    }

    void HammerTimeState::Exit()
    {
        if (BigBrawlerStateMachine* bigBrawlerStatMachine = dynamic_cast<BigBrawlerStateMachine*>(m_stateMachine))
            bigBrawlerStatMachine->ResetHammerPose();
    }

    BigBrawlerStateMachine::BigBrawlerStateMachine(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity),
        idleState(*this),
        chaseState(*this),
        hammerTimeState(*this) {}

    void RegisterBigBrawlerStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, targetTag);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, detectionRange);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, bodyColliderSize);
        RegisterAccessorProperty(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, chaseState, moveSpeed);
        RegisterAccessorProperty(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hammerTimeState, hammerRestDegrees);
        RegisterAccessorProperty(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hammerTimeState, hammerSwingDegrees);
        RegisterAccessorProperty(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hammerTimeState, attackRange);
        RegisterAccessorProperty(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hammerTimeState, attackDuration);
        RegisterAccessorProperty(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hammerTimeState, attackDamageTime);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, maxHealth);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, logStateChanges);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hammerVisual);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(bigBrawlerStateMachineConf, AICombat::BigBrawlerStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            bigBrawlerStateMachineConf,
            AICombat::BigBrawlerStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        bigBrawlerStateMachineConf.DEFAULT_DRAW_INSPECTOR(AICombat::BigBrawlerStateMachine);

        _app.RegisterScript(bigBrawlerStateMachineConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(bigBrawlerStateMachineConf, BigBrawlerStateMachine)

    void BigBrawlerStateMachine::Create()
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

    void BigBrawlerStateMachine::Ready()
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
        AddState(hammerTimeState);

        ResetHammerPose();
        ChangeState(IdleState::Name);
    }

    void BigBrawlerStateMachine::Destroy()
    {
        hammerVisual = nullptr;
        SuperPupUtilities::StateMachine::Destroy();
    }

    void BigBrawlerStateMachine::Update(float _dt)
    {
        if (!IsAlive())
            return;

        m_stateTime += _dt;
        SuperPupUtilities::StateMachine::Update(_dt);
    }

    Canis::Entity* BigBrawlerStateMachine::FindClosestTarget() const
    {
        if (targetTag.empty() || !entity.HasComponent<Canis::Transform>())
            return nullptr;

        const Canis::Transform& transform = entity.GetComponent<Canis::Transform>();
        const Canis::Vector3 origin = transform.GetGlobalPosition();
        Canis::Entity* closestTarget = nullptr;
        float closestDistance = detectionRange;

        for (Canis::Entity* candidate : entity.scene.GetEntitiesWithTag(targetTag))
        {
            if (candidate == nullptr || candidate == &entity || !candidate->active)
                continue;

            if (!candidate->HasComponent<Canis::Transform>())
                continue;

            if (const BigBrawlerStateMachine* other = candidate->GetScript<BigBrawlerStateMachine>())
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

    float BigBrawlerStateMachine::DistanceTo(const Canis::Entity& _other) const
    {
        if (!entity.HasComponent<Canis::Transform>() || !_other.HasComponent<Canis::Transform>())
            return std::numeric_limits<float>::max();

        const Canis::Vector3 selfPosition = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
        const Canis::Vector3 targetPosition = _other.GetComponent<Canis::Transform>().GetGlobalPosition();
        return glm::distance(selfPosition, targetPosition);
    }

    void BigBrawlerStateMachine::FaceTarget(const Canis::Entity& _target)
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

    void BigBrawlerStateMachine::MoveTowards(const Canis::Entity& _target, float _speed, float _dt)
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

    void BigBrawlerStateMachine::ChangeState(const std::string& _stateName)
    {
        if (SuperPupUtilities::StateMachine::GetCurrentStateName() == _stateName)
            return;

        if (!SuperPupUtilities::StateMachine::ChangeState(_stateName))
            return;

        m_stateTime = 0.0f;

        if (logStateChanges)
            Canis::Debug::Log("%s -> %s", entity.name.c_str(), _stateName.c_str());
    }

    const std::string& BigBrawlerStateMachine::GetCurrentStateName() const
    {
        return SuperPupUtilities::StateMachine::GetCurrentStateName();
    }

    float BigBrawlerStateMachine::GetStateTime() const
    {
        return m_stateTime;
    }

    float BigBrawlerStateMachine::GetAttackRange() const
    {
        return hammerTimeState.attackRange;
    }

    int BigBrawlerStateMachine::GetCurrentHealth() const
    {
        return m_currentHealth;
    }

    void BigBrawlerStateMachine::ResetHammerPose()
    {
        SetHammerSwing(0.0f);
    }

    void BigBrawlerStateMachine::SetHammerSwing(float _normalized)
    {
        if (hammerVisual == nullptr || !hammerVisual->HasComponent<Canis::Transform>())
            return;

        Canis::Transform& hammerTransform = hammerVisual->GetComponent<Canis::Transform>();
        const float normalized = Clamp01(_normalized);
        const float swingBlend = (normalized <= 0.5f)
            ? normalized * 2.0f
            : (1.0f - normalized) * 2.0f;

        hammerTransform.rotation.z = DEG2RAD *
            (hammerTimeState.hammerRestDegrees + (hammerTimeState.hammerSwingDegrees * swingBlend));
    }

    void BigBrawlerStateMachine::TakeDamage(int _damage)
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

    void BigBrawlerStateMachine::PlayHitSfx()
    {
        const Canis::AudioAssetHandle& selectedSfx = m_useFirstHitSfx ? hitSfxPath1 : hitSfxPath2;
        m_useFirstHitSfx = !m_useFirstHitSfx;

        if (selectedSfx.Empty())
            return;

        Canis::AudioManager::PlaySFX(selectedSfx, std::clamp(hitSfxVolume, 0.0f, 1.0f));
    }

    void BigBrawlerStateMachine::SpawnDeathEffect()
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

    bool BigBrawlerStateMachine::IsAlive() const
    {
        return m_currentHealth > 0;
    }
}
