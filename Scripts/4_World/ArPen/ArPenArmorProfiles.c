class ArPenArmorProfile
{
    string ArmorClass;
    bool Enabled = true;
    // Soft armor stays in DayZ's native GlobalArmor damage path.
    bool IsSoftArmor;
    // Unconverted armor keeps base/max Krupp. Its armor-health pool is the
    // schema Health.damage multiplier times GlobalHealth hitpoints.
    bool UseSimpleHealthScaling = true;
    string ArmorLevel = "Unrated";
    float ArmorSchemaHealthDamageMultiplier = 1.0;
    float ArmorSchemaHealthCapacity = 100.0;
    float Krupp = 350.0;
    float ArmorHealth = 100.0;
    float ThicknessMM = 4.0;
    float MinHealthFactor = 0.35;
    float HealthExponent = 1.25;
    float KruppLossPerAbsorbedDamage = 0.70;
    float ItemDamagePerAbsorbedDamage = 0.35;
    // Plate deformation model. ResistanceConstant is calibrated in
    // joules per kg/m2 from the reference threat/areal-density table.
    float ArealDensityKGPerM2 = 25.9;
    float AcousticImpedance = 37.5;
    float ResistanceConstant = 151.6;
    float PlateToughnessJ = 1000.0;
    float BaseDeformationMM = 4.0;
    float MaxDeformationMM = 44.0;
    float BaseCrackRadiusMM = 25.0;
    float GlobalCouplingLow = 0.05;
    float GlobalCouplingSpread = 0.25;
    float MultiHitSpread = 1.0;
    string MaterialID = "silicon_carbide";
    string MaterialType = "Ceramic";
    float MaterialDensityGCM3 = 3.16;
    float BrinellHardness = 0.0;
    float SteelDentToughnessConstant = 2.0;
    float SameHitRadiusMM = 35.0;
    bool IsHelmet = false;
    float HelmetShellMassKG = 1.5;
    float HelmetCurvatureFactor = 0.60;
    float HelmetEnergyTransmission = 0.05;
    float HelmetStoppingDistanceMM = 20.0;
    float HelmetTraumaLimitG = 400.0;
    float SurfaceAreaCM2 = 2500.0;
    // Global metal-volume settings; no position-based dent sites are stored.
    float DentDiameterMultiplier = 4.0;
    float DepthDamageWeight = 0.0;
};

class ArPenArmorProfileFile
{
    int Version = 12;
    ref array<ref ArPenArmorProfile> Profiles;

    void ArPenArmorProfileFile()
    {
        Profiles = new array<ref ArPenArmorProfile>;
    }
};

class ArPenArmorProfiles
{
    protected static const string DIRECTORY = "$profile:ArPen";
    protected static const string FILE_PATH = "$profile:ArPen/ArmorProfiles.json";
    protected static ref ArPenArmorProfileFile s_File;
    protected static bool s_Loaded;

    static void Initialize()
    {
        if (s_Loaded)
            return;

        s_Loaded = true;
        s_File = new ArPenArmorProfileFile();

        if (FileExist(FILE_PATH))
        {
            string loadError;
            ArPenArmorProfileFile loadedFile;
            if (JsonFileLoader<ArPenArmorProfileFile>.LoadFile(FILE_PATH, loadedFile, loadError) && loadedFile)
                s_File = loadedFile;
            else
                ErrorEx("[ArPen] Armor profile load failed: " + loadError);
        }

        if (!s_File.Profiles)
            s_File.Profiles = new array<ref ArPenArmorProfile>;

        int removed = RemoveUnsupportedProfiles();
        int added = AddVanillaTestProfiles();
        added += EnrollLoadedArmorClasses();
        if (!FileExist(FILE_PATH) || added > 0 || removed > 0)
            Save();

        Print("[ArPen] Loaded " + s_File.Profiles.Count().ToString() + " armor profiles; added " + added.ToString() + "; removed " + removed.ToString());
    }

    static bool GetArmorData(string armorClass, out ArPenArmorData data)
    {
        Initialize();

        foreach (ArPenArmorProfile profile : s_File.Profiles)
        {
            if (!profile || profile.ArmorClass != armorClass || !profile.Enabled)
                continue;

            data = new ArPenArmorData();
            data.Enabled = profile.Enabled;
            data.IsSoftArmor = profile.IsSoftArmor;
            data.UseSimpleHealthScaling = profile.UseSimpleHealthScaling;
            data.ArmorLevel = profile.ArmorLevel;
            data.ArmorSchemaHealthDamageMultiplier = profile.ArmorSchemaHealthDamageMultiplier;
            data.ArmorSchemaHealthCapacity = profile.ArmorSchemaHealthCapacity;
            data.BaseKrupp = profile.Krupp;
            data.BaseArmorHealth = profile.ArmorHealth;
            data.ThicknessMM = profile.ThicknessMM;
            data.MinHealthFactor = profile.MinHealthFactor;
            data.HealthExponent = profile.HealthExponent;
            data.KruppLossPerAbsorbedDamage = profile.KruppLossPerAbsorbedDamage;
            data.ItemDamagePerAbsorbedDamage = profile.ItemDamagePerAbsorbedDamage;
            data.ArealDensityKGPerM2 = profile.ArealDensityKGPerM2;
            data.AcousticImpedance = profile.AcousticImpedance;
            data.ResistanceConstant = profile.ResistanceConstant;
            data.PlateToughnessJ = profile.PlateToughnessJ;
            data.BaseDeformationMM = profile.BaseDeformationMM;
            data.MaxDeformationMM = profile.MaxDeformationMM;
            data.BaseCrackRadiusMM = profile.BaseCrackRadiusMM;
            data.GlobalCouplingLow = profile.GlobalCouplingLow;
            data.GlobalCouplingSpread = profile.GlobalCouplingSpread;
            data.MultiHitSpread = profile.MultiHitSpread;
            data.MaterialID = profile.MaterialID;
            data.MaterialType = profile.MaterialType;
            data.MaterialDensityGCM3 = profile.MaterialDensityGCM3;
            data.BrinellHardness = profile.BrinellHardness;
            data.SteelDentToughnessConstant = profile.SteelDentToughnessConstant;
            data.SameHitRadiusMM = profile.SameHitRadiusMM;
            data.IsHelmet = profile.IsHelmet;
            data.HelmetShellMassKG = profile.HelmetShellMassKG;
            data.HelmetCurvatureFactor = profile.HelmetCurvatureFactor;
            data.HelmetEnergyTransmission = profile.HelmetEnergyTransmission;
            data.HelmetStoppingDistanceMM = profile.HelmetStoppingDistanceMM;
            data.HelmetTraumaLimitG = profile.HelmetTraumaLimitG;
            data.SurfaceAreaCM2 = profile.SurfaceAreaCM2;
            data.DentDiameterMultiplier = profile.DentDiameterMultiplier;
            data.DepthDamageWeight = profile.DepthDamageWeight;
            data.FirstHitResidualIntegrity = 0.75;
            data.SubsequentHitIntegrityRatio = 0.333333;
            data.CeramicDamageExponent = 1.75;
            data.CrackInitiationEnergyFraction = 0.18;
            data.SubfloorDamageScale = 0.02;

            ArPenMaterialData material;
            if (ArPenMaterialLibrary.Get(profile.MaterialID, material))
            {
                data.MaterialType = material.Family;
                data.MaterialDensityGCM3 = material.DensityGCM3;
                data.AcousticImpedance = material.AcousticImpedance;
                data.BrinellHardness = material.BrinellHardness;
                data.PlateToughnessJ = material.PlateToughnessJ;
                data.BaseCrackRadiusMM = material.BaseCrackRadiusMM;
                data.GlobalCouplingLow = material.GlobalCouplingLow;
                data.GlobalCouplingSpread = material.GlobalCouplingSpread;
                data.MultiHitSpread = material.MultiHitSpread;
                data.FirstHitResidualIntegrity = material.FirstHitResidualIntegrity;
                data.SubsequentHitIntegrityRatio = material.SubsequentHitIntegrityRatio;
                data.CeramicDamageExponent = material.CeramicDamageExponent;
                data.CrackInitiationEnergyFraction = material.CrackInitiationEnergyFraction;
                data.SubfloorDamageScale = material.SubfloorDamageScale;
            }
            return data.BaseKrupp > 0.0 && data.ThicknessMM > 0.0;
        }

        return false;
    }

    protected static int EnrollLoadedArmorClasses()
    {
        int added;
        int count = GetGame().ConfigGetChildrenCount("CfgVehicles");

        for (int i = 0; i < count; i++)
        {
            string className;
            if (!GetGame().ConfigGetChildName("CfgVehicles", i, className))
                continue;

            string path = "CfgVehicles " + className;
            if (!GetGame().ConfigIsExisting(path + " DamageSystem GlobalArmor Projectile"))
                continue;
            if (!HasArmorSlot(path))
                continue;
            if (!IsSupportedHardBallisticClass(className))
                continue;
            if (FindProfile(className))
                continue;

            ArPenArmorProfile profile = new ArPenArmorProfile();
            profile.ArmorClass = className;
            ApplyClassDefaults(profile);
            ApplyVanillaTestDefaults(profile);
            s_File.Profiles.Insert(profile);
            added++;
        }

        return added;
    }

    protected static void ApplyClassDefaults(ArPenArmorProfile profile)
    {
        if (!profile)
            return;

        profile.IsSoftArmor = IsSoftArmorClass(profile.ArmorClass);
        profile.UseSimpleHealthScaling = true;
        profile.ArmorLevel = "Unrated";
        profile.ArmorSchemaHealthDamageMultiplier = ReadArmorSchemaHealthDamageMultiplier(profile.ArmorClass);
        profile.ArmorSchemaHealthCapacity = ReadArmorSchemaHealthCapacity(profile.ArmorClass, profile.ArmorSchemaHealthDamageMultiplier);
        if (profile.IsSoftArmor)
        {
            profile.MaterialID = "kevlar";
            profile.MaterialType = "SoftArmor";
            return;
        }

        // Unity test helmet: 0.01016 m = 10.16 mm.
        if (IsModernBallisticHelmet(profile.ArmorClass))
        {
            profile.Krupp = 1500.0;
            profile.ArmorHealth = 100.0;
            profile.ThicknessMM = 10.16;
            profile.MinHealthFactor = 0.35;
            profile.HealthExponent = 1.25;
            profile.KruppLossPerAbsorbedDamage = 0.70;
            profile.ItemDamagePerAbsorbedDamage = 1.0;
            profile.ArealDensityKGPerM2 = 5.9;
            profile.AcousticImpedance = 33.5;
            profile.PlateToughnessJ = 650.0;
            profile.BaseDeformationMM = 5.0;
            profile.MaterialID = "uhmwpe";
            profile.MaterialType = "Polymer";
            profile.MaterialDensityGCM3 = 1.44;
            profile.IsHelmet = true;
            profile.HelmetShellMassKG = 1.5;
            profile.HelmetCurvatureFactor = 0.60;
            profile.HelmetEnergyTransmission = 0.05;
            profile.HelmetStoppingDistanceMM = 20.0;
            profile.HelmetTraumaLimitG = 400.0;
            profile.SurfaceAreaCM2 = 1800.0;
            profile.TileSurfaceAreaCM2 = 1800.0;
        }

        // Unity torso armor: 0.024 m = 24 mm, health pool 800.
        // The registry only enrolls items that already have projectile armor,
        // so all enrolled Vest classes are valid torso-armor test candidates.
        if (profile.ArmorClass.Contains("Vest") || profile.ArmorClass.Contains("PlateCarrier"))
        {
            profile.Krupp = 2000.0;
            profile.ArmorHealth = 800.0;
            profile.ThicknessMM = 24.0;
            profile.MinHealthFactor = 0.35;
            profile.HealthExponent = 1.25;
            profile.KruppLossPerAbsorbedDamage = 0.70;
            profile.ItemDamagePerAbsorbedDamage = 1.0;
            // Default ceramic plate is silicon carbide from the reference table.
            profile.ArealDensityKGPerM2 = 25.9;
            profile.AcousticImpedance = 37.5;
            profile.PlateToughnessJ = 1000.0;
            profile.BaseDeformationMM = 4.0;
            profile.MaterialID = "silicon_carbide";
            profile.MaterialType = "Ceramic";
            profile.MaterialDensityGCM3 = 3.16;

            // Name-based test defaults let AR500/steel mods enroll without
            // requiring a direct config block. JSON values remain editable.
            if (profile.ArmorClass.Contains("AR500") || profile.ArmorClass.Contains("Steel"))
            {
                profile.MaterialID = "ar500_steel";
                profile.MaterialType = "Steel";
                profile.MaterialDensityGCM3 = 7.85;
                profile.BrinellHardness = 514.0;
                profile.ArealDensityKGPerM2 = 62.8;
                profile.BaseCrackRadiusMM = 0.0;
                profile.GlobalCouplingLow = 0.02;
                profile.GlobalCouplingSpread = 0.03;
                profile.MultiHitSpread = 0.05;
            }

            if (profile.ArmorClass.Contains("AR600"))
            {
                profile.MaterialID = "ar600_steel";
                profile.BrinellHardness = 600.0;
            }
        }

        // Generic hard armor keeps full base/max Krupp. The vanilla armor
        // reduction determines its custom health pool instead of Krupp.
        if (profile.UseSimpleHealthScaling)
            profile.ArmorHealth = profile.ArmorSchemaHealthCapacity;
    }

    protected static int AddVanillaTestProfiles()
    {
        int added;
        added += AddVanillaArmor("PlateCarrierVest", "III", 2000.0, 800.0, 24.0, "silicon_carbide", "Ceramic");
        added += AddVanillaArmor("BallisticHelmet_ColorBase", "IIIA", 1500.0, 100.0, 10.16, "uhmwpe", "Polymer");
        added += AddVanillaArmor("GorkaHelmet", "IIIA", 1500.0, 100.0, 10.16, "uhmwpe", "Polymer");
        added += AddVanillaArmor("Mich2001Helmet", "IIIA", 1500.0, 100.0, 10.16, "uhmwpe", "Polymer");
        added += AddVanillaArmor("Ssh68Helmet", "Fragment", 6000.0, 85.0, 1.5, "ar500_steel", "Steel");
        return added;
    }

    protected static int AddVanillaArmor(string armorClass, string armorLevel, float krupp, float armorHealth, float thicknessMM, string materialID, string materialType)
    {
        if (!GetGame().ConfigIsExisting("CfgVehicles " + armorClass) || FindProfile(armorClass))
            return 0;

        ArPenArmorProfile profile = new ArPenArmorProfile();
        profile.ArmorClass = armorClass;
        ApplyClassDefaults(profile);
        profile.IsSoftArmor = false;
        profile.UseSimpleHealthScaling = false;
        profile.ArmorLevel = armorLevel;
        profile.Krupp = krupp;
        profile.ArmorHealth = armorHealth;
        profile.ThicknessMM = thicknessMM;
        profile.MaterialID = materialID;
        profile.MaterialType = materialType;
        s_File.Profiles.Insert(profile);
        return 1;
    }

    protected static float ReadArmorSchemaHealthDamageMultiplier(string armorClass)
    {
        string healthPath = "CfgVehicles " + armorClass + " DamageSystem GlobalArmor Projectile Health damage";
        if (!GetGame().ConfigIsExisting(healthPath))
            return 1.0;
        return Math.Clamp(GetGame().ConfigGetFloat(healthPath), 0.0, 1.0);
    }

    protected static float ReadArmorSchemaHealthCapacity(string armorClass, float damageMultiplier)
    {
        string hitpointsPath = "CfgVehicles " + armorClass + " DamageSystem GlobalHealth Health hitpoints";
        float healthCapacity = 100.0;
        if (GetGame().ConfigIsExisting(hitpointsPath))
            healthCapacity = GetGame().ConfigGetFloat(hitpointsPath);
        return Math.Max(healthCapacity * Math.Clamp(damageMultiplier, 0.0, 1.0), 1.0);
    }

    protected static bool IsSoftArmorClass(string armorClass)
    {
        return armorClass.Contains("Kevlar") || armorClass.Contains("SoftArmor") || armorClass.Contains("PressVest") || armorClass.Contains("PoliceVest");
    }

    static bool IsSupportedHardBallisticClass(string armorClass)
    {
        if (IsSoftArmorClass(armorClass))
            return false;
        if (armorClass.Contains("PlateCarrierVest") || IsModernBallisticHelmet(armorClass) || armorClass.Contains("Ssh68Helmet"))
            return true;
        return armorClass.Contains("AR500") || armorClass.Contains("AR600") || armorClass.Contains("SteelPlate") || armorClass.Contains("CeramicPlate") || armorClass.Contains("HardArmor") || armorClass.Contains("ArmorPlate");
    }

    protected static bool IsModernBallisticHelmet(string armorClass)
    {
        return armorClass.Contains("BallisticHelmet") || armorClass.Contains("GorkaHelmet") || armorClass.Contains("Mich2001Helmet");
    }

    protected static int RemoveUnsupportedProfiles()
    {
        int removed;
        for (int i = s_File.Profiles.Count() - 1; i >= 0; i--)
        {
            ArPenArmorProfile profile = s_File.Profiles[i];
            if (!profile || !IsSupportedHardBallisticClass(profile.ArmorClass))
            {
                s_File.Profiles.Remove(i);
                removed++;
            }
        }
        return removed;
    }

    protected static void ApplyVanillaTestDefaults(ArPenArmorProfile profile)
    {
        if (!profile)
            return;

        if (profile.ArmorClass.Contains("PlateCarrierVest"))
        {
            profile.IsSoftArmor = false;
            profile.UseSimpleHealthScaling = false;
            profile.ArmorLevel = "III";
            profile.Krupp = 2000.0;
            profile.ArmorHealth = 800.0;
            profile.ThicknessMM = 24.0;
            profile.MaterialID = "silicon_carbide";
            profile.MaterialType = "Ceramic";
        }
        else if (IsModernBallisticHelmet(profile.ArmorClass))
        {
            profile.IsSoftArmor = false;
            profile.UseSimpleHealthScaling = false;
            profile.ArmorLevel = "IIIA";
            profile.Krupp = 1500.0;
            profile.ArmorHealth = 100.0;
            profile.ThicknessMM = 10.16;
            profile.MaterialID = "uhmwpe";
            profile.MaterialType = "Polymer";
            profile.IsHelmet = true;
        }
        else if (profile.ArmorClass.Contains("Ssh68Helmet"))
        {
            profile.IsSoftArmor = false;
            profile.UseSimpleHealthScaling = false;
            profile.ArmorLevel = "Fragment";
            profile.Krupp = 6000.0;
            profile.ArmorHealth = 85.0;
            profile.ThicknessMM = 1.5;
            profile.MaterialID = "ar500_steel";
            profile.MaterialType = "Steel";
            profile.IsHelmet = true;
        }
    }

    protected static bool HasArmorSlot(string path)
    {
        if (!GetGame().ConfigIsExisting(path + " inventorySlot"))
            return false;

        TStringArray slots = new TStringArray;
        GetGame().ConfigGetTextArray(path + " inventorySlot", slots);
        foreach (string slot : slots)
        {
            if (slot == "Vest" || slot == "Headgear")
                return true;
        }

        string singleSlot;
        if (GetGame().ConfigGetText(path + " inventorySlot", singleSlot))
            return singleSlot == "Vest" || singleSlot == "Headgear";

        return false;
    }

    protected static ArPenArmorProfile FindProfile(string armorClass)
    {
        foreach (ArPenArmorProfile profile : s_File.Profiles)
        {
            if (profile && profile.ArmorClass == armorClass)
                return profile;
        }
        return NULL;
    }

    protected static void Save()
    {
        MakeDirectory(DIRECTORY);
        string saveError;
        if (!JsonFileLoader<ArPenArmorProfileFile>.SaveFile(FILE_PATH, s_File, saveError))
            ErrorEx("[ArPen] Armor profile save failed: " + saveError);
    }
};
