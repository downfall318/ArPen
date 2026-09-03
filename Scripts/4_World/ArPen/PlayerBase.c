modded class PlayerBase
{
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
            string softMessage = "Event: ARMOR HIT\nAmmo: " + ammo;
            softMessage = softMessage + "\nMuzzle threat: " + ammoData.ThreatLevel;
            softMessage = softMessage + "\nImpact threat: " + effectiveThreatLevel;
            softMessage = softMessage + "\nZone: " + dmgZone + "\nArmor: " + armor.GetType();
            softMessage = softMessage + "\nClass: SOFT ARMOR (native damage)";
            NotificationSystem.SendNotificationToPlayerIdentityExtended(GetIdentity(), 12.0, "ArPen Armor Hit", softMessage, "");
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
                postItemHealth = armorItem.GetHealth("", "Health");
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
            // Reuse DayZ's base damage result, but recalculate it at the
            // residual velocity after the projectile exits the armor.
            float postPenetrationScale = hitResult.ExitVelocity / Math.Max(hitResult.ImpactVelocity, 0.001);
            postPenetrationScale = Math.Clamp(postPenetrationScale, 0.0, 1.0);
            customHealthDamage = healthDamage * postPenetrationScale;
            customBloodDamage = bloodDamage * postPenetrationScale;
            customShockDamage = shockDamage * postPenetrationScale;
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

        if (customHealthDamage > 0.0)
            DecreaseHealth("", "Health", customHealthDamage);
        if (customBloodDamage > 0.0)
            DecreaseHealth("", "Blood", customBloodDamage);
        if (customShockDamage > 0.0)
            DecreaseHealth("", "Shock", customShockDamage);

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

        string message = "Event: " + penetrationStatus + "\nAmmo: " + ammo + " | Zone: " + dmgZone;
        message = message + "\nMuzzle threat: " + ammoData.ThreatLevel;
        message = message + "\nImpact threat: " + hitResult.EffectiveThreatLevel;
        message = message + "\nStatus: " + penetrationStatus;
        message = message + "\nV: " + hitResult.ImpactVelocity.ToString() + " -> " + hitResult.ExitVelocity.ToString() + " m/s";

        if (enrolledArmor)
        {
            message = message + "\nArmor: " + armor.GetType();
            message = message + "\nArmor level: " + armorData.ArmorLevel;
            message = message + "\nSchema health damage: " + armorData.ArmorSchemaHealthDamageMultiplier.ToString();
            message = message + "\nSchema armor HP: " + armorData.ArmorSchemaHealthCapacity.ToString();
            message = message + "\nMaterial: " + armorData.MaterialID + " (" + armorData.MaterialType + ")";
            message = message + "\nHardness: " + hitResult.CurrentKrupp.ToString() + " -> " + postKrupp.ToString();
            message = message + "\nEffective K: " + hitResult.EffectiveKrupp.ToString();
            message = message + "\nThickness: " + armorData.ThicknessMM.ToString() + " mm";
            message = message + "\nArmor HP: " + hitResult.CurrentArmorHealth.ToString() + " -> " + postArmorHealth.ToString() + " / " + hitResult.BaseArmorHealth.ToString();
            message = message + "\nItem HP: " + hitResult.ItemHealth.ToString() + " -> " + postItemHealth.ToString() + " / " + hitResult.ItemMaxHealth.ToString();
            message = message + "\nHealth factor: " + hitResult.ArmorHealth01.ToString();
            message = message + "\nPENMAX: " + hitResult.PenetrationDistanceMM.ToString() + " mm vs " + armorData.ThicknessMM.ToString() + " mm";
            message = message + "\nArmor damage: " + hitResult.ArmorDamage.ToString();
            if (!hitResult.Penetrated)
            {
                message = message + "\nDeformation: " + hitResult.DeformationMM.ToString() + " mm";
                message = message + "\nEnergy: " + hitResult.ImpactEnergyJ.ToString() + " / " + hitResult.PlateThresholdJ.ToString() + " J";
                message = message + "\nBrittleness: " + hitResult.Brittleness.ToString();
                message = message + "\nCrack radius: " + hitResult.CrackRadiusMM.ToString() + " mm";
                if (armorData.MaterialType == "Ceramic")
                {
                    message = message + "\nHealth-scaled ceramic damage: " + hitResult.DamageFractionOfRemaining.ToString();
                }
                if (armorData.MaterialType == "Steel")
                    message = message + "\nMetal volume loss: +" + hitResult.AddedMetalLossVolumeMM3.ToString() + " mm3";
                if (armorData.IsHelmet)
                    message = message + "\nHelmet trauma: " + hitResult.TransmittedAccelerationG.ToString() + " g / " + armorData.HelmetTraumaLimitG.ToString() + " g";
            }
        }

        if (!hitResult.Penetrated)
            message = message + "\nBlunt base/severity: " + stoppedBaseDamage.ToString() + " / " + bluntSeverity.ToString();
        message = message + "\nDamage H/B/S: " + customHealthDamage.ToString() + "/" + customBloodDamage.ToString() + "/" + customShockDamage.ToString();
        string notificationTitle = "ArPen Damage Event";
        if (enrolledArmor)
            notificationTitle = "ArPen Armor Hit";
        NotificationSystem.SendNotificationToPlayerIdentityExtended(GetIdentity(), 12.0, notificationTitle, message, "");

        Print("[ArPen] Formula: B=(V*sqrt(M))/(Keff*sqrt(C))*PM");
        Print("[ArPen] PenetrationStatus = " + penetrationStatus);

        return false;
    }
}
