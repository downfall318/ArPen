// One queued packet owns one canceled hard-armor hit. Values are damage amounts,
// never health snapshots: successive queued bullets consume the current pools.
class ArPenZoneDamage
{
    string ZoneName;
    float HealthLoss;
};

class ArPenWearerDamage
{
    float GlobalHealthLoss;
    float GlobalBloodLoss;
    float GlobalShockLoss;
    bool Penetrated;
    string HitAmmoClassName;
    int HitComponentIndex;
    ref array<ref ArPenZoneDamage> Zones = new array<ref ArPenZoneDamage>;
};
