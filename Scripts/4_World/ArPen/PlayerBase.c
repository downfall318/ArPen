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

        foreach (ArPenZoneDamage zoneDamage : packet.Zones)
        {
            if (zoneDamage.HealthLoss <= 0 || zoneDamage.ZoneName == "")
                continue;
            float remaining = Math.Max(0, GetHealth(zoneDamage.ZoneName, "Health") - zoneDamage.HealthLoss);
            SetHealth(zoneDamage.ZoneName, "Health", remaining);
        }

        // Apply the requested transfer once from this packet's starting pool.
        // Never revive a character killed by native local-zone consequences.
        if (packet.GlobalHealthLoss > 0 && IsAlive())
            SetHealth("", "Health", Math.Max(0, beforeHealth - packet.GlobalHealthLoss));
        SetHealth("", "Blood", Math.Max(0, beforeBlood - packet.GlobalBloodLoss));
        SetHealth("", "Shock", Math.Max(0, beforeShock - packet.GlobalShockLoss));

        if (IsAlive())
        {
            // A custom penetration opens a wound independently of vanilla
            // GlobalArmor Blood=0. The manager validates the original hit
            // selection and refuses duplicate/otherwise disallowed sources.
            if (packet.Penetrated && GetBleedingManagerServer())
                GetBleedingManagerServer().AttemptAddBleedingSource(packet.HitComponentIndex);

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
                    string refillPath = "CfgAmmo " + packet.HitAmmoClassName + " unconRefillModifier";
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
        // Armor already ruined before this hit uses the native damage event.
        // An intact plate that this hit ruins still completes this calculation.
        if (armor && armor.IsRuined())
        {
            bool acceptedRuined = super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);
            if (testHit && acceptedRuined)
                GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(testHit.Finish, 0, false, this);
            return acceptedRuined;
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
                armorItem.ArPen_ApplyDamage(armorData, hitResult);
            if (hitResult.StoppedByDestroyedTile)
            {
                if (testHit)
                {
                    testHit.Result = hitResult;
                    GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(testHit.Finish, 0, false, this);
                }
                return false;
            }
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
        packet.HitAmmoClassName = ammo;
        packet.HitComponentIndex = component;
        packet.HitPosition = modelPos;
        packet.HitSource = source;
        packet.Penetrated = hitResult.Penetrated;
        packet.Telemetry = testHit;

        // Keep the original local health amount; transfer it separately below.
        ArPenZoneDamage localDamage = new ArPenZoneDamage();
        localDamage.ZoneName = dmgZone;
        localDamage.HealthLoss = Math.Max(0, customHealthDamage);
        packet.Zones.Insert(localDamage);
        if (hitResult.Penetrated)
        {
            packet.GlobalBloodLoss = customBloodDamage;
            packet.GlobalShockLoss = customShockDamage;
        }
        else
        {
            packet.GlobalBloodLoss = 0;
            packet.GlobalShockLoss = customShockDamage;
        }

        // Transfer each calculated channel independently; preserve local health damage.
        // Shock already includes its zone factor (custom stopped formula or engine result).
        // Apply it once; do not multiply head shock again during transfer.
        if (dmgZone == "Torso")
        {
            packet.GlobalHealthLoss = localDamage.HealthLoss;
            packet.GlobalShockLoss = customShockDamage;
        }
        else if (dmgZone == "Head" || dmgZone == "Brain")
        {
            // Only firearm hits reach the custom path above.
            packet.GlobalHealthLoss = localDamage.HealthLoss * 2.0;
            packet.GlobalShockLoss = customShockDamage;
        }

        if (testHit)
            testHit.Result = hitResult;
        GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ArPen_ApplyCustomDamage, 0, false, packet);
        return false;
    }
}
