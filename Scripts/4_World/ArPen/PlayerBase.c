modded class PlayerBase
{
    protected void ArPen_ApplyCustomDamage(float healthDamage, float bloodDamage, float shockDamage)
    {
        if (healthDamage > 0.0)
            DecreaseHealth("", "Health", healthDamage);
        if (bloodDamage > 0.0)
            DecreaseHealth("", "Blood", bloodDamage);
        if (shockDamage > 0.0)
            DecreaseHealth("", "Shock", shockDamage);
    }

    protected float ArPen_RemoveVanillaArmorReduction(float damage, EntityAI armor, string damageChannel)
    {
        if (!armor || damage <= 0.0)
            return damage;

        string multiplierPath = "CfgVehicles " + armor.GetType() + " DamageSystem GlobalArmor Projectile " + damageChannel + " damage";
        if (!GetGame().ConfigIsExisting(multiplierPath))
            return damage;

        float vanillaArmorMultiplier = GetGame().ConfigGetFloat(multiplierPath);
        if (vanillaArmorMultiplier <= 0.0001 || vanillaArmorMultiplier >= 1.0)
            return damage;

        return damage / vanillaArmorMultiplier;
    }

    protected float ArPen_GetStoppedHealthZoneMultiplier(string damageZone)
    {
        if (damageZone == "Head" || damageZone == "Brain")
            return 2.0;
        if (damageZone == "LeftArm" || damageZone == "RightArm" || damageZone == "LeftHand" || damageZone == "RightHand")
            return 0.1;
        if (damageZone == "LeftLeg" || damageZone == "RightLeg")
            return 0.3;
        if (damageZone == "LeftFoot" || damageZone == "RightFoot")
            return 0.12;
        return 1.0;
    }

    protected float ArPen_GetStoppedShockZoneMultiplier(string damageZone)
    {
        if (damageZone == "Head" || damageZone == "Brain")
            return 3.0;
        if (damageZone == "LeftArm" || damageZone == "RightArm" || damageZone == "LeftLeg" || damageZone == "RightLeg")
            return 0.33;
        if (damageZone == "LeftHand" || damageZone == "RightHand" || damageZone == "LeftFoot" || damageZone == "RightFoot")
            return 0.1;
        return 1.0;
    }

    override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
    {
        ArPenAmmoData ammoData;

        // Only explicitly enrolled ammo suppresses the vanilla damage event.
        if (!ArPenConfig.ReadAmmo(ammo, ammoData))
            return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

        float impactVelocity = ammoData.InitialVelocity * Math.Max(speedCoef, 0.0);
        float impactEnergyJ = 0.5 * ammoData.BulletMassKG * impactVelocity * impactVelocity;
        string effectiveThreatLevel = ArPenAmmoProfiles.GetEffectiveThreatLevel(ammoData, impactEnergyJ);

        float healthDamage = damageResult.GetDamage(dmgZone, "Health");
        float bloodDamage = damageResult.GetDamage(dmgZone, "Blood");
        float shockDamage = damageResult.GetDamage(dmgZone, "Shock");

        string componentZone = GetDamageZoneNameByComponentIndex(component);

        TStringArray componentNames = new TStringArray;
        int componentNameResult = GetActionComponentNameList(component, componentNames, "fire");

        vector componentPos = GetActionComponentPosition(component, "fire");
        vector targetPos = GetPosition();

        string componentNamesText = "";

        foreach (string componentName : componentNames)
        {
            if (componentNamesText != "")
                componentNamesText = componentNamesText + ",";

            componentNamesText = componentNamesText + componentName;
        }

        if (componentNamesText == "")
            componentNamesText = "(none)";

        float range = -1;
        string sourceType = "NULL";
        string attackerType = "NULL";

        if (source)
        {
            sourceType = source.GetType();

            Man rootPlayer = source.GetHierarchyRootPlayer();

            if (rootPlayer)
            {
                PlayerBase attacker = PlayerBase.Cast(rootPlayer);

                if (attacker)
                {
                    attackerType = attacker.GetType();
                    range = vector.Distance(attacker.GetPosition(), targetPos);
                }
            }
        }

        Print("[ArPen] ========================================");
        Print("[ArPen] DAMAGE EVENT");
        Print("[ArPen] Ammo = " + ammo);
        Print("[ArPen] MuzzleThreatLevel = " + ammoData.ThreatLevel);
        Print("[ArPen] EffectiveThreatLevel = " + effectiveThreatLevel);
        Print("[ArPen] ReferenceThreatEnergyJ = " + ammoData.ReferenceThreatEnergyJ.ToString());
        Print("[ArPen] Zone = " + dmgZone);
        Print("[ArPen] Component = " + component.ToString());
        Print("[ArPen] ComponentZone = " + componentZone);
        Print("[ArPen] FireComponentResult = " + componentNameResult.ToString());
        Print("[ArPen] FireComponentNames = " + componentNamesText);
        Print("[ArPen] FireComponentPosition = " + componentPos.ToString());
        Print("[ArPen] ModelPos = " + modelPos.ToString());
        Print("[ArPen] HealthDamage = " + healthDamage.ToString());
        Print("[ArPen] BloodDamage = " + bloodDamage.ToString());
        Print("[ArPen] ShockDamage = " + shockDamage.ToString());
        Print("[ArPen] SpeedCoef = " + speedCoef.ToString());
        Print("[ArPen] SourceType = " + sourceType);
        Print("[ArPen] AttackerType = " + attackerType);

        if (range >= 0)
            Print("[ArPen] Range = " + range.ToString());

        Print("[ArPen] ========================================");

        EntityAI armor = ArPenBallistics.FindArmor(this, dmgZone);
        ArPenArmorData armorData;
        ArPenHitResult hitResult;
        bool enrolledArmor;
        float postArmorHealth;
        float postItemHealth;
        float postKrupp;
        bool hasArmorProfile = ArPenConfig.ReadArmor(armor, armorData);

        // Equipped armor that is not explicitly enrolled as hard ballistic
        // armor—including soft armor and protective/cosmetic headgear—uses
        // DayZ's original GlobalArmor result.
        if (armor && !hasArmorProfile)
        {
            Print("[ArPen] ARMOR HIT | NATIVE FALLBACK | " + armor.GetType());
            return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
        }

        // Kevlar and other soft armor deliberately retain DayZ's native
        // GlobalArmor calculation and damage application.
        if (hasArmorProfile && armorData.IsSoftArmor)
        {
            Print("[ArPen] ARMOR HIT | SOFT ARMOR FALLBACK | " + armor.GetType());
            return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
        }

        if (hasArmorProfile)
        {
            enrolledArmor = true;
            hitResult = ArPenBallistics.Calculate(ammoData, armorData, armor, speedCoef, modelPos);
            ItemBase armorItem = ItemBase.Cast(armor);
            if (armorItem)
            {
                armorItem.ArPen_ApplyDamage(armorData, hitResult.ArmorDamage, hitResult.AddedMetalLossVolumeMM3, hitResult.AddedDentVolumeMM3);
                postArmorHealth = armorItem.ArPen_GetCurrentArmorHealth(armorData);
                // Item health is applied after this callback to avoid a
                // re-entrant ruined-item hit; report the queued result.
                float queuedItemDamage = hitResult.ItemMaxHealth * (hitResult.ArmorDamage / Math.Max(hitResult.BaseArmorHealth, 1.0));
                postItemHealth = Math.Max(0.0, hitResult.ItemHealth - queuedItemDamage);
                postKrupp = armorItem.ArPen_GetCurrentKrupp(armorData);
            }

            Print("[ArPen] Armor = " + armor.GetType());
            Print("[ArPen] ArmorHealth01 = " + hitResult.ArmorHealth01.ToString());
            Print("[ArPen] Hardness = " + hitResult.CurrentKrupp.ToString());
            Print("[ArPen] EffectiveKrupp = " + hitResult.EffectiveKrupp.ToString());
            Print("[ArPen] ThicknessMM = " + armorData.ThicknessMM.ToString());
            Print("[ArPen] ImpactVelocity = " + hitResult.ImpactVelocity.ToString());
            Print("[ArPen] ExitVelocity = " + hitResult.ExitVelocity.ToString());
            Print("[ArPen] PenetrationDistanceMM = " + hitResult.PenetrationDistanceMM.ToString());
            Print("[ArPen] ArmorDamage = " + hitResult.ArmorDamage.ToString());
            Print("[ArPen] ImpactEnergyJ = " + hitResult.ImpactEnergyJ.ToString());
            Print("[ArPen] PlateThresholdJ = " + hitResult.PlateThresholdJ.ToString());
            Print("[ArPen] RatedPlateThresholdJ = " + hitResult.RatedPlateThresholdJ.ToString());
            Print("[ArPen] Brittleness = " + hitResult.Brittleness.ToString());
            Print("[ArPen] LocalDamage = " + hitResult.LocalDamage.ToString());
            Print("[ArPen] CrackRadiusMM = " + hitResult.CrackRadiusMM.ToString());
            Print("[ArPen] GlobalCouplingFactor = " + hitResult.GlobalCouplingFactor.ToString());
            Print("[ArPen] MultiHitPenalty = " + hitResult.MultiHitPenalty.ToString());
            Print("[ArPen] DeformationMM = " + hitResult.DeformationMM.ToString());
            Print("[ArPen] MaterialType = " + armorData.MaterialType);
            Print("[ArPen] EffectiveThicknessMM = " + hitResult.EffectiveThicknessMM.ToString());
            Print("[ArPen] TransmittedAccelerationG = " + hitResult.TransmittedAccelerationG.ToString());
            Print("[ArPen] HelmetTrauma = " + hitResult.HelmetTrauma.ToString());
            Print("[ArPen] EnergyFraction = " + hitResult.EnergyFraction.ToString());
            Print("[ArPen] CrackDamageScale = " + hitResult.CrackDamageScale.ToString());
            Print("[ArPen] DamageFractionOfRemaining = " + hitResult.DamageFractionOfRemaining.ToString());
            Print("[ArPen] Penetrated = " + hitResult.Penetrated.ToString());
            Print("[ArPen] DamageMultiplier = " + hitResult.DamageMultiplier.ToString());
        }
        else
        {
            hitResult = new ArPenHitResult();
            hitResult.ImpactVelocity = ammoData.InitialVelocity * Math.Max(speedCoef, 0.0);
            hitResult.ExitVelocity = hitResult.ImpactVelocity;
            hitResult.Penetrated = true;
            hitResult.ImpactEnergyJ = impactEnergyJ;
            hitResult.EffectiveThreatLevel = effectiveThreatLevel;
            hitResult.DamageMultiplier = Math.Clamp(hitResult.ImpactVelocity / ammoData.InitialVelocity, 0.0, 1.0);
            Print("[ArPen] No enrolled armor for zone; applying full custom damage");
        }

        float customHealthDamage;
        float customBloodDamage;
        float customShockDamage;
        float bluntSeverity;
        float stoppedBaseDamage;

        if (hitResult.Penetrated)
        {
            // damageResult already contains the equipped item's vanilla
            // GlobalArmor multiplier. A hard-armor perforation must bypass
            // that reduction or the first penetration is treated like a stop.
            // Ruined armor is already ignored by DayZ, so do not normalize it.
            bool bypassIntactArmor = enrolledArmor && hitResult.ItemHealth > 0.0;
            if (bypassIntactArmor)
            {
                customHealthDamage = ArPen_RemoveVanillaArmorReduction(healthDamage, armor, "Health");
                customBloodDamage = ArPen_RemoveVanillaArmorReduction(bloodDamage, armor, "Blood");
                customShockDamage = ArPen_RemoveVanillaArmorReduction(shockDamage, armor, "Shock");
            }
            else
            {
                customHealthDamage = healthDamage;
                customBloodDamage = bloodDamage;
                customShockDamage = shockDamage;
            }
        }
        else
        {
            // Rebuild stopped-hit trauma from ArPen inputs. Do not reuse
            // damageResult here: it already contains DayZ GlobalArmor multipliers.
            float speedRatio = hitResult.ImpactVelocity / Math.Max(ammoData.InitialVelocity, 0.001);
            stoppedBaseDamage = ammoData.BaseDamage * speedRatio * speedRatio;

            // A stopped projectile transfers all residual energy to armor/body.
            // Current PlateThresholdJ already reflects ceramic/polymer health.
            float armorLoad = (hitResult.ImpactEnergyJ * hitResult.TransferredEnergyFraction) / Math.Max(hitResult.PlateThresholdJ, 1.0);
            float energyBluntSeverity = Math.Pow(Math.Clamp(armorLoad, 0.0, 1.0), 1.25);
            float depthBluntSeverity = Math.Pow(Math.Clamp(hitResult.DepthRatio, 0.0, 1.0), 2.0);
            // PenetrationMultiplier is already included in DepthRatio through
            // PenetrationDistanceMM, so near-perforations now raise blunt trauma.
            bluntSeverity = Math.Max(energyBluntSeverity, depthBluntSeverity);

            float healthZoneMultiplier = ArPen_GetStoppedHealthZoneMultiplier(dmgZone);
            float shockZoneMultiplier = ArPen_GetStoppedShockZoneMultiplier(dmgZone);
            bool isHeadHit = dmgZone == "Head" || dmgZone == "Brain";
            float bluntHealthMultiplier = ammoData.BluntTorsoHealthMultiplier;
            float bluntShockMultiplier = ammoData.BluntTorsoShockMultiplier;
            if (isHeadHit)
            {
                bluntHealthMultiplier = ammoData.BluntHeadHealthMultiplier;
                bluntShockMultiplier = ammoData.BluntHeadShockMultiplier;
            }
            customHealthDamage = stoppedBaseDamage * healthZoneMultiplier * bluntHealthMultiplier * bluntSeverity;
            customShockDamage = stoppedBaseDamage * shockZoneMultiplier * bluntShockMultiplier * bluntSeverity;
            customBloodDamage = 0.0;
        }

        // Returning false below cancels DayZ's original event. Applying health
        // changes inside EEOnDamageCalculated is unreliable because the active
        // damage transaction can overwrite nested DecreaseHealth calls. Queue
        // the custom result for the next script-call-queue update instead.
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ArPen_ApplyCustomDamage, 0, false, customHealthDamage, customBloodDamage, customShockDamage);

        Print("[ArPen] CustomHealthDamage = " + customHealthDamage.ToString());
        Print("[ArPen] CustomBloodDamage = " + customBloodDamage.ToString());
        Print("[ArPen] CustomShockDamage = " + customShockDamage.ToString());
        if (!hitResult.Penetrated)
        {
            Print("[ArPen] StoppedBaseDamage = " + stoppedBaseDamage.ToString());
            Print("[ArPen] BluntSeverity = " + bluntSeverity.ToString());
            Print("[ArPen] BluntDepthRatio = " + hitResult.DepthRatio.ToString());
            Print("[ArPen] BluntHeadHealthMultiplier = " + ammoData.BluntHeadHealthMultiplier.ToString());
            Print("[ArPen] BluntTorsoHealthMultiplier = " + ammoData.BluntTorsoHealthMultiplier.ToString());
            Print("[ArPen] BluntHeadShockMultiplier = " + ammoData.BluntHeadShockMultiplier.ToString());
            Print("[ArPen] BluntTorsoShockMultiplier = " + ammoData.BluntTorsoShockMultiplier.ToString());
        }

        string penetrationStatus = "UNARMORED";
        if (enrolledArmor)
        {
            if (hitResult.Penetrated)
                penetrationStatus = "PENETRATED";
            else
                penetrationStatus = "STOPPED";
        }

        Print("[ArPen] Formula: B=(V*sqrt(M))/(Keff*sqrt(C))*PM");
        Print("[ArPen] PenetrationStatus = " + penetrationStatus);

        return false;
    }
}
