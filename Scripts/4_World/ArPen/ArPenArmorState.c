class ArPenDentSite
{
    vector ModelPosition;
    float DepthMM;
};

class ArPenCeramicTileState
{
    vector ModelPosition;
    int HitCount;
    float Integrity = 1.0;
};

modded class ItemBase
{
    protected bool m_ArPenKruppInitialized;
    protected float m_ArPenCurrentKrupp;
    protected float m_ArPenCurrentArmorHealth;
    protected ref array<ref ArPenDentSite> m_ArPenDentSites;
    protected ref array<ref ArPenCeramicTileState> m_ArPenCeramicTiles;

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

    protected float ArPen_GetTileRadiusM(float tileSurfaceAreaCM2)
    {
        // Treat the configured tile area as a circular lookup footprint.
        float radiusCM = Math.Sqrt(Math.Max(tileSurfaceAreaCM2, 1.0) / Math.PI);
        return radiusCM * 0.01;
    }

    ArPenCeramicTileState ArPen_FindCeramicTile(vector modelPosition, float tileSurfaceAreaCM2)
    {
        if (!m_ArPenCeramicTiles)
            return NULL;

        float radiusM = ArPen_GetTileRadiusM(tileSurfaceAreaCM2);
        foreach (ArPenCeramicTileState tile : m_ArPenCeramicTiles)
        {
            if (tile && vector.Distance(tile.ModelPosition, modelPosition) <= radiusM)
                return tile;
        }
        return NULL;
    }

    float ArPen_GetCeramicTileIntegrity(vector modelPosition, float tileSurfaceAreaCM2)
    {
        ArPenCeramicTileState tile = ArPen_FindCeramicTile(modelPosition, tileSurfaceAreaCM2);
        if (!tile)
            return 1.0;
        return Math.Clamp(tile.Integrity, 0.0, 1.0);
    }

    int ArPen_GetCeramicTileHitCount(vector modelPosition, float tileSurfaceAreaCM2)
    {
        ArPenCeramicTileState tile = ArPen_FindCeramicTile(modelPosition, tileSurfaceAreaCM2);
        if (!tile)
            return 0;
        return tile.HitCount;
    }

    void ArPen_RecordCeramicTileHit(vector modelPosition, float tileSurfaceAreaCM2, float resultingIntegrity)
    {
        if (!m_ArPenCeramicTiles)
            m_ArPenCeramicTiles = new array<ref ArPenCeramicTileState>;

        ArPenCeramicTileState tile = ArPen_FindCeramicTile(modelPosition, tileSurfaceAreaCM2);
        if (!tile)
        {
            tile = new ArPenCeramicTileState();
            tile.ModelPosition = modelPosition;
            m_ArPenCeramicTiles.Insert(tile);
        }

        tile.HitCount++;
        tile.Integrity = Math.Clamp(resultingIntegrity, 0.0, 1.0);
    }

    void ArPen_ApplyAreaIntegrityLoss(ArPenArmorData armorData, float armorHealthLoss, float kruppLoss)
    {
        ArPen_GetCurrentKrupp(armorData);
        m_ArPenCurrentArmorHealth = Math.Max(0.0, m_ArPenCurrentArmorHealth - Math.Max(armorHealthLoss, 0.0));
        m_ArPenCurrentKrupp = Math.Max(0.0, m_ArPenCurrentKrupp - Math.Max(kruppLoss, 0.0));

        float healthFraction = Math.Max(armorHealthLoss, 0.0) / Math.Max(armorData.BaseArmorHealth, 1.0);
        float itemHealthLoss = GetMaxHealth("", "Health") * healthFraction;
        if (itemHealthLoss > 0.0)
            DecreaseHealth("", "Health", itemHealthLoss);
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
