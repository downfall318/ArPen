class ArPenArmorData
{
    bool Enabled;
    bool IsSoftArmor;
    bool UseSimpleHealthScaling;
    string ArmorLevel;
    float ArmorSchemaHealthDamageMultiplier;
    float ArmorSchemaHealthCapacity;
    float BaseKrupp;
    float BaseArmorHealth;
    float ThicknessMM;
    float MinHealthFactor;
    float HealthExponent;
    float KruppLossPerAbsorbedDamage;
    float ItemDamagePerAbsorbedDamage;
    float ArealDensityKGPerM2;
    float AcousticImpedance;
    float ResistanceConstant;
    float PlateToughnessJ;
    float BaseDeformationMM;
    float MaxDeformationMM;
    float BaseCrackRadiusMM;
    float GlobalCouplingLow;
    float GlobalCouplingSpread;
    float MultiHitSpread;
    string MaterialID;
    string MaterialType;
    float MaterialDensityGCM3;
    float BrinellHardness;
    float SteelDentToughnessConstant;
    float SameHitRadiusMM;
    bool IsHelmet;
    float HelmetShellMassKG;
    float HelmetCurvatureFactor;
    float HelmetEnergyTransmission;
    float HelmetStoppingDistanceMM;
    float HelmetTraumaLimitG;
    float SurfaceAreaCM2;
    float DentDiameterMultiplier;
    float DepthDamageWeight;
    float FirstHitResidualIntegrity;
    float SubsequentHitIntegrityRatio;
    float CeramicDamageExponent;
    float CrackInitiationEnergyFraction;
    float SubfloorDamageScale;
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
    float BluntHeadHealthMultiplier;
    float BluntTorsoHealthMultiplier;
    float BluntHeadShockMultiplier;
    float BluntTorsoShockMultiplier;
    string ThreatLevel;
    float ReferenceThreatEnergyJ;
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
        data.IsSoftArmor = GetGame().ConfigGetInt(path + " isSoftArmor") == 1;
        data.UseSimpleHealthScaling = GetGame().ConfigGetInt(path + " useSimpleHealthScaling") == 1;
        data.BaseKrupp = ReadFloat(path, "krupp", 0.0);
        data.BaseArmorHealth = ReadFloat(path, "armorHealth", 100.0);
        data.ArmorLevel = ReadString(path, "armorLevel", "Unrated");
        data.ArmorSchemaHealthDamageMultiplier = ReadFloat(path, "armorSchemaHealthDamageMultiplier", 1.0);
        data.ArmorSchemaHealthCapacity = ReadFloat(path, "armorSchemaHealthCapacity", data.BaseArmorHealth);
        data.ThicknessMM = ReadFloat(path, "thicknessMM", 0.0);
        data.MinHealthFactor = ReadFloat(path, "minHealthFactor", 0.35);
        data.HealthExponent = ReadFloat(path, "healthExponent", 1.0);
        data.KruppLossPerAbsorbedDamage = ReadFloat(path, "kruppLossPerAbsorbedDamage", 0.0);
        data.ItemDamagePerAbsorbedDamage = ReadFloat(path, "itemDamagePerAbsorbedDamage", 0.0);
        data.ArealDensityKGPerM2 = ReadFloat(path, "arealDensityKGPerM2", 25.9);
        data.AcousticImpedance = ReadFloat(path, "acousticImpedance", 37.5);
        data.ResistanceConstant = ReadFloat(path, "resistanceConstant", 151.6);
        data.PlateToughnessJ = ReadFloat(path, "plateToughnessJ", 1000.0);
        data.BaseDeformationMM = ReadFloat(path, "baseDeformationMM", 4.0);
        data.MaxDeformationMM = ReadFloat(path, "maxDeformationMM", 44.0);
        data.BaseCrackRadiusMM = ReadFloat(path, "baseCrackRadiusMM", 25.0);
        data.GlobalCouplingLow = ReadFloat(path, "globalCouplingLow", 0.05);
        data.GlobalCouplingSpread = ReadFloat(path, "globalCouplingSpread", 0.25);
        data.MultiHitSpread = ReadFloat(path, "multiHitSpread", 1.0);
        data.MaterialID = ReadString(path, "materialID", "silicon_carbide");
        data.MaterialType = ReadString(path, "materialType", "Ceramic");
        data.MaterialDensityGCM3 = ReadFloat(path, "materialDensityGCM3", 3.16);
        data.BrinellHardness = ReadFloat(path, "brinellHardness", 0.0);
        data.SteelDentToughnessConstant = ReadFloat(path, "steelDentToughnessConstant", 2.0);
        data.SameHitRadiusMM = ReadFloat(path, "sameHitRadiusMM", 35.0);
        data.IsHelmet = GetGame().ConfigGetInt(path + " isHelmet") == 1;
        data.HelmetShellMassKG = ReadFloat(path, "helmetShellMassKG", 1.5);
        data.HelmetCurvatureFactor = ReadFloat(path, "helmetCurvatureFactor", 0.60);
        data.HelmetEnergyTransmission = ReadFloat(path, "helmetEnergyTransmission", 0.05);
        data.HelmetStoppingDistanceMM = ReadFloat(path, "helmetStoppingDistanceMM", 20.0);
        data.HelmetTraumaLimitG = ReadFloat(path, "helmetTraumaLimitG", 400.0);
        data.SurfaceAreaCM2 = ReadFloat(path, "surfaceAreaCM2", 2500.0);
        data.DentDiameterMultiplier = ReadFloat(path, "dentDiameterMultiplier", 4.0);
        data.DepthDamageWeight = ReadFloat(path, "depthDamageWeight", 0.0);
        data.FirstHitResidualIntegrity = ReadFloat(path, "firstHitResidualIntegrity", 0.75);
        data.SubsequentHitIntegrityRatio = ReadFloat(path, "subsequentHitIntegrityRatio", 0.333333);
        data.CeramicDamageExponent = ReadFloat(path, "ceramicDamageExponent", 1.75);
        data.CrackInitiationEnergyFraction = ReadFloat(path, "crackInitiationEnergyFraction", 0.18);
        data.SubfloorDamageScale = ReadFloat(path, "subfloorDamageScale", 0.02);

        ArPenMaterialData material;
        if (ArPenMaterialLibrary.Get(data.MaterialID, material))
        {
            data.MaterialType = material.Family;
            data.MaterialDensityGCM3 = material.DensityGCM3;
            data.AcousticImpedance = material.AcousticImpedance;
            data.BrinellHardness = material.BrinellHardness;
            data.PlateToughnessJ = material.PlateToughnessJ;
            data.BaseCrackRadiusMM = material.BaseCrackRadiusMM;
            data.GlobalCouplingLow = material.GlobalCouplingLow;
            data.GlobalCouplingSpread = material.GlobalCouplingSpread;
            data.MultiHitSpread = material.MultiHitSpread;
            data.FirstHitResidualIntegrity = material.FirstHitResidualIntegrity;
            data.SubsequentHitIntegrityRatio = material.SubsequentHitIntegrityRatio;
            data.CeramicDamageExponent = material.CeramicDamageExponent;
            data.CrackInitiationEnergyFraction = material.CrackInitiationEnergyFraction;
            data.SubfloorDamageScale = material.SubfloorDamageScale;
        }
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
        data.BluntHeadHealthMultiplier = ReadFloat(path, "bluntHeadHealthMultiplier", 0.10);
        data.BluntTorsoHealthMultiplier = ReadFloat(path, "bluntTorsoHealthMultiplier", 0.10);
        data.BluntHeadShockMultiplier = ReadFloat(path, "bluntHeadShockMultiplier", 0.35);
        data.BluntTorsoShockMultiplier = ReadFloat(path, "bluntTorsoShockMultiplier", 0.35);
        data.ThreatLevel = ReadString(path, "threatLevel", "Unrated");
        data.ReferenceThreatEnergyJ = ReadFloat(path, "referenceThreatEnergyJ", 0.0);
        return data.Enabled && !data.UseLegacyFallback && data.InitialVelocity > 0.0 && data.BulletMassKG > 0.0 && data.CaliberMM > 0.0;
    }

    protected static string ReadString(string parentPath, string field, string fallback)
    {
        string path = parentPath + " " + field;
        if (!GetGame().ConfigIsExisting(path))
            return fallback;
        string value;
        if (!GetGame().ConfigGetText(path, value))
            return fallback;
        return value;
    }

    protected static float ReadFloat(string parentPath, string field, float fallback)
    {
        string path = parentPath + " " + field;
        if (!GetGame().ConfigIsExisting(path))
            return fallback;
        return GetGame().ConfigGetFloat(path);
    }
};
