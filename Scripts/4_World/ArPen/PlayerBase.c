modded class PlayerBase
{
    bool ArPen_TestTarget;

    protected void ArPen_ApplyCustomDamage(ArPenWearerDamage packet)
    {
        if (!packet || !IsAlive())
            return;
        ArPenTestHit testHit = packet.Telemetry;
        float beforeHealth = GetHealth("", "Health");
        float beforeBlood = GetHealth("", "Blood");
        float beforeShock = GetHealth("", "Shock");
        if (testHit)
        {
            testHit.Health = beforeHealth;
            testHit.Blood = beforeBlood;
            testHit.Shock = beforeShock;
            testHit.ZoneHealth = GetHealth(testHit.Zone, "Health");
            testHit.Bleeds = ArPenTestTelemetry.BleedCount(this);
        }

        bool fatalZone = false;
        foreach (ArPenZoneDamage zoneDamage : packet.Zones)
        {
            if (zoneDamage.HealthLoss <= 0 || zoneDamage.ZoneName == "")
                continue;
            float remaining = Math.Max(0, GetHealth(zoneDamage.ZoneName, "Health") - zoneDamage.HealthLoss);
            SetHealth(zoneDamage.ZoneName, "Health", remaining);
            string fatalPath = "CfgVehicles " + GetType() + " DamageSystem DamageZones " + zoneDamage.ZoneName + " fatalInjuryCoef";
            if (GetGame().ConfigIsExisting(fatalPath))
            {
                float fatalThreshold = GetGame().ConfigGetFloat(fatalPath);
                if (fatalThreshold >= 0 && remaining <= GetMaxHealth(zoneDamage.ZoneName, "Health") * fatalThreshold)
                    fatalZone = true;
            }
        }

        // Absolute destinations from this application's starting pools prevent
        // adding zone damage a second time to the global damage result. Never
        // restore health if a fatal zone has already killed the character.
        float remainingGlobal = Math.Max(0, beforeHealth - packet.GlobalHealthLoss);
        if (fatalZone || !IsAlive())
            remainingGlobal = 0;
        SetHealth("", "Health", remainingGlobal);
        SetHealth("", "Blood", Math.Max(0, beforeBlood - packet.GlobalBloodLoss));
        SetHealth("", "Shock", Math.Max(0, beforeShock - packet.GlobalShockLoss));

        if (IsAlive())
        {
            if (packet.Penetrated && packet.WoundBloodDamage > 0 && GetBleedingManagerServer())
                GetBleedingManagerServer().ProcessHit(packet.WoundBloodDamage, packet.HitSource, packet.HitComponentIndex, packet.HitZone, packet.AmmoType, packet.HitPosition);

            // Equivalent injury checks to vanilla PlayerBase.EEHitBy, without
            // replaying EEHitBy and its second bleeding/nonlethal damage pass.
            if (GetHealth("RightLeg", "Health") <= 1 || GetHealth("LeftLeg", "Health") <= 1 || GetHealth("RightFoot", "Health") <= 1 || GetHealth("LeftFoot", "Health") <= 1)
            {
                if (GetModifiersManager().IsModifierActive(eModifiers.MDF_BROKEN_LEGS))
                    GetModifiersManager().DeactivateModifier(eModifiers.MDF_BROKEN_LEGS);
                GetModifiersManager().ActivateModifier(eModifiers.MDF_BROKEN_LEGS);
            }
            if (packet.GlobalShockLoss > 0)
            {
                m_LastShockHitTime = GetGame().GetTime();
                if (!IsUnconscious())
                {
                    string refillPath = "CfgAmmo " + packet.AmmoType + " unconRefillModifier";
                    m_UnconRefillModifier = 1;
                    if (GetGame().ConfigIsExisting(refillPath))
                        m_UnconRefillModifier = GetGame().ConfigGetInt(refillPath);
                }
            }
            if (m_ActionManager)
                m_ActionManager.Interrupt();
            m_ShockHandler.CheckValue(true);
        }
        if (testHit)
            testHit.Finish(this);
    }

    protected float ArPen_RemoveVanillaArmorReduction(float damage, EntityAI armor, string damageChannel)
    {
        if (damage <= 0.0)
            return 0.0;
        if (!armor)
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
        ArPenTestHit testHit;
        if (ArPen_TestTarget && ArPenTestSpawner.Enabled())
        {
            testHit = new ArPenTestHit();
            testHit.Capture(this, dmgZone, ammo, component);
        }
        ArPenAmmoData ammoData;

        // Only explicitly enrolled ammo suppresses the vanilla damage event.
        if (damageType != DamageType.FIRE_ARM || !ArPenConfig.ReadAmmo(ammo, ammoData))
        {
            bool accepted0 = super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
            if (testHit && accepted0)
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(testHit.Finish, 0, false, this);
            return accepted0;
        }

        float impactVelocity = ammoData.InitialVelocity * Math.Max(speedCoef, 0.0);
        float impactEnergyJ = 0.5 * ammoData.BulletMassKG * impactVelocity * impactVelocity;
        string effectiveThreatLevel = ArPenAmmoProfiles.GetEffectiveThreatLevel(ammoData, impactEnergyJ);

        float healthDamage = damageResult.GetDamage(dmgZone, "Health");
        float bloodDamage = damageResult.GetDamage(dmgZone, "Blood");
        float shockDamage = damageResult.GetDamage(dmgZone, "Shock");

        EntityAI armor = ArPenBallistics.FindArmor(this, dmgZone);
        if (!armor || armor.IsRuined())
        {
            bool acceptedNative = super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
            if (testHit && acceptedNative)
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(testHit.Finish, 0, false, this);
            return acceptedNative;
        }
        ArPenArmorData armorData;
        ArPenHitResult hitResult;
        bool enrolledArmor = false;
        bool hasArmorProfile = ArPenConfig.ReadArmor(armor, armorData);

        // Equipped armor that is not explicitly enrolled as hard ballistic
        // armor—including soft armor and protective/cosmetic headgear—uses
        // DayZ's original GlobalArmor result.
        if (armor && !hasArmorProfile)
        {
            bool accepted1 = super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
            if (testHit && accepted1)
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(testHit.Finish, 0, false, this);
            return accepted1;
        }

        // Kevlar and other soft armor deliberately retain DayZ's native
        // GlobalArmor calculation and damage application.
        if (hasArmorProfile && armorData.IsSoftArmor)
        {
            bool accepted2 = super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
            if (testHit && accepted2)
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(testHit.Finish, 0, false, this);
            return accepted2;
        }

        if (hasArmorProfile)
        {
            enrolledArmor = true;
            hitResult = ArPenBallistics.Calculate(ammoData, armorData, armor, speedCoef, modelPos);
            ItemBase armorItem = ItemBase.Cast(armor);
            if (armorItem)
                armorItem.ArPen_ApplyDamage(armorData, hitResult.ArmorDamage, hitResult.AddedMetalLossVolumeMM3, hitResult.AddedDentVolumeMM3);
        }
        else
        {
            hitResult = new ArPenHitResult();
            hitResult.ImpactVelocity = impactVelocity;
            hitResult.ExitVelocity = impactVelocity;
            hitResult.Penetrated = true;
            hitResult.ImpactEnergyJ = impactEnergyJ;
            hitResult.EffectiveThreatLevel = effectiveThreatLevel;
            hitResult.DamageMultiplier = Math.Clamp(hitResult.ImpactVelocity / ammoData.InitialVelocity, 0.0, 1.0);
        }

        float customHealthDamage;
        float customBloodDamage;
        float customShockDamage;

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
            float stoppedBaseDamage = ammoData.BaseDamage * speedRatio * speedRatio;

            // A stopped projectile transfers all residual energy to armor/body.
            // Current PlateThresholdJ already reflects ceramic/polymer health.
            float armorLoad = (hitResult.ImpactEnergyJ * hitResult.TransferredEnergyFraction) / Math.Max(hitResult.PlateThresholdJ, 1.0);
            float energyBluntSeverity = Math.Pow(Math.Clamp(armorLoad, 0.0, 1.0), 1.25);
            float depthBluntSeverity = Math.Pow(Math.Clamp(hitResult.DepthRatio, 0.0, 1.0), 2.0);
            // PenetrationMultiplier is already included in DepthRatio through
            // PenetrationDistanceMM, so near-perforations now raise blunt trauma.
            float bluntSeverity = Math.Max(energyBluntSeverity, depthBluntSeverity);

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

        ArPenWearerDamage packet = new ArPenWearerDamage();
        packet.HitZone = dmgZone;
        packet.AmmoType = ammo;
        packet.HitComponentIndex = component;
        packet.HitPosition = modelPos;
        packet.HitSource = source;
        packet.Penetrated = hitResult.Penetrated;
        packet.Telemetry = testHit;

        if (hitResult.Penetrated)
        {
            // DayZ supplies GLOBAL results separately from each zone's result.
            // Armor was intact when this event was calculated, even if this shot
            // has since ruined it. Normalize using that pre-hit state.
            packet.GlobalHealthLoss = ArPen_RemoveVanillaArmorReduction(damageResult.GetDamage("", "Health"), armor, "Health");
            packet.GlobalBloodLoss = ArPen_RemoveVanillaArmorReduction(damageResult.GetDamage("", "Blood"), armor, "Blood");
            packet.GlobalShockLoss = ArPen_RemoveVanillaArmorReduction(damageResult.GetDamage("", "Shock"), armor, "Shock");
            packet.WoundBloodDamage = customBloodDamage;
            array<string> damageZones = new array<string>;
            GetDamageZones(damageZones);
            foreach (string affectedZone : damageZones)
            {
                float zoneLoss = ArPen_RemoveVanillaArmorReduction(damageResult.GetDamage(affectedZone, "Health"), armor, "Health");
                if (zoneLoss <= 0)
                    continue;
                ArPenZoneDamage penetratingZone = new ArPenZoneDamage();
                penetratingZone.ZoneName = affectedZone;
                penetratingZone.HealthLoss = zoneLoss;
                packet.Zones.Insert(penetratingZone);
            }
        }
        else
        {
            // Preserve the tuned GLOBAL blunt-trauma formula. Convert its health
            // result into local HP through the character's configured transfer.
            packet.GlobalHealthLoss = customHealthDamage;
            packet.GlobalBloodLoss = 0;
            packet.GlobalShockLoss = customShockDamage;
            string transferPath = "CfgVehicles " + GetType() + " DamageSystem DamageZones " + dmgZone + " Health transferToGlobalCoef";
            float transfer = 1;
            if (GetGame().ConfigIsExisting(transferPath))
                transfer = GetGame().ConfigGetFloat(transferPath);
            ArPenZoneDamage stoppedZone = new ArPenZoneDamage();
            stoppedZone.ZoneName = dmgZone;
            stoppedZone.HealthLoss = customHealthDamage;
            if (transfer > 0)
                stoppedZone.HealthLoss = customHealthDamage / transfer;
            packet.Zones.Insert(stoppedZone);
        }

        if (testHit)
            testHit.Result = hitResult;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ArPen_ApplyCustomDamage, 0, false, packet);
        return false;
    }
}
