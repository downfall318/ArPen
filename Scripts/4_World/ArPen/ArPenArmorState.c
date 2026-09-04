modded class ItemBase
{
    protected static const int ARPEN_STORE_VERSION = 2;
    protected bool m_ArPenStateInitialized;
    protected float m_ArPenCurrentKrupp;
    protected float m_ArPenCurrentArmorHealth;
    protected float m_ArPenMetalLossVolumeMM3;
    protected float m_ArPenDentVolumeMM3;

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        super.OnStoreSave(ctx);
        ctx.Write(ARPEN_STORE_VERSION);
        ctx.Write(m_ArPenStateInitialized);
        ctx.Write(m_ArPenCurrentKrupp);
        ctx.Write(m_ArPenCurrentArmorHealth);
        ctx.Write(m_ArPenMetalLossVolumeMM3);
        ctx.Write(m_ArPenDentVolumeMM3);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;
        int arPenVersion;
        if (!ctx.Read(arPenVersion))
            return true;
        if (arPenVersion < 1 || arPenVersion > ARPEN_STORE_VERSION)
            return false;
        if (!ctx.Read(m_ArPenStateInitialized) || !ctx.Read(m_ArPenCurrentKrupp) || !ctx.Read(m_ArPenCurrentArmorHealth))
            return false;

        if (arPenVersion == 1)
        {
            // Consume legacy location-based data, but do not retain or use it.
            int dentCount;
            if (!ctx.Read(dentCount) || dentCount < 0 || dentCount > 256)
                return false;
            for (int dentIndex = 0; dentIndex < dentCount; dentIndex++)
            {
                vector oldDentPosition;
                float oldDentDepth;
                if (!ctx.Read(oldDentPosition) || !ctx.Read(oldDentDepth))
                    return false;
            }
            int tileCount;
            if (!ctx.Read(tileCount) || tileCount < 0 || tileCount > 256)
                return false;
            for (int tileIndex = 0; tileIndex < tileCount; tileIndex++)
            {
                vector oldTilePosition;
                int oldHitCount;
                float oldIntegrity;
                if (!ctx.Read(oldTilePosition) || !ctx.Read(oldHitCount) || !ctx.Read(oldIntegrity))
                    return false;
            }
            return true;
        }
        return ctx.Read(m_ArPenMetalLossVolumeMM3) && ctx.Read(m_ArPenDentVolumeMM3);
    }

    protected void ArPen_EnsureState(ArPenArmorData armorData)
    {
        if (m_ArPenStateInitialized)
            return;
        m_ArPenCurrentKrupp = armorData.BaseKrupp;
        // Begin from the item's real condition so a pre-damaged or spawned
        // item cannot receive a hidden full armor-health pool.
        float itemHealth01 = Math.Clamp(GetHealth01("", "Health"), 0.0, 1.0);
        m_ArPenCurrentArmorHealth = armorData.BaseArmorHealth * itemHealth01;
        m_ArPenMetalLossVolumeMM3 = 0.0;
        m_ArPenDentVolumeMM3 = 0.0;
        m_ArPenStateInitialized = true;
    }

    float ArPen_GetCurrentKrupp(ArPenArmorData armorData)
    {
        ArPen_EnsureState(armorData);
        return m_ArPenCurrentKrupp;
    }

    float ArPen_GetCurrentArmorHealth(ArPenArmorData armorData)
    {
        ArPen_EnsureState(armorData);
        return m_ArPenCurrentArmorHealth;
    }

    float ArPen_GetMetalLossVolumeMM3(ArPenArmorData armorData)
    {
        ArPen_EnsureState(armorData);
        return m_ArPenMetalLossVolumeMM3;
    }

    float ArPen_GetDentVolumeMM3(ArPenArmorData armorData)
    {
        ArPen_EnsureState(armorData);
        return m_ArPenDentVolumeMM3;
    }

    protected void ArPen_ApplyItemDamage(float itemDamage)
    {
        if (itemDamage > 0.0)
            DecreaseHealth("", "Health", itemDamage);
    }

    void ArPen_ApplyDamage(ArPenArmorData armorData, float armorDamage, float addedMetalLossVolumeMM3 = 0.0, float addedDentVolumeMM3 = 0.0)
    {
        ArPen_EnsureState(armorData);
        float appliedDamage = Math.Min(Math.Max(armorDamage, 0.0), m_ArPenCurrentArmorHealth);
        m_ArPenCurrentArmorHealth = Math.Max(0.0, m_ArPenCurrentArmorHealth - appliedDamage);

        float panelAreaMM2 = Math.Max(armorData.SurfaceAreaCM2, 1.0) * 100.0;
        float panelVolumeMM3 = panelAreaMM2 * Math.Max(armorData.ThicknessMM, 0.0);
        float failureVolumeMM3 = panelVolumeMM3 * 0.25;
        m_ArPenMetalLossVolumeMM3 = Math.Min(failureVolumeMM3, Math.Max(0.0, m_ArPenMetalLossVolumeMM3 + Math.Max(addedMetalLossVolumeMM3, 0.0)));
        m_ArPenDentVolumeMM3 = Math.Min(panelVolumeMM3, Math.Max(0.0, m_ArPenDentVolumeMM3 + Math.Max(addedDentVolumeMM3, 0.0)));

        // Hardness itself is never reduced. Ceramic/polymer effective Krupp is
        // derived from remaining health; metal always uses BaseKrupp.
        m_ArPenCurrentKrupp = armorData.BaseKrupp;

        float itemDamage = GetMaxHealth("", "Health") * (appliedDamage / Math.Max(armorData.BaseArmorHealth, 1.0));
        if (itemDamage > 0.0)
        {
            // Ruining an armor item from inside PlayerBase's active damage
            // transaction can make DayZ re-evaluate the same projectile and
            // emit duplicate player hits. Keep armor state synchronous, but
            // defer the visible item-health transition until the callback ends.
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ArPen_ApplyItemDamage, 0, false, itemDamage);
        }
    }
};
