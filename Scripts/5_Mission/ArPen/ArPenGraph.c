class ArPenGraphPoint
{
    float Velocity;
    float ArmorDamage;
    bool Penetrated;
    string EffectiveThreatLevel;
};

class ArPenGraphState
{
    float ArmorHealth;
    float CurrentKrupp;
    float ItemHealth01 = 1.0;
    float TileIntegrity = 1.0;
    float DentDepthMM;
};

class ArPenGraphSimulator
{
    static ArPenGraphPoint CalculateShot(ArPenAmmoData ammoData, ArPenArmorData armorData, float velocity, ArPenGraphState state)
    {
        ArPenGraphPoint point = new ArPenGraphPoint();
        point.Velocity = velocity;
        float impactEnergyJ = 0.5 * ammoData.BulletMassKG * velocity * velocity;
        point.EffectiveThreatLevel = ArPenAmmoProfiles.GetEffectiveThreatLevel(ammoData, impactEnergyJ);

        float armorHealth01 = Math.Min(state.ItemHealth01, Math.Clamp(state.ArmorHealth / Math.Max(armorData.BaseArmorHealth, 1.0), 0.0, 1.0));
        float effectiveKrupp = state.CurrentKrupp;
        if (!armorData.UseSimpleHealthScaling)
        {
            float healthFactor = Math.Pow(armorHealth01, armorData.HealthExponent);
            healthFactor = armorData.MinHealthFactor + ((1.0 - armorData.MinHealthFactor) * healthFactor);
            effectiveKrupp = state.CurrentKrupp * healthFactor;
        }
        if (armorData.MaterialType == "Ceramic" && !armorData.UseSimpleHealthScaling)
            effectiveKrupp = armorData.BaseKrupp * state.TileIntegrity;

        float effectiveThicknessMM = armorData.ThicknessMM;
        if (armorData.MaterialType == "Steel")
            effectiveThicknessMM = Math.Max(armorData.ThicknessMM - state.DentDepthMM, 0.1);

        float penetrationDistanceMM;
        if (effectiveKrupp > 0.0 && ammoData.CaliberMM > 0.0)
        {
            penetrationDistanceMM = (velocity * Math.Sqrt(ammoData.BulletMassKG)) / (effectiveKrupp * Math.Sqrt(ammoData.CaliberMM));
            penetrationDistanceMM = penetrationDistanceMM * ammoData.PenetrationMultiplier * 1000.0;
        }

        float traveledMM = Math.Min(penetrationDistanceMM, effectiveThicknessMM);
        float velocityScale = Math.Clamp(traveledMM / Math.Max(effectiveThicknessMM, 0.1), 0.0, 1.0);
        float damagePerVelocity = ammoData.BaseDamage / Math.Max(ammoData.InitialVelocity, 0.001);
        float baseArmorDamage = damagePerVelocity * 5.0 * velocity * Math.Pow(ammoData.PenetrationMultiplier, 4.0);
        float calculatedArmorDamage = Math.Min(baseArmorDamage * velocityScale * 3.0 / Math.Max(armorHealth01, 0.01), state.ArmorHealth);

        float exitVelocity;
        if (effectiveThicknessMM <= penetrationDistanceMM)
        {
            float energyRemainingRatio = 1.0 - (effectiveThicknessMM / penetrationDistanceMM);
            exitVelocity = velocity * Math.Sqrt(Math.Max(energyRemainingRatio, 0.0));
            point.Penetrated = exitVelocity > 50.0;
        }

        float ratedPlateThresholdJ = armorData.ArealDensityKGPerM2 * armorData.ResistanceConstant;
        float plateThresholdJ = ratedPlateThresholdJ;
        if (armorData.MaterialType == "Ceramic")
            plateThresholdJ = plateThresholdJ * state.TileIntegrity;
        if (armorData.IsHelmet)
            plateThresholdJ = plateThresholdJ / Math.Clamp(armorData.HelmetCurvatureFactor, 0.1, 1.0);
        if (armorData.IsHelmet && impactEnergyJ <= plateThresholdJ)
            point.Penetrated = false;

        if (armorData.MaterialType == "Ceramic")
        {
            float crackFloor = Math.Clamp(armorData.CrackInitiationEnergyFraction, 0.01, 0.95);
            float energyFraction = impactEnergyJ / Math.Max(ratedPlateThresholdJ, 1.0);
            float crackDamageScale;
            if (energyFraction <= crackFloor)
            {
                float belowFloor = Math.Clamp(energyFraction / crackFloor, 0.0, 1.0);
                crackDamageScale = Math.Clamp(armorData.SubfloorDamageScale, 0.0, 1.0) * Math.Pow(belowFloor, Math.Max(armorData.CeramicDamageExponent, 1.0));
            }
            else
            {
                float aboveFloor = Math.Clamp((energyFraction - crackFloor) / (1.0 - crackFloor), 0.0, 1.0);
                crackDamageScale = Math.Clamp(armorData.SubfloorDamageScale, 0.0, 1.0) + ((1.0 - Math.Clamp(armorData.SubfloorDamageScale, 0.0, 1.0)) * Math.Pow(aboveFloor, Math.Max(armorData.CeramicDamageExponent, 1.0)));
            }

            float firstHitDamage = 1.0 - Math.Clamp(armorData.FirstHitResidualIntegrity, 0.0, 1.0);
            float establishedCrackDamage = 1.0 - Math.Clamp(armorData.SubsequentHitIntegrityRatio, 0.0, 1.0);
            float fractureNetwork = Math.Clamp((1.0 - state.TileIntegrity) / Math.Max(firstHitDamage, 0.001), 0.0, 1.0);
            float calibratedRatedDamage = firstHitDamage + ((establishedCrackDamage - firstHitDamage) * fractureNetwork);
            float damageFraction = Math.Clamp(calibratedRatedDamage * crackDamageScale, 0.0, 1.0);
            float resultingIntegrity = state.TileIntegrity * (1.0 - damageFraction);
            float affectedAreaCM2 = Math.Min(armorData.TileSurfaceAreaCM2, armorData.SurfaceAreaCM2);
            point.ArmorDamage = Math.Max(state.TileIntegrity - resultingIntegrity, 0.0) * affectedAreaCM2 * (armorData.BaseArmorHealth / Math.Max(armorData.SurfaceAreaCM2, 1.0));
            state.TileIntegrity = resultingIntegrity;
            state.ArmorHealth = Math.Max(0.0, state.ArmorHealth - point.ArmorDamage);
            state.CurrentKrupp = Math.Max(0.0, state.CurrentKrupp - (armorData.BaseKrupp * point.ArmorDamage / Math.Max(armorData.BaseArmorHealth, 1.0)));
            state.ItemHealth01 = Math.Max(0.0, state.ItemHealth01 - (point.ArmorDamage / Math.Max(armorData.BaseArmorHealth, 1.0)));
        }
        else
        {
            point.ArmorDamage = calculatedArmorDamage;
            if (armorData.MaterialType == "Steel" && !point.Penetrated)
            {
                float addedDentMM = impactEnergyJ / (Math.Max(armorData.BrinellHardness, 1.0) * Math.Max(armorData.SteelDentToughnessConstant, 0.01));
                state.DentDepthMM = state.DentDepthMM + Math.Min(addedDentMM, effectiveThicknessMM);
            }
            state.ArmorHealth = Math.Max(0.0, state.ArmorHealth - point.ArmorDamage);
            state.CurrentKrupp = Math.Max(0.0, state.CurrentKrupp - (point.ArmorDamage * armorData.KruppLossPerAbsorbedDamage));
            state.ItemHealth01 = Math.Max(0.0, state.ItemHealth01 - ((point.ArmorDamage / Math.Max(armorData.BaseArmorHealth, 1.0)) * armorData.ItemDamagePerAbsorbedDamage));
        }
        return point;
    }

    static void BuildShotGraph(ArPenAmmoData ammoData, ArPenArmorData armorData, int shotNumber, out array<ref ArPenGraphPoint> points)
    {
        points = new array<ref ArPenGraphPoint>;
        int maxVelocity = Math.Ceil(ammoData.InitialVelocity / 50.0) * 50;
        for (int velocity = 0; velocity <= maxVelocity; velocity += 50)
        {
            ArPenGraphState state = new ArPenGraphState();
            state.ArmorHealth = armorData.BaseArmorHealth;
            state.CurrentKrupp = armorData.BaseKrupp;
            ArPenGraphPoint point;
            for (int shot = 1; shot <= shotNumber; shot++)
                point = CalculateShot(ammoData, armorData, velocity, state);
            points.Insert(point);
        }
    }
};

class ArPenGraphRow
{
    protected Widget m_Root;
    protected CanvasWidget m_Canvas;
    protected TextWidget m_Title;
    protected TextWidget m_Detail;

    void ArPenGraphRow(Widget parent, string ammoName, string armorName, ArPenAmmoData ammoData, ArPenArmorData armorData, int shotNumber)
    {
        m_Root = GetGame().GetWorkspace().CreateWidgets("ArPen/GUI/layouts/ArPenGraphRow.layout", parent);
        m_Canvas = CanvasWidget.Cast(m_Root.FindAnyWidget("GraphCanvas"));
        m_Title = TextWidget.Cast(m_Root.FindAnyWidget("GraphTitle"));
        m_Detail = TextWidget.Cast(m_Root.FindAnyWidget("GraphDetail"));
        m_Title.SetText("Shot " + shotNumber.ToString() + " — " + ammoName + " vs " + armorName);
        m_Root.Update();
        Draw(ammoData, armorData, shotNumber);
    }

    protected int ThreatColor(string level)
    {
        if (level.Contains("IIIA+")) return ARGB(255, 105, 220, 255);
        if (level.Contains("III+")) return ARGB(255, 255, 155, 70);
        if (level.Contains("IV")) return ARGB(255, 205, 92, 255);
        if (level.Contains("III")) return ARGB(255, 255, 215, 75);
        if (level.Contains("IIA")) return ARGB(255, 80, 220, 150);
        if (level.Contains("II")) return ARGB(255, 90, 160, 255);
        if (level.Contains("Sub-IIA")) return ARGB(255, 150, 220, 150);
        return ARGB(255, 125, 125, 135);
    }

    protected void Draw(ArPenAmmoData ammoData, ArPenArmorData armorData, int shotNumber)
    {
        array<ref ArPenGraphPoint> points;
        ArPenGraphSimulator.BuildShotGraph(ammoData, armorData, shotNumber, points);
        if (!m_Canvas || !points || points.Count() < 2)
            return;

        float width;
        float height;
        m_Canvas.GetScreenSize(width, height);
        float left = 54.0;
        float right = width - 18.0;
        float top = 12.0;
        float bottom = height - 32.0;
        // Keep every consecutive-shot graph on the same Y scale so the
        // curves can be compared directly while scrolling.
        float maxDamage = Math.Max(armorData.BaseArmorHealth, 1.0);

        m_Canvas.Clear();
        int grid = ARGB(100, 120, 130, 145);
        m_Canvas.DrawLine(left, top, left, bottom, 1.0, ARGB(255, 220, 225, 230));
        m_Canvas.DrawLine(left, bottom, right, bottom, 1.0, ARGB(255, 220, 225, 230));
        for (int gridLine = 1; gridLine <= 4; gridLine++)
        {
            float gy = top + ((bottom - top) * gridLine / 4.0);
            m_Canvas.DrawLine(left, gy, right, gy, 1.0, grid);
        }

        float maxVelocity = points[points.Count() - 1].Velocity;
        ArPenGraphPoint previous = points[0];
        float previousX = left;
        float previousY = bottom - ((previous.ArmorDamage / maxDamage) * (bottom - top));
        for (int i = 0; i < points.Count(); i++)
        {
            ArPenGraphPoint current = points[i];
            float px = left + ((current.Velocity / Math.Max(maxVelocity, 1.0)) * (right - left));
            float py = bottom - ((current.ArmorDamage / maxDamage) * (bottom - top));
            int statusColor = ARGB(255, 65, 205, 150);
            if (current.Penetrated)
                statusColor = ARGB(255, 255, 80, 80);
            if (i > 0)
                m_Canvas.DrawLine(previousX, previousY, px, py, 3.0, statusColor);
            m_Canvas.DrawLine(px, bottom + 5.0, px, bottom + 13.0, 5.0, ThreatColor(current.EffectiveThreatLevel));
            previousX = px;
            previousY = py;
        }

        string transitions;
        string lastThreat;
        foreach (ArPenGraphPoint threatPoint : points)
        {
            if (threatPoint.EffectiveThreatLevel != lastThreat)
            {
                if (transitions != "")
                    transitions = transitions + "  |  ";
                transitions = transitions + threatPoint.Velocity.ToString() + "+ m/s: " + threatPoint.EffectiveThreatLevel;
                lastThreat = threatPoint.EffectiveThreatLevel;
            }
        }
        m_Detail.SetText("Y: armor health damage (max " + maxDamage.ToString() + ")   X: impact velocity, 50 m/s steps\nGreen = stopped   Red = penetrated   Bottom strip = effective threat\n" + transitions);
    }
};
