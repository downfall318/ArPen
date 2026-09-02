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

    static ArPenHitResult Calculate(ArPenAmmoData ammoData, ArPenArmorData armorData, EntityAI armor, float speedCoef, vector modelPosition)
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
        result.EffectiveThicknessMM = armorData.ThicknessMM;
        if (armorData.MaterialType == "Steel")
        {
            result.PreviousDentDepthMM = armorItem.ArPen_GetLocalDentDepth(modelPosition, armorData.SameHitRadiusMM);
            result.EffectiveThicknessMM = Math.Max(armorData.ThicknessMM - result.PreviousDentDepthMM, 0.1);
        }
        float itemHealth01 = Math.Clamp(armorItem.GetHealth01("", "Health"), 0.0, 1.0);
        float trackedHealth01 = Math.Clamp(result.CurrentArmorHealth / Math.Max(result.BaseArmorHealth, 1.0), 0.0, 1.0);
        result.ArmorHealth01 = Math.Min(itemHealth01, trackedHealth01);
        result.CurrentKrupp = armorItem.ArPen_GetCurrentKrupp(armorData);
        result.PreviousTileIntegrity = 1.0;
        if (armorData.MaterialType == "Ceramic")
        {
            result.MaterialHitCount = armorItem.ArPen_GetCeramicTileHitCount(modelPosition, armorData.TileSurfaceAreaCM2) + 1;
            result.PreviousTileIntegrity = armorItem.ArPen_GetCeramicTileIntegrity(modelPosition, armorData.TileSurfaceAreaCM2);
        }
        float healthFactor = Math.Pow(result.ArmorHealth01, armorData.HealthExponent);
        healthFactor = armorData.MinHealthFactor + ((1.0 - armorData.MinHealthFactor) * healthFactor);
        result.EffectiveKrupp = result.CurrentKrupp * healthFactor;
        if (armorData.UseSimpleHealthScaling)
            result.EffectiveKrupp = armorData.BaseKrupp * Math.Clamp(armorData.ArmorSchemaHealthProtection, 0.0, 1.0);
        if (armorData.MaterialType == "Ceramic" && !armorData.UseSimpleHealthScaling)
        {
            // Local ceramic residual strength is anchored directly to the
            // tile curve, preserving I2 / I1 = 1/3 exactly.
            result.EffectiveKrupp = armorData.BaseKrupp * result.PreviousTileIntegrity;
        }
        if (result.EffectiveKrupp <= 0.0 || ammoData.CaliberMM <= 0.0)
            return result;

        result.PenetrationDistanceMM = (result.ImpactVelocity * Math.Sqrt(ammoData.BulletMassKG)) / (result.EffectiveKrupp * Math.Sqrt(ammoData.CaliberMM));
        result.PenetrationDistanceMM = result.PenetrationDistanceMM * ammoData.PenetrationMultiplier;
        // Unity B is a world-distance value; armor profiles are explicitly mm.
        result.PenetrationDistanceMM = result.PenetrationDistanceMM * 1000.0;

        float traveledMM = Math.Min(result.PenetrationDistanceMM, result.EffectiveThicknessMM);
        float velocityScale = Math.Clamp(traveledMM / result.EffectiveThicknessMM, 0.0, 1.0);
        float damagePerVelocity = ammoData.BaseDamage / ammoData.InitialVelocity;
        float baseArmorDamage = damagePerVelocity * 5.0 * result.ImpactVelocity * Math.Pow(ammoData.PenetrationMultiplier, 4.0);
        result.ArmorDamage = Math.Min(baseArmorDamage * velocityScale * 3.0 / Math.Max(result.ArmorHealth01, 0.01), result.CurrentArmorHealth);

        if (result.EffectiveThicknessMM <= result.PenetrationDistanceMM)
        {
            float energyRemainingRatio = 1.0 - (result.EffectiveThicknessMM / result.PenetrationDistanceMM);
            result.ExitVelocity = result.ImpactVelocity * Math.Sqrt(Math.Max(energyRemainingRatio, 0.0));
            result.Penetrated = result.ExitVelocity > 50.0;
        }

        CalculateMaterialResponse(result, armorData);
        if (armorData.MaterialType == "Ceramic")
            CalculateCeramicIntegrityLoss(result, armorData);

        if (armorData.IsHelmet)
            CalculateHelmetResponse(result, armorData);

        if (result.Penetrated)
            result.DamageMultiplier = Math.Clamp(result.ExitVelocity / ammoData.InitialVelocity, 0.0, 1.0);
        else
            result.DamageMultiplier = 0.0;

        return result;
    }

    protected static void CalculateMaterialResponse(ArPenHitResult result, ArPenArmorData armorData)
    {
        result.RatedPlateThresholdJ = armorData.ArealDensityKGPerM2 * armorData.ResistanceConstant;
        result.PlateThresholdJ = result.RatedPlateThresholdJ;
        if (armorData.MaterialType == "Ceramic")
            result.PlateThresholdJ = result.PlateThresholdJ * result.PreviousTileIntegrity;
        if (armorData.IsHelmet)
            result.PlateThresholdJ = result.PlateThresholdJ / Math.Clamp(armorData.HelmetCurvatureFactor, 0.1, 1.0);

        float toughness = Math.Max(armorData.PlateToughnessJ, 1.0);
        result.LocalDamage = (result.ImpactEnergyJ - result.PlateThresholdJ) / toughness;

        if (armorData.MaterialType == "Steel")
        {
            result.Brittleness = CalculateSteelBrittleness(armorData.BrinellHardness);
            result.CrackRadiusMM = 0.0;
            result.GlobalCouplingFactor = armorData.GlobalCouplingLow + (result.Brittleness * armorData.GlobalCouplingSpread);
            result.MultiHitPenalty = 1.0 + (result.Brittleness * armorData.MultiHitSpread);
            if (result.PreviousDentDepthMM > 0.0)
                result.MultiHitPenalty = result.MultiHitPenalty + (result.PreviousDentDepthMM / Math.Max(armorData.ThicknessMM, 0.1));

            if (!result.Penetrated)
            {
                float hardness = Math.Max(armorData.BrinellHardness, 1.0);
                float dentConstant = Math.Max(armorData.SteelDentToughnessConstant, 0.01);
                result.AddedDentDepthMM = result.ImpactEnergyJ / (hardness * dentConstant);
                result.AddedDentDepthMM = Math.Min(result.AddedDentDepthMM, result.EffectiveThicknessMM);
                result.DeformationMM = result.PreviousDentDepthMM + result.AddedDentDepthMM;
            }
            return;
        }

        if (armorData.MaterialType == "Ceramic")
        {
            result.Brittleness = Math.Clamp((armorData.AcousticImpedance - 33.5) / (51.3 - 33.5), 0.0, 1.0);
            result.CrackRadiusMM = armorData.BaseCrackRadiusMM * (1.0 + result.Brittleness);
            result.GlobalCouplingFactor = armorData.GlobalCouplingLow + (result.Brittleness * armorData.GlobalCouplingSpread);
            result.MultiHitPenalty = 1.0 + (result.Brittleness * armorData.MultiHitSpread);
        }
        else
        {
            // Aramid/UHMWPE-style composite shells deform without ceramic
            // crack propagation or steel's permanent local thickness loss.
            result.Brittleness = 0.05;
            result.CrackRadiusMM = 0.0;
            result.GlobalCouplingFactor = 0.05;
            result.MultiHitPenalty = 1.05;
        }

        if (!result.Penetrated)
        {
            float thresholdLoad = result.ImpactEnergyJ / Math.Max(result.PlateThresholdJ, 1.0);
            float elasticLoad = Math.Clamp(thresholdLoad, 0.0, 1.0);
            float overload = Math.Max(result.LocalDamage, 0.0);
            float deformationScale = elasticLoad * elasticLoad * (1.0 + overload);
            result.DeformationMM = Math.Min(armorData.BaseDeformationMM * deformationScale, armorData.MaxDeformationMM);
        }
    }

    protected static void CalculateCeramicIntegrityLoss(ArPenHitResult result, ArPenArmorData armorData)
    {
        float firstHitIntegrity = Math.Clamp(armorData.FirstHitResidualIntegrity, 0.0, 1.0);
        float secondHitResidualRatio = Math.Clamp(armorData.SubsequentHitIntegrityRatio, 0.0, 1.0);
        float exponent = Math.Max(armorData.CeramicDamageExponent, 1.0);
        float crackFloor = Math.Clamp(armorData.CrackInitiationEnergyFraction, 0.01, 0.95);
        float subfloorScale = Math.Clamp(armorData.SubfloorDamageScale, 0.0, 1.0);

        // Normalize against the intact rated threshold, not the already-damaged
        // tile threshold. Range loss is naturally included in ImpactEnergyJ.
        result.EnergyFraction = result.ImpactEnergyJ / Math.Max(result.RatedPlateThresholdJ, 1.0);

        if (result.EnergyFraction <= crackFloor)
        {
            float belowFloor = Math.Clamp(result.EnergyFraction / crackFloor, 0.0, 1.0);
            result.CrackDamageScale = subfloorScale * Math.Pow(belowFloor, exponent);
        }
        else
        {
            float aboveFloor = Math.Clamp((result.EnergyFraction - crackFloor) / (1.0 - crackFloor), 0.0, 1.0);
            result.CrackDamageScale = subfloorScale + ((1.0 - subfloorScale) * Math.Pow(aboveFloor, exponent));
        }

        float firstHitDamage = 1.0 - firstHitIntegrity;
        float establishedCrackDamage = 1.0 - secondHitResidualRatio;
        float fractureNetwork = Math.Clamp((1.0 - result.PreviousTileIntegrity) / Math.Max(firstHitDamage, 0.001), 0.0, 1.0);
        float calibratedRatedDamage = firstHitDamage + ((establishedCrackDamage - firstHitDamage) * fractureNetwork);

        result.DamageFractionOfRemaining = Math.Clamp(calibratedRatedDamage * result.CrackDamageScale, 0.0, 1.0);
        result.ResultingTileIntegrity = result.PreviousTileIntegrity * (1.0 - result.DamageFractionOfRemaining);
        result.AffectedSurfaceAreaCM2 = Math.Min(armorData.TileSurfaceAreaCM2, armorData.SurfaceAreaCM2);
        result.HealthPerSurfaceArea = result.BaseArmorHealth / Math.Max(armorData.SurfaceAreaCM2, 1.0);

        float newlyLostIntegrity = Math.Max(result.PreviousTileIntegrity - result.ResultingTileIntegrity, 0.0);
        result.IntegrityHealthLoss = newlyLostIntegrity * result.AffectedSurfaceAreaCM2 * result.HealthPerSurfaceArea;

        // Krupp loss follows the same fraction of the armor's total integrity
        // removed by this tile event. It is applied directly after calculation.
        float totalHealthFractionLost = result.IntegrityHealthLoss / Math.Max(result.BaseArmorHealth, 1.0);
        result.IntegrityKruppLoss = armorData.BaseKrupp * totalHealthFractionLost;
    }

    protected static float CalculateSteelBrittleness(float brinellHardness)
    {
        if (brinellHardness <= 550.0)
        {
            float ar500Range = Math.Clamp((brinellHardness - 477.0) / (550.0 - 477.0), 0.0, 1.0);
            return 0.10 + (0.10 * ar500Range);
        }

        // Brittleness rises rapidly above the AR500 range.
        float highHardnessRange = Math.Clamp((brinellHardness - 550.0) / 100.0, 0.0, 1.0);
        return 0.20 + (0.80 * highHardnessRange);
    }

    protected static void CalculateHelmetResponse(ArPenHitResult result, ArPenArmorData armorData)
    {
        // CurvatureFactor below one represents load spreading: it raises the
        // penetration threshold and reduces the energy delivered to the head.
        float curvature = Math.Clamp(armorData.HelmetCurvatureFactor, 0.1, 1.0);
        float shellMass = Math.Max(armorData.HelmetShellMassKG, 0.1);
        float stoppingDistanceM = Math.Max(armorData.HelmetStoppingDistanceMM, 1.0) * 0.001;
        float transmittedEnergy = result.ImpactEnergyJ * Math.Clamp(armorData.HelmetEnergyTransmission, 0.0, 1.0) * curvature;
        result.TransmittedAccelerationG = transmittedEnergy / (shellMass * stoppingDistanceM * 9.80665);
        result.HelmetTrauma = result.TransmittedAccelerationG > armorData.HelmetTraumaLimitG;

        // Helmets use the energy threshold rather than flat-plate thickness
        // alone. Preserve the Krupp exit calculation when both checks agree.
        if (result.ImpactEnergyJ <= result.PlateThresholdJ)
        {
            result.Penetrated = false;
            result.ExitVelocity = 0.0;
        }
    }
};
