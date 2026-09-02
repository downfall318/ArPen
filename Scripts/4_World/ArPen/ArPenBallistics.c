class ArPenHitResult
{
    EntityAI Armor;
    float ArmorHealth01;
    float ItemHealth;
    float ItemMaxHealth;
    float CurrentArmorHealth;
    float BaseArmorHealth;
    float CurrentKrupp;
    float EffectiveKrupp;
    float ImpactVelocity;
    float ExitVelocity;
    float PenetrationDistanceMM;
    float ArmorDamage;
    float ImpactEnergyJ;
    float PlateThresholdJ;
    float Brittleness;
    float LocalDamage;
    float CrackRadiusMM;
    float GlobalCouplingFactor;
    float MultiHitPenalty;
    float DeformationMM;
    bool Penetrated;
    float DamageMultiplier;
};

class ArPenBallistics
{
    static EntityAI FindArmor(PlayerBase player, string damageZone)
    {
        if (!player)
            return NULL;
        if (damageZone == "Head" || damageZone == "Brain")
            return player.FindAttachmentBySlotName("Headgear");
        if (damageZone == "Torso" || damageZone == "LeftArm" || damageZone == "RightArm")
            return player.FindAttachmentBySlotName("Vest");
        return NULL;
    }

    static ArPenHitResult Calculate(ArPenAmmoData ammoData, ArPenArmorData armorData, EntityAI armor, float speedCoef)
    {
        ArPenHitResult result = new ArPenHitResult();
        result.Armor = armor;
        result.ImpactVelocity = ammoData.InitialVelocity * Math.Max(speedCoef, 0.0);
        result.ImpactEnergyJ = 0.5 * ammoData.BulletMassKG * result.ImpactVelocity * result.ImpactVelocity;
        result.DamageMultiplier = Math.Clamp(result.ImpactVelocity / ammoData.InitialVelocity, 0.0, 1.0);
        ItemBase armorItem = ItemBase.Cast(armor);
        if (!armorItem)
            return result;
        result.ItemHealth = armorItem.GetHealth("", "Health");
        result.ItemMaxHealth = armorItem.GetMaxHealth("", "Health");
        result.CurrentArmorHealth = armorItem.ArPen_GetCurrentArmorHealth(armorData);
        result.BaseArmorHealth = armorData.BaseArmorHealth;
        float itemHealth01 = Math.Clamp(armorItem.GetHealth01("", "Health"), 0.0, 1.0);
        float trackedHealth01 = Math.Clamp(result.CurrentArmorHealth / Math.Max(result.BaseArmorHealth, 1.0), 0.0, 1.0);
        result.ArmorHealth01 = Math.Min(itemHealth01, trackedHealth01);
        result.CurrentKrupp = armorItem.ArPen_GetCurrentKrupp(armorData);
        float healthFactor = Math.Pow(result.ArmorHealth01, armorData.HealthExponent);
        healthFactor = armorData.MinHealthFactor + ((1.0 - armorData.MinHealthFactor) * healthFactor);
        result.EffectiveKrupp = result.CurrentKrupp * healthFactor;
        if (result.EffectiveKrupp <= 0.0 || ammoData.CaliberMM <= 0.0)
            return result;

        result.PenetrationDistanceMM = (result.ImpactVelocity * Math.Sqrt(ammoData.BulletMassKG)) / (result.EffectiveKrupp * Math.Sqrt(ammoData.CaliberMM));
        result.PenetrationDistanceMM = result.PenetrationDistanceMM * ammoData.PenetrationMultiplier;
        // Unity B is a world-distance value; armor profiles are explicitly mm.
        result.PenetrationDistanceMM = result.PenetrationDistanceMM * 1000.0;

        float traveledMM = Math.Min(result.PenetrationDistanceMM, armorData.ThicknessMM);
        float velocityScale = Math.Clamp(traveledMM / armorData.ThicknessMM, 0.0, 1.0);
        float damagePerVelocity = ammoData.BaseDamage / ammoData.InitialVelocity;
        float baseArmorDamage = damagePerVelocity * 5.0 * result.ImpactVelocity * Math.Pow(ammoData.PenetrationMultiplier, 4.0);
        result.ArmorDamage = Math.Min(baseArmorDamage * velocityScale * 3.0 / Math.Max(result.ArmorHealth01, 0.01), result.CurrentArmorHealth);

        if (armorData.ThicknessMM <= result.PenetrationDistanceMM)
        {
            float energyRemainingRatio = 1.0 - (armorData.ThicknessMM / result.PenetrationDistanceMM);
            result.ExitVelocity = result.ImpactVelocity * Math.Sqrt(Math.Max(energyRemainingRatio, 0.0));
            result.Penetrated = result.ExitVelocity > 50.0;
        }

        if (!result.Penetrated)
            CalculateStoppedRoundDeformation(result, armorData);

        if (result.Penetrated)
            result.DamageMultiplier = Math.Clamp(result.ExitVelocity / ammoData.InitialVelocity, 0.0, 1.0);
        else
            result.DamageMultiplier = 0.0;

        return result;
    }

    protected static void CalculateStoppedRoundDeformation(ArPenHitResult result, ArPenArmorData armorData)
    {
        result.PlateThresholdJ = armorData.ArealDensityKGPerM2 * armorData.ResistanceConstant;
        result.Brittleness = Math.Clamp((armorData.AcousticImpedance - 33.5) / (51.3 - 33.5), 0.0, 1.0);

        float toughness = Math.Max(armorData.PlateToughnessJ, 1.0);
        result.LocalDamage = (result.ImpactEnergyJ - result.PlateThresholdJ) / toughness;
        result.CrackRadiusMM = armorData.BaseCrackRadiusMM * (1.0 + result.Brittleness);
        result.GlobalCouplingFactor = armorData.GlobalCouplingLow + (result.Brittleness * armorData.GlobalCouplingSpread);
        result.MultiHitPenalty = 1.0 + (result.Brittleness * armorData.MultiHitSpread);

        // Deformation is computed for stopped rounds only. It is intentionally
        // not applied to the item's health or quality in this implementation.
        float thresholdLoad = result.ImpactEnergyJ / Math.Max(result.PlateThresholdJ, 1.0);
        float elasticLoad = Math.Clamp(thresholdLoad, 0.0, 1.0);
        float overload = Math.Max(result.LocalDamage, 0.0);
        float deformationScale = elasticLoad * elasticLoad * (1.0 + overload);
        result.DeformationMM = Math.Min(armorData.BaseDeformationMM * deformationScale, armorData.MaxDeformationMM);
    }
};
