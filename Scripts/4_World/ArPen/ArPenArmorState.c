modded class ItemBase
{
    protected bool m_ArPenKruppInitialized;
    protected float m_ArPenCurrentKrupp;
    protected float m_ArPenCurrentArmorHealth;

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
