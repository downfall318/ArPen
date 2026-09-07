modded class ItemBase
{
    protected bool m_ArPenStateInitialized;
    protected float m_ArPenCurrentKrupp;
    protected float m_ArPenCurrentArmorHealth;
    protected float m_ArPenMetalLossVolumeMM3;
    protected float m_ArPenDentVolumeMM3;
    protected ref array<float> m_ArPenTileHealth = new array<float>;
    protected float m_ArPenLastItemHealth01;
    // Damage already committed to our state, but not yet to engine item HP.
    protected float m_ArPenPendingItemDamage;

    override void OnStoreSave(ParamsWriteContext ctx)
    {
        // Persist engine HP and tile HP from the same completed state.
        if (m_ArPenPendingItemDamage > 0.0)
        {
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).Remove(ArPen_ApplyItemDamage);
            ArPen_ApplyItemDamage(m_ArPenPendingItemDamage);
        }
        super.OnStoreSave(ctx);
        ctx.Write(m_ArPenStateInitialized);
        ctx.Write(m_ArPenCurrentKrupp);
        ctx.Write(m_ArPenCurrentArmorHealth);
        ctx.Write(m_ArPenMetalLossVolumeMM3);
        ctx.Write(m_ArPenDentVolumeMM3);
        ctx.Write(m_ArPenTileHealth);
        ctx.Write(m_ArPenLastItemHealth01);
    }

    override bool OnStoreLoad(ParamsReadContext ctx, int version)
    {
        if (!super.OnStoreLoad(ctx, version))
            return false;
        // One fixed layout, matching OnStoreSave; no legacy readers.
        if (!ctx.Read(m_ArPenStateInitialized) || !ctx.Read(m_ArPenCurrentKrupp) || !ctx.Read(m_ArPenCurrentArmorHealth))
            return false;
        if (!ctx.Read(m_ArPenMetalLossVolumeMM3) || !ctx.Read(m_ArPenDentVolumeMM3))
            return false;
        return ctx.Read(m_ArPenTileHealth) && ctx.Read(m_ArPenLastItemHealth01);
    }

    protected float ArPen_EffectiveItemHealth01()
    {
        return Math.Clamp((GetHealth("", "Health") - m_ArPenPendingItemDamage) / Math.Max(GetMaxHealth("", "Health"), 0.001), 0.0, 1.0);
    }

    protected float ArPen_TileCondition()
    {
        float total;
        int defeated;
        foreach (float health : m_ArPenTileHealth)
        {
            total += health;
            if (health <= 0.0)
                defeated++;
        }
        int count = m_ArPenTileHealth.Count();
        if (count == 0 || defeated * 4 >= count)
            return 0.0;
        return total / count;
    }

    protected void ArPen_EnsureState(ArPenArmorData armorData)
    {
        float itemHealth01 = ArPen_EffectiveItemHealth01();
        float panelVolume = Math.Max(armorData.SurfaceAreaCM2, 1.0) * 100.0 * Math.Max(armorData.ThicknessMM, 0.0);
        int tileCount = ArPenConfig.TileCount(armorData);
        if (!m_ArPenTileHealth)
            m_ArPenTileHealth = new array<float>;
        if (!m_ArPenStateInitialized || m_ArPenTileHealth.Count() != tileCount)
        {
            m_ArPenCurrentArmorHealth = armorData.BaseArmorHealth * itemHealth01;
            m_ArPenMetalLossVolumeMM3 = panelVolume * 0.25 * (1.0 - itemHealth01);
            m_ArPenDentVolumeMM3 = 0.0;
            m_ArPenTileHealth.Clear();
            for (int i = 0; i < tileCount; i++)
                m_ArPenTileHealth.Insert(itemHealth01);
            m_ArPenLastItemHealth01 = itemHealth01;
            m_ArPenStateInitialized = true;
        }
        else if (Math.AbsFloat(itemHealth01 - m_ArPenLastItemHealth01) > 0.00001)
        {
            // Repairs and damage outside ArPen update the same durability pool.
            // Excluding pending damage prevents a second pellet from healing it.
            float previous = m_ArPenLastItemHealth01;
            for (int tile = 0; tile < tileCount; tile++)
            {
                float tileHealth = m_ArPenTileHealth[tile];
                if (itemHealth01 > previous)
                    tileHealth += (1.0 - tileHealth) * Math.Clamp((itemHealth01 - previous) / Math.Max(1.0 - previous, 0.00001), 0.0, 1.0);
                else
                    tileHealth *= itemHealth01 / Math.Max(previous, 0.00001);
                m_ArPenTileHealth[tile] = Math.Clamp(tileHealth, 0.0, 1.0);
            }
            if (itemHealth01 > previous)
                m_ArPenDentVolumeMM3 *= (1.0 - itemHealth01) / Math.Max(1.0 - previous, 0.00001);
            m_ArPenMetalLossVolumeMM3 = panelVolume * 0.25 * (1.0 - itemHealth01);
            m_ArPenLastItemHealth01 = itemHealth01;
        }
        m_ArPenCurrentKrupp = armorData.BaseKrupp;
        float condition = itemHealth01;
        if (tileCount > 0)
            condition = ArPen_TileCondition();
        m_ArPenCurrentArmorHealth = armorData.BaseArmorHealth * condition;
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

    float ArPen_GetTileHealth01(ArPenArmorData armorData, int tile)
    {
        ArPen_EnsureState(armorData);
        return m_ArPenTileHealth[tile];
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
        DecreaseHealth("", "Health", itemDamage);
        m_ArPenPendingItemDamage = Math.Max(0.0, m_ArPenPendingItemDamage - itemDamage);
    }

    void ArPen_ApplyDamage(ArPenArmorData armorData, ArPenHitResult hit)
    {
        ArPen_EnsureState(armorData);
        float appliedDamage = Math.Min(Math.Max(hit.ArmorDamage, 0.0), m_ArPenCurrentArmorHealth);
        if (hit.TileIndex >= 0 && hit.TileIndex < m_ArPenTileHealth.Count())
        {
            // Tile damage is measured in that tile's configured HP, not whole-vest HP.
            float tileHealth = m_ArPenTileHealth[hit.TileIndex];
            float capacity = armorData.Tiles[hit.TileIndex];
            float remainingTileHP = Math.Max(0.0, tileHealth * capacity - hit.ArmorDamage);
            m_ArPenTileHealth[hit.TileIndex] = remainingTileHP / capacity;
            m_ArPenCurrentArmorHealth = armorData.BaseArmorHealth * ArPen_TileCondition();
        }
        else
            m_ArPenCurrentArmorHealth = Math.Max(0.0, m_ArPenCurrentArmorHealth - appliedDamage);

        float panelVolume = Math.Max(armorData.SurfaceAreaCM2, 1.0) * 100.0 * Math.Max(armorData.ThicknessMM, 0.0);
        m_ArPenMetalLossVolumeMM3 = Math.Min(panelVolume * 0.25, Math.Max(0.0, m_ArPenMetalLossVolumeMM3 + Math.Max(hit.AddedMetalLossVolumeMM3, 0.0)));
        m_ArPenDentVolumeMM3 = Math.Min(panelVolume, Math.Max(0.0, m_ArPenDentVolumeMM3 + Math.Max(hit.AddedDentVolumeMM3, 0.0)));
        m_ArPenCurrentKrupp = armorData.BaseKrupp;

        float nextCondition = Math.Clamp(m_ArPenCurrentArmorHealth / Math.Max(armorData.BaseArmorHealth, 0.001), 0.0, 1.0);
        float itemDamage = GetMaxHealth("", "Health") * Math.Max(0.0, ArPen_EffectiveItemHealth01() - nextCondition);
        m_ArPenLastItemHealth01 = nextCondition;
        if (itemDamage > 0.0)
        {
            m_ArPenPendingItemDamage += itemDamage;
            // Keep the item-health transition outside the active projectile callback.
            GetGame().GetCallQueue(CALL_CATEGORY_SYSTEM).CallLater(ArPen_ApplyItemDamage, 0, false, itemDamage);
        }
    }
};
