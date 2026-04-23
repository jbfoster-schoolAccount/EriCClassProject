#include <AICombat/BigHammerDamage.hpp>

#include <AICombat/BigBrawlerStateMachine.hpp>

#include <Canis/App.hpp>
#include <Canis/ConfigHelper.hpp>

#include <algorithm>

namespace AICombat
{
    namespace
    {
        ScriptConf bigHammerDamageConf = {};
    }

    void RegisterBigHammerDamageScript(Canis::App& _app)
    {
        REGISTER_PROPERTY(bigHammerDamageConf, AICombat::BigHammerDamage, owner);
        REGISTER_PROPERTY(bigHammerDamageConf, AICombat::BigHammerDamage, sensorSize);
        REGISTER_PROPERTY(bigHammerDamageConf, AICombat::BigHammerDamage, damage);
        REGISTER_PROPERTY(bigHammerDamageConf, AICombat::BigHammerDamage, targetTag);

        DEFAULT_CONFIG_AND_REQUIRED(
            bigHammerDamageConf,
            AICombat::BigHammerDamage,
            Canis::Transform,
            Canis::Rigidbody,
            Canis::BoxCollider);

        bigHammerDamageConf.DEFAULT_DRAW_INSPECTOR(AICombat::BigHammerDamage);

        _app.RegisterScript(bigHammerDamageConf);
    }

    DEFAULT_UNREGISTER_SCRIPT(bigHammerDamageConf, BigHammerDamage)

    void BigHammerDamage::Create()
    {
        entity.GetComponent<Canis::Transform>();

        Canis::Rigidbody& rigidbody = entity.GetComponent<Canis::Rigidbody>();
        rigidbody.motionType = Canis::RigidbodyMotionType::STATIC;
        rigidbody.useGravity = false;
        rigidbody.isSensor = true;
        rigidbody.allowSleeping = false;
        rigidbody.linearVelocity = Canis::Vector3(0.0f);
        rigidbody.angularVelocity = Canis::Vector3(0.0f);

        entity.GetComponent<Canis::BoxCollider>().size = sensorSize;
    }

    void BigHammerDamage::Ready()
    {
        if (owner == nullptr)
            owner = FindOwnerFromHierarchy();

        if (targetTag.empty())
        {
            if (BigBrawlerStateMachine* ownerStateMachine = GetOwnerStateMachine())
                targetTag = ownerStateMachine->targetTag;
        }
    }

    void BigHammerDamage::Update(float)
    {
        CheckSensorEnter();
    }

    void BigHammerDamage::CheckSensorEnter()
    {
        if (!entity.HasComponents<Canis::BoxCollider, Canis::Rigidbody>())
            return;

        BigBrawlerStateMachine* ownerStateMachine = GetOwnerStateMachine();
        if (ownerStateMachine == nullptr || !ownerStateMachine->IsAlive())
        {
            m_hitTargetsThisSwing.clear();
            return;
        }

        const bool damageWindowOpen =
            ownerStateMachine->GetCurrentStateName() == HammerTimeState::Name &&
            ownerStateMachine->GetStateTime() >= ownerStateMachine->hammerTimeState.attackDamageTime;

        if (!damageWindowOpen)
        {
            m_hitTargetsThisSwing.clear();
            return;
        }

        for (Canis::Entity* other : entity.GetComponent<Canis::BoxCollider>().entered)
        {
            if (other == nullptr || !other->active || other == owner || HasDamagedThisSwing(*other))
                continue;

            BigBrawlerStateMachine* targetStateMachine = other->GetScript<BigBrawlerStateMachine>();
            if (targetStateMachine == nullptr || !targetStateMachine->IsAlive())
                continue;

            if (other->tag != targetTag)
                continue;

            targetStateMachine->TakeDamage(damage);
            m_hitTargetsThisSwing.push_back(other);
        }
    }

    BigBrawlerStateMachine* BigHammerDamage::GetOwnerStateMachine()
    {
        if (owner == nullptr)
            owner = FindOwnerFromHierarchy();

        if (owner == nullptr || !owner->active)
            return nullptr;

        return owner->GetScript<BigBrawlerStateMachine>();
    }

    Canis::Entity* BigHammerDamage::FindOwnerFromHierarchy() const
    {
        if (!entity.HasComponent<Canis::Transform>())
            return nullptr;

        Canis::Entity* current = entity.GetComponent<Canis::Transform>().parent;
        while (current != nullptr)
        {
            if (current->HasScript<BigBrawlerStateMachine>())
                return current;

            if (!current->HasComponent<Canis::Transform>())
                break;

            current = current->GetComponent<Canis::Transform>().parent;
        }

        return nullptr;
    }

    bool BigHammerDamage::HasDamagedThisSwing(Canis::Entity& _target) const
    {
        return std::find(m_hitTargetsThisSwing.begin(), m_hitTargetsThisSwing.end(), &_target)
            != m_hitTargetsThisSwing.end();
    }
}
