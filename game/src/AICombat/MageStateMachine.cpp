#include <AICombat/MageStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/AudioManager.hpp>
#include <Canis/ConfigHelper.hpp>
#include <Canis/Debug.hpp>

#include <SuperPupUtilities/Bullet.hpp>
#include <SuperPupUtilities/SimpleObjectPool.hpp>

#include <algorithm>
#include <cmath>
#include <limits>

namespace AICombat
{
    namespace
    {
        ScriptConf mageStateMachineConf = {};
    }

    MageIdleState::MageIdleState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void MageIdleState::Enter()
    {
        if (MageStateMachine* mageStateMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
            mageStateMachine->ResetShot();
    }

    void MageIdleState::Update(float)
    {
        if (MageStateMachine* mageStateMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
        {
            if (mageStateMachine->FindClosestTarget() != nullptr)
                mageStateMachine->ChangeState(MageChaseState::Name);
        }
    }

    MageChaseState::MageChaseState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void MageChaseState::Enter()
    {
        if (MageStateMachine* mageStateMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
            mageStateMachine->ResetShot();
    }

    void MageChaseState::Update(float _dt)
    {
        MageStateMachine* mageStateMachine = dynamic_cast<MageStateMachine*>(m_stateMachine);
        if (mageStateMachine == nullptr)
            return;

        Canis::Entity* target = mageStateMachine->FindClosestTarget();

        if (target == nullptr)
        {
            mageStateMachine->ChangeState(MageIdleState::Name);
            return;
        }

        mageStateMachine->FaceTarget(*target);

        if (mageStateMachine->DistanceTo(*target) <= mageStateMachine->GetAttackRange())
        {
            mageStateMachine->ChangeState(MageShotState::Name);
            return;
        }

        mageStateMachine->MoveTowards(*target, moveSpeed, _dt);
    }

    MageShotState::MageShotState(SuperPupUtilities::StateMachine& _stateMachine) :
        State(Name, _stateMachine) {}

    void MageShotState::Enter()
    {
        if (MageStateMachine* mageStatMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
            mageStatMachine->ResetShot();
    }

    void MageShotState::Update(float)
    {
        MageStateMachine* mageStatMachine = dynamic_cast<MageStateMachine*>(m_stateMachine);
        if (mageStatMachine == nullptr)
            return;

        if (Canis::Entity* target = mageStatMachine->FindClosestTarget())
            mageStatMachine->FaceTarget(*target);

        duration = duration + 0.001f;

        if (duration >= shotTime)
        {
            mageStatMachine->ShootShot();
            duration = 0.0f;
        }

        if (mageStatMachine->GetStateTime() < duration)
            return;

        if (mageStatMachine->FindClosestTarget() != nullptr)
            mageStatMachine->ChangeState(MageChaseState::Name);
        else
            mageStatMachine->ChangeState(MageIdleState::Name);
    }

    void MageShotState::Exit()
    {
        if (MageStateMachine* mageStatMachine = dynamic_cast<MageStateMachine*>(m_stateMachine))
            mageStatMachine->ResetShot();
    }

    MageStateMachine::MageStateMachine(Canis::Entity& _entity) :
        SuperPupUtilities::StateMachine(_entity),
        idleState(*this),
        chaseState(*this),
        shotState(*this) {}

    void RegisterMageStateMachineScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, targetTag);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, detectionRange);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, bodyColliderSize);
        RegisterAccessorProperty(mageStateMachineConf, AICombat::MageStateMachine, chaseState, moveSpeed);
        RegisterAccessorProperty(mageStateMachineConf, AICombat::MageStateMachine, shotState, shotTime);
        RegisterAccessorProperty(mageStateMachineConf, AICombat::MageStateMachine, shotState, attackRange);
        RegisterAccessorProperty(mageStateMachineConf, AICombat::MageStateMachine, shotState, duration);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, maxHealth);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, logStateChanges);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, hitSfxPath1);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, hitSfxPath2);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, hitSfxVolume);
        REGISTER_PROPERTY(mageStateMachineConf, AICombat::MageStateMachine, deathEffectPrefab);

        DEFAULT_CONFIG_AND_REQUIRED(
            mageStateMachineConf,
            AICombat::MageStateMachine,
            Canis::Transform,
            Canis::Material,
            Canis::Model,
            Canis::Rigidbody,
            Canis::BoxCollider);

        mageStateMachineConf.DEFAULT_DRAW_INSPECTOR(AICombat::MageStateMachine);

        _app.RegisterScript(mageStateMachineConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(mageStateMachineConf, MageStateMachine)

    void MageStateMachine::Create()
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

    void MageStateMachine::Ready()
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
        AddState(shotState);

        ResetShot();
        ChangeState(MageIdleState::Name);
    }

    void MageStateMachine::Destroy()
    {
        SuperPupUtilities::StateMachine::Destroy();
    }

    void MageStateMachine::Update(float _dt)
    {
        if (!IsAlive())
            return;

        m_stateTime += _dt;
        SuperPupUtilities::StateMachine::Update(_dt);
    }

    Canis::Entity* MageStateMachine::FindClosestTarget() const
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

            if (const MageStateMachine* other = candidate->GetScript<MageStateMachine>())
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

    float MageStateMachine::DistanceTo(const Canis::Entity& _other) const
    {
        if (!entity.HasComponent<Canis::Transform>() || !_other.HasComponent<Canis::Transform>())
            return std::numeric_limits<float>::max();

        const Canis::Vector3 selfPosition = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
        const Canis::Vector3 targetPosition = _other.GetComponent<Canis::Transform>().GetGlobalPosition();
        return glm::distance(selfPosition, targetPosition);
    }

    void MageStateMachine::FaceTarget(const Canis::Entity& _target)
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

    void MageStateMachine::MoveTowards(const Canis::Entity& _target, float _speed, float _dt)
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

    void MageStateMachine::ShootShot()
    {
        if (!entity.HasComponent<Canis::Transform>())
            return;

        Canis::Entity* target = FindClosestTarget();
        if (target == nullptr || !target->HasComponent<Canis::Transform>())
            return;

        const Canis::Vector3 position = entity.GetComponent<Canis::Transform>().GetGlobalPosition();
        const Canis::Vector3 targetPos = target->GetComponent<Canis::Transform>().GetGlobalPosition();
    
        Canis::Vector3 direction = targetPos - position;
        direction.y = 0.0f;

        if (glm::dot(direction, direction) <= 0.0001f)
            return;

        direction = glm::normalize(direction);

        const float yaw = std::atan2(-direction.x, -direction.z);
        const Canis::Vector3 rotation = Canis::Vector3(0.0f, yaw, 0.0f);

        auto* pool = SuperPupUtilities::SimpleObjectPool::Instance;
        if (pool == nullptr)
            return;

        Canis::Entity* projectile = pool->Spawn("laser_bullet", position, rotation);
        if (projectile == nullptr)
            return;

        if (SuperPupUtilities::Bullet* bullet = projectile->GetScript<SuperPupUtilities::Bullet>())
        {
            Debug::Log("BulletDid thing");
            bullet->speed = 10.0f;
            bullet->lifeTime = 2.0f;
            bullet->hitImpulse = 1.0f;

            bullet->targetTags.clear();
            bullet->targetTags.push_back(target->tag);

            bullet->Launch();
        }
    }
    
    void MageStateMachine::ResetShot()
    {
        m_stateTime = 0;
    }

    void MageStateMachine::ChangeState(const std::string& _stateName)
    {
        if (SuperPupUtilities::StateMachine::GetCurrentStateName() == _stateName)
            return;

        if (!SuperPupUtilities::StateMachine::ChangeState(_stateName))
            return;

        m_stateTime = 0.0f;

        if (logStateChanges)
            Canis::Debug::Log("%s -> %s", entity.name.c_str(), _stateName.c_str());
    }

    const std::string& MageStateMachine::GetCurrentStateName() const
    {
        return SuperPupUtilities::StateMachine::GetCurrentStateName();
    }

    float MageStateMachine::GetStateTime() const
    {
        return m_stateTime;
    }

    float MageStateMachine::GetAttackRange() const
    {
        return shotState.attackRange;
    }

    int MageStateMachine::GetCurrentHealth() const
    {
        return m_currentHealth;
    }

    void MageStateMachine::TakeDamage(int _damage)
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

    void MageStateMachine::PlayHitSfx()
    {
        const Canis::AudioAssetHandle& selectedSfx = m_useFirstHitSfx ? hitSfxPath1 : hitSfxPath2;
        m_useFirstHitSfx = !m_useFirstHitSfx;

        if (selectedSfx.Empty())
            return;

        Canis::AudioManager::PlaySFX(selectedSfx, std::clamp(hitSfxVolume, 0.0f, 1.0f));
    }

    void MageStateMachine::SpawnDeathEffect()
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

    bool MageStateMachine::IsAlive() const
    {
        return m_currentHealth > 0;
    }
}
