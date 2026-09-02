class ArPenArmorData
{
    bool Enabled;
    float BaseKrupp;
    float BaseArmorHealth;
    float ThicknessMM;
    float MinHealthFactor;
    float HealthExponent;
    float KruppLossPerAbsorbedDamage;
    float ItemDamagePerAbsorbedDamage;
};

class ArPenAmmoData
{
    bool Enabled;
    bool UseLegacyFallback;
    float InitialVelocity;
    float BulletMassKG;
    float BallisticCoefficient;
    float CaliberMM;
    bool IsTracer;
    float BaseDamage;
    float Decibels;
    float PenetrationMultiplier;
    float BloodDamageMultiplier;
    float ShockDamageMultiplier;
    float BluntShockMultiplier;
};

class ArPenConfig
{
    static bool ReadArmor(EntityAI armor, out ArPenArmorData data)
    {
        if (!armor)
            return false;

        if (ArPenArmorProfiles.GetArmorData(armor.GetType(), data))
            return true;

        // Optional direct config enrollment remains supported for armor mods
        // that want to ship their own values.
        string path = "CfgVehicles " + armor.GetType() + " ArPen";
        if (!GetGame().ConfigIsExisting(path + " enabled"))
            return false;
        data = new ArPenArmorData();
        data.Enabled = GetGame().ConfigGetInt(path + " enabled") == 1;
        data.BaseKrupp = ReadFloat(path, "krupp", 0.0);
        data.BaseArmorHealth = ReadFloat(path, "armorHealth", 100.0);
        data.ThicknessMM = ReadFloat(path, "thicknessMM", 0.0);
        data.MinHealthFactor = ReadFloat(path, "minHealthFactor", 0.35);
        data.HealthExponent = ReadFloat(path, "healthExponent", 1.0);
        data.KruppLossPerAbsorbedDamage = ReadFloat(path, "kruppLossPerAbsorbedDamage", 0.0);
        data.ItemDamagePerAbsorbedDamage = ReadFloat(path, "itemDamagePerAbsorbedDamage", 0.0);
        return data.Enabled && data.BaseKrupp > 0.0 && data.ThicknessMM > 0.0;
    }

    static bool ReadAmmo(string ammo, out ArPenAmmoData data)
    {
        if (GetGame().ConfigIsExisting("ArPenSettings useLegacyFallback"))
        {
            if (GetGame().ConfigGetInt("ArPenSettings useLegacyFallback") == 1)
                return false;
        }

        if (ArPenAmmoProfiles.GetAmmoData(ammo, data))
            return true;

        // Optional direct config enrollment for ammunition mods.
        string path = "CfgAmmo " + ammo + " ArPen";
        if (!GetGame().ConfigIsExisting(path + " enabled"))
            return false;
        data = new ArPenAmmoData();
        data.Enabled = GetGame().ConfigGetInt(path + " enabled") == 1;
        data.UseLegacyFallback = GetGame().ConfigGetInt(path + " useLegacyFallback") == 1;
        data.InitialVelocity = ReadFloat(path, "initialVelocity", 0.0);
        data.BulletMassKG = ReadFloat(path, "bulletMassKG", 0.0);
        data.BallisticCoefficient = ReadFloat(path, "ballisticCoefficient", 0.0);
        data.CaliberMM = ReadFloat(path, "caliberMM", 0.0);
        data.IsTracer = GetGame().ConfigGetInt(path + " isTracer") == 1;
        data.BaseDamage = ReadFloat(path, "baseDamage", 0.0);
        data.Decibels = ReadFloat(path, "decibels", 0.0);
        data.PenetrationMultiplier = ReadFloat(path, "penetrationMultiplier", 1.0);
        data.BloodDamageMultiplier = ReadFloat(path, "bloodDamageMultiplier", 0.5);
        data.ShockDamageMultiplier = ReadFloat(path, "shockDamageMultiplier", 1.0);
        data.BluntShockMultiplier = ReadFloat(path, "bluntShockMultiplier", 0.25);
        return data.Enabled && !data.UseLegacyFallback && data.InitialVelocity > 0.0 && data.BulletMassKG > 0.0 && data.CaliberMM > 0.0;
    }

    protected static float ReadFloat(string parentPath, string field, float fallback)
    {
        string path = parentPath + " " + field;
        if (!GetGame().ConfigIsExisting(path))
            return fallback;
        return GetGame().ConfigGetFloat(path);
    }
};
