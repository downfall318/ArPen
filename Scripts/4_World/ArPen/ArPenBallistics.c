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
    string EffectiveThreatLevel;
    float PlateThresholdJ;
    float RatedPlateThresholdJ;
    float Brittleness;
    float LocalDamage;
    float CrackRadiusMM;
    float GlobalCouplingFactor;
    float MultiHitPenalty;
    float DeformationMM;
    float PreviousDentDepthMM;
    float AddedDentDepthMM;
    float EffectiveThicknessMM;
    float TransmittedAccelerationG;
    bool HelmetTrauma;
    int MaterialHitCount;
    float PreviousTileIntegrity;
    float ResultingTileIntegrity;
    float AffectedSurfaceAreaCM2;
    float HealthPerSurfaceArea;
    float IntegrityHealthLoss;
    float IntegrityKruppLoss;
    float EnergyFraction;
    float CrackDamageScale;
    float DamageFractionOfRemaining;
    float DepthRatio;
    float TransferredEnergyFraction;
    float AddedChannelVolumeMM3;
    float AddedDentVolumeMM3;
    float AddedMetalLossVolumeMM3;
    float CumulativeMetalLossVolumeMM3;
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

    protected static float CombinedSeverity(float energySeverity, float depthFraction, float depthWeight)
    {
        float energy = Math.Clamp(energySeverity, 0.0, 1.0);
        float depth = Math.Clamp(depthFraction, 0.0, 1.0);
        float weight = Math.Max(depthWeight, 0.0);
        if (weight <= 0.0 || depth <= 0.0)
            return energy;
        return 1.0 - ((1.0 - energy) * Math.Pow(1.0 - depth, weight));
    }

    protected static float CeramicDamageFraction(float health01, float severity)
    {
        float health = Math.Clamp(health01, 0.0, 1.0);
        float fullSeverityDamage;
        if (health <= 0.25)
            fullSeverityDamage = health;
        else if (health <= 0.75)
            fullSeverityDamage = 0.25 + ((health - 0.25) * 0.5);
        else
            fullSeverityDamage = 0.5 - (health - 0.75);
        return Math.Min(health, Math.Max(fullSeverityDamage, 0.0) * Math.Pow(Math.Clamp(severity, 0.0, 1.0), 1.35));
    }

    static ArPenHitResult Calculate(ArPenAmmoData ammoData, ArPenArmorData armorData, EntityAI armor, float speedCoef, vector modelPosition)
    {
        ArPenHitResult result = new ArPenHitResult();
        result.Armor = armor;
        result.ImpactVelocity = ammoData.InitialVelocity * Math.Max(speedCoef, 0.0);
        result.ImpactEnergyJ = 0.5 * ammoData.BulletMassKG * result.ImpactVelocity * result.ImpactVelocity;
        result.EffectiveThreatLevel = ArPenAmmoProfiles.GetEffectiveThreatLevel(ammoData, result.ImpactEnergyJ);
        ItemBase armorItem = ItemBase.Cast(armor);
        if (!armorItem)
            return result;

        result.ItemHealth = armorItem.GetHealth("", "Health");
        result.ItemMaxHealth = armorItem.GetMaxHealth("", "Health");
        result.CurrentArmorHealth = armorItem.ArPen_GetCurrentArmorHealth(armorData);
        result.BaseArmorHealth = Math.Max(armorData.BaseArmorHealth, 0.001);
        result.ArmorHealth01 = Math.Min(Math.Clamp(armorItem.GetHealth01("", "Health"), 0.0, 1.0), Math.Clamp(result.CurrentArmorHealth / result.BaseArmorHealth, 0.0, 1.0));
        result.CurrentKrupp = armorData.BaseKrupp;
        if (armorData.MaterialType == "Steel")
            result.EffectiveKrupp = armorData.BaseKrupp;
        else
            result.EffectiveKrupp = armorData.BaseKrupp * Math.Sqrt(result.ArmorHealth01);

        float panelAreaMM2 = Math.Max(armorData.SurfaceAreaCM2, 1.0) * 100.0;
        result.EffectiveThicknessMM = armorData.ThicknessMM;
        if (armorData.MaterialType == "Steel")
            result.EffectiveThicknessMM = Math.Max(0.01, armorData.ThicknessMM - (armorItem.ArPen_GetDentVolumeMM3(armorData) / panelAreaMM2));
        if (result.EffectiveKrupp <= 0.0 || ammoData.CaliberMM <= 0.0)
            return result;

        result.PenetrationDistanceMM = (result.ImpactVelocity * Math.Sqrt(ammoData.BulletMassKG)) / (result.EffectiveKrupp * Math.Sqrt(ammoData.CaliberMM));
        result.PenetrationDistanceMM = result.PenetrationDistanceMM * ammoData.PenetrationMultiplier * 1000.0;
        result.DepthRatio = result.PenetrationDistanceMM / Math.Max(result.EffectiveThicknessMM, 0.001);
        float depthFraction = Math.Clamp(result.DepthRatio, 0.0, 1.0);
        if (result.DepthRatio >= 1.0 && result.PenetrationDistanceMM > 0.0)
        {
            result.ExitVelocity = result.ImpactVelocity * Math.Sqrt(Math.Max(0.0, 1.0 - (1.0 / result.DepthRatio)));
            result.Penetrated = result.ExitVelocity > 50.0;
        }

        float residualEnergyFraction = 0.0;
        if (result.Penetrated && result.ImpactVelocity > 0.0)
            residualEnergyFraction = Math.Pow(result.ExitVelocity / result.ImpactVelocity, 2.0);
        if (result.Penetrated)
            result.TransferredEnergyFraction = Math.Clamp(1.0 - residualEnergyFraction, 0.0, 1.0);
        else
            result.TransferredEnergyFraction = 1.0;

        float energyRaw = (ammoData.BaseDamage / Math.Max(ammoData.InitialVelocity, 1.0)) * 5.0 * result.ImpactVelocity * Math.Pow(ammoData.PenetrationMultiplier, 4.0) * 3.0;
        result.EnergyFraction = Math.Clamp(Math.Min(energyRaw, result.BaseArmorHealth) / result.BaseArmorHealth, 0.0, 1.0);
        float severity = CombinedSeverity(result.EnergyFraction, depthFraction, armorData.DepthDamageWeight);

        if (armorData.MaterialType == "Ceramic")
        {
            result.DamageFractionOfRemaining = CeramicDamageFraction(result.ArmorHealth01, severity);
            result.ArmorDamage = Math.Min(result.CurrentArmorHealth, result.DamageFractionOfRemaining * result.BaseArmorHealth);
        }
        else if (armorData.MaterialType == "Steel")
        {
            float panelVolume = panelAreaMM2 * armorData.ThicknessMM;
            float failureVolume = Math.Max(panelVolume * 0.25, 0.001);
            float projectileArea = Math.PI * Math.Pow(Math.Max(ammoData.CaliberMM, 0.01) * 0.5, 2.0);
            float traversedDepth = Math.Min(Math.Max(result.PenetrationDistanceMM, 0.0), result.EffectiveThicknessMM);
            result.AddedChannelVolumeMM3 = projectileArea * traversedDepth;
            float spread = Math.Max(armorData.DentDiameterMultiplier, 1.0);
            float dentAnnulusArea = Math.Max(0.0, (projectileArea * spread * spread) - projectileArea);
            float dentDepth = Math.Min(result.EffectiveThicknessMM * 0.25, traversedDepth * result.TransferredEnergyFraction * 0.25);
            result.AddedDentVolumeMM3 = dentAnnulusArea * dentDepth;
            result.AddedMetalLossVolumeMM3 = result.AddedChannelVolumeMM3 + result.AddedDentVolumeMM3;

            float previousLoss = armorItem.ArPen_GetMetalLossVolumeMM3(armorData);
            result.CumulativeMetalLossVolumeMM3 = Math.Min(failureVolume, previousLoss + result.AddedMetalLossVolumeMM3);
            float healthAfter = result.BaseArmorHealth * Math.Max(0.0, 1.0 - (result.CumulativeMetalLossVolumeMM3 / failureVolume));
            result.ArmorDamage = Math.Min(result.CurrentArmorHealth, Math.Max(0.0, result.CurrentArmorHealth - healthAfter));
        }
        else
        {
            float speed01 = Math.Max(0.0, result.ImpactVelocity / Math.Max(ammoData.InitialVelocity, 1.0));
            float rawDamage = result.BaseArmorHealth * (Math.Max(ammoData.BaseDamage, 0.0) / 100.0) * speed01 * speed01 * (1.0 + ((1.0 - result.ArmorHealth01) * 0.75));
            float penetrationFactor;
            if (result.Penetrated)
                penetrationFactor = 0.18 + (0.82 * Math.Pow(1.0 / Math.Max(result.DepthRatio, 1.0), 1.35));
            else
                penetrationFactor = 0.12 + (0.88 * Math.Pow(depthFraction, 1.8));
            float depthBoost = 1.0;
            if (result.EnergyFraction > 0.000001)
                depthBoost = Math.Max(0.25, severity / result.EnergyFraction);
            result.ArmorDamage = Math.Min(result.CurrentArmorHealth, Math.Min(result.BaseArmorHealth, rawDamage * penetrationFactor * depthBoost));
        }

        if (result.CurrentArmorHealth - result.ArmorDamage <= Math.Max(0.5, result.BaseArmorHealth * 0.002))
            result.ArmorDamage = result.CurrentArmorHealth;
        CalculateMaterialResponse(result, armorData);
        if (armorData.IsHelmet)
            CalculateHelmetResponse(result, armorData);
        result.DamageMultiplier = 0.0;
        if (result.Penetrated)
            result.DamageMultiplier = Math.Clamp(result.ExitVelocity / Math.Max(ammoData.InitialVelocity, 0.001), 0.0, 1.0);
        return result;
    }

    protected static void CalculateMaterialResponse(ArPenHitResult result, ArPenArmorData armorData)
    {
        result.RatedPlateThresholdJ = armorData.ArealDensityKGPerM2 * armorData.ResistanceConstant;
        float healthStrength = Math.Sqrt(result.ArmorHealth01);
        if (armorData.MaterialType == "Steel")
            healthStrength = 1.0;
        result.PlateThresholdJ = result.RatedPlateThresholdJ * healthStrength;
        if (armorData.IsHelmet)
            result.PlateThresholdJ = result.PlateThresholdJ / Math.Clamp(armorData.HelmetCurvatureFactor, 0.1, 1.0);
        result.LocalDamage = (result.ImpactEnergyJ - result.PlateThresholdJ) / Math.Max(armorData.PlateToughnessJ, 1.0);
        if (armorData.MaterialType == "Steel")
            result.Brittleness = CalculateSteelBrittleness(armorData.BrinellHardness);
        else if (armorData.MaterialType == "Ceramic")
            result.Brittleness = Math.Clamp((armorData.AcousticImpedance - 33.5) / (51.3 - 33.5), 0.0, 1.0);
        else
            result.Brittleness = 0.05;
        if (!result.Penetrated)
        {
            float load = Math.Clamp(result.ImpactEnergyJ / Math.Max(result.PlateThresholdJ, 1.0), 0.0, 1.0);
            result.DeformationMM = Math.Min(armorData.BaseDeformationMM * load * load * (1.0 + Math.Max(result.LocalDamage, 0.0)), armorData.MaxDeformationMM);
        }
    }

    protected static float CalculateSteelBrittleness(float hardness)
    {
        if (hardness <= 550.0)
            return 0.10 + (0.10 * Math.Clamp((hardness - 477.0) / 73.0, 0.0, 1.0));
        return 0.20 + (0.80 * Math.Clamp((hardness - 550.0) / 100.0, 0.0, 1.0));
    }

    protected static void CalculateHelmetResponse(ArPenHitResult result, ArPenArmorData armorData)
    {
        float curvature = Math.Clamp(armorData.HelmetCurvatureFactor, 0.1, 1.0);
        float transmittedEnergy = result.ImpactEnergyJ * Math.Clamp(armorData.HelmetEnergyTransmission, 0.0, 1.0) * curvature;
        result.TransmittedAccelerationG = transmittedEnergy / (Math.Max(armorData.HelmetShellMassKG, 0.1) * Math.Max(armorData.HelmetStoppingDistanceMM, 1.0) * 0.001 * 9.80665);
        result.HelmetTrauma = result.TransmittedAccelerationG > armorData.HelmetTraumaLimitG;
        // Diagnostic only: the energy threshold no longer vetoes a depth-predicted perforation.
    }
};
