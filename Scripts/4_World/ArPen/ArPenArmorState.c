class ArPenDentSite
{
    vector ModelPosition;
    float DepthMM;
};

modded class ItemBase
{
    protected bool m_ArPenKruppInitialized;
    protected float m_ArPenCurrentKrupp;
    protected float m_ArPenCurrentArmorHealth;
    protected ref array<ref ArPenDentSite> m_ArPenDentSites;

    float ArPen_GetCurrentKrupp(ArPenArmorData armorData)
    {
        if (!m_ArPenKruppInitialized)
        {
            m_ArPenCurrentKrupp = armorData.BaseKrupp;
            m_ArPenCurrentArmorHealth = armorData.BaseArmorHealth;
            m_ArPenKruppInitialized = true;
        }
        return m_ArPenCurrentKrupp;
    }

    float ArPen_GetCurrentArmorHealth(ArPenArmorData armorData)
    {
        ArPen_GetCurrentKrupp(armorData);
        return m_ArPenCurrentArmorHealth;
    }

    float ArPen_GetLocalDentDepth(vector modelPosition, float radiusMM)
    {
        if (!m_ArPenDentSites)
            return 0.0;

        float radiusM = Math.Max(radiusMM, 0.0) * 0.001;
        float depthMM;
        foreach (ArPenDentSite site : m_ArPenDentSites)
        {
            if (site && vector.Distance(site.ModelPosition, modelPosition) <= radiusM)
                depthMM = Math.Max(depthMM, site.DepthMM);
        }
        return depthMM;
    }

    void ArPen_RecordDent(vector modelPosition, float radiusMM, float addedDepthMM)
    {
        if (addedDepthMM <= 0.0)
            return;
        if (!m_ArPenDentSites)
            m_ArPenDentSites = new array<ref ArPenDentSite>;

        float radiusM = Math.Max(radiusMM, 0.0) * 0.001;
        foreach (ArPenDentSite site : m_ArPenDentSites)
        {
            if (site && vector.Distance(site.ModelPosition, modelPosition) <= radiusM)
            {
                site.DepthMM = site.DepthMM + addedDepthMM;
                return;
            }
        }

        ArPenDentSite newSite = new ArPenDentSite();
        newSite.ModelPosition = modelPosition;
        newSite.DepthMM = addedDepthMM;
        m_ArPenDentSites.Insert(newSite);
    }

    void ArPen_AbsorbDamage(ArPenArmorData armorData, float absorbedDamage)
    {
        float currentKrupp = ArPen_GetCurrentKrupp(armorData);
        m_ArPenCurrentKrupp = Math.Max(0.0, currentKrupp - (absorbedDamage * armorData.KruppLossPerAbsorbedDamage));
        m_ArPenCurrentArmorHealth = Math.Max(0.0, m_ArPenCurrentArmorHealth - absorbedDamage);
        float itemDamageRatio = absorbedDamage / Math.Max(armorData.BaseArmorHealth, 1.0);
        float itemDamage = GetMaxHealth("", "Health") * itemDamageRatio * armorData.ItemDamagePerAbsorbedDamage;
        if (itemDamage > 0.0)
            DecreaseHealth("", "Health", itemDamage);
    }
};
