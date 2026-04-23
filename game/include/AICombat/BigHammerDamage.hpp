#pragma once

#include <Canis/Entity.hpp>

#include <vector>

namespace AICombat
{
    class BigBrawlerStateMachine;

    class BigHammerDamage : public Canis::ScriptableEntity
    {
    public:
        static constexpr const char* ScriptName = "AICombat::BigHammerDamage";

        Canis::Entity* owner = nullptr;
        Canis::Vector3 sensorSize = Canis::Vector3(2.0f);
        int damage = 20;
        std::string targetTag = "";

        explicit BigHammerDamage(Canis::Entity& _entity) : Canis::ScriptableEntity(_entity) {}

        void Create() override;
        void Ready() override;
        void Update(float _dt) override;

        void CheckSensorEnter();

    private:
        BigBrawlerStateMachine* GetOwnerStateMachine();
        Canis::Entity* FindOwnerFromHierarchy() const;
        bool HasDamagedThisSwing(Canis::Entity& _target) const;

        std::vector<Canis::Entity*> m_hitTargetsThisSwing = {};
    };

    void RegisterBigHammerDamageScript(Canis::App& _app);
    void UnRegisterBigHammerDamageScript(Canis::App& _app);
}
