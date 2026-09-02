modded class PlayerBase
{

    override bool EEOnDamageCalculated(TotalDamageResult damageResult, int damageType, EntityAI source, int component, string dmgZone, string ammo, vector modelPos, float speedCoef)
    {
        ArPenAmmoData ammoData;

        // Only explicitly enrolled ammo suppresses the vanilla damage event.
        if (!ArPenConfig.ReadAmmo(ammo, ammoData))
            return super.EEOnDamageCalculated(damageResult, damageType, source, component, dmgZone, ammo, modelPos, speedCoef);

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

        if (ArPenConfig.ReadArmor(armor, armorData))
        {
            enrolledArmor = true;
            hitResult = ArPenBallistics.Calculate(ammoData, armorData, armor, speedCoef);
            ItemBase armorItem = ItemBase.Cast(armor);
            if (armorItem)
            {
                armorItem.ArPen_AbsorbDamage(armorData, hitResult.ArmorDamage);
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
            Print("[ArPen] Penetrated = " + hitResult.Penetrated.ToString());
            Print("[ArPen] DamageMultiplier = " + hitResult.DamageMultiplier.ToString());
        }
        else
        {
            hitResult = new ArPenHitResult();
            hitResult.ImpactVelocity = ammoData.InitialVelocity * Math.Max(speedCoef, 0.0);
            hitResult.ExitVelocity = hitResult.ImpactVelocity;
            hitResult.Penetrated = true;
            hitResult.DamageMultiplier = Math.Clamp(hitResult.ImpactVelocity / ammoData.InitialVelocity, 0.0, 1.0);
            Print("[ArPen] No enrolled armor for zone; applying full custom damage");
        }

        float customHealthDamage = ammoData.BaseDamage * hitResult.DamageMultiplier;
        float customBloodDamage = customHealthDamage * ammoData.BloodDamageMultiplier;
        float customShockDamage;

        if (hitResult.Penetrated)
            customShockDamage = customHealthDamage * ammoData.ShockDamageMultiplier;
        else
            customShockDamage = ammoData.BaseDamage * ammoData.BluntShockMultiplier;

        if (customHealthDamage > 0.0)
            DecreaseHealth("", "Health", customHealthDamage);
        if (customBloodDamage > 0.0)
            DecreaseHealth("", "Blood", customBloodDamage);
        if (customShockDamage > 0.0)
            DecreaseHealth("", "Shock", customShockDamage);

        Print("[ArPen] CustomHealthDamage = " + customHealthDamage.ToString());
        Print("[ArPen] CustomBloodDamage = " + customBloodDamage.ToString());
        Print("[ArPen] CustomShockDamage = " + customShockDamage.ToString());

        string penetrationStatus = "UNARMORED";
        if (enrolledArmor)
        {
            if (hitResult.Penetrated)
                penetrationStatus = "PENETRATED";
            else
                penetrationStatus = "STOPPED";
        }

        string message = "Ammo: " + ammo + " | Zone: " + dmgZone;
        message = message + "\nStatus: " + penetrationStatus;
        message = message + "\nV: " + hitResult.ImpactVelocity.ToString() + " -> " + hitResult.ExitVelocity.ToString() + " m/s";

        if (enrolledArmor)
        {
            message = message + "\nArmor: " + armor.GetType();
            message = message + "\nHardness: " + hitResult.CurrentKrupp.ToString() + " -> " + postKrupp.ToString();
            message = message + "\nEffective K: " + hitResult.EffectiveKrupp.ToString();
            message = message + "\nThickness: " + armorData.ThicknessMM.ToString() + " mm";
            message = message + "\nArmor HP: " + hitResult.CurrentArmorHealth.ToString() + " -> " + postArmorHealth.ToString() + " / " + hitResult.BaseArmorHealth.ToString();
            message = message + "\nItem HP: " + hitResult.ItemHealth.ToString() + " -> " + postItemHealth.ToString() + " / " + hitResult.ItemMaxHealth.ToString();
            message = message + "\nHealth factor: " + hitResult.ArmorHealth01.ToString();
            message = message + "\nPENMAX: " + hitResult.PenetrationDistanceMM.ToString() + " mm vs " + armorData.ThicknessMM.ToString() + " mm";
            message = message + "\nArmor damage: " + hitResult.ArmorDamage.ToString();
        }

        message = message + "\nDamage H/B/S: " + customHealthDamage.ToString() + "/" + customBloodDamage.ToString() + "/" + customShockDamage.ToString();
        NotificationSystem.SendNotificationToPlayerIdentityExtended(NULL, 12.0, "ArPen Formula", message, "");

        Print("[ArPen] Formula: B=(V*sqrt(M))/(Keff*sqrt(C))*PM");
        Print("[ArPen] PenetrationStatus = " + penetrationStatus);

        return false;
    }
}
