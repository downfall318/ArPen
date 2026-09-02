class ArPenArmorProfile
{
    string ArmorClass;
    bool Enabled = true;
    // Soft armor stays in DayZ's native GlobalArmor damage path.
    bool IsSoftArmor;
    // Unconverted armor uses base Krupp scaled by the protection percentage
    // in DamageSystem/GlobalArmor/Projectile/Health.
    bool UseSimpleHealthScaling = true;
    string ArmorLevel = "Unrated";
    float ArmorSchemaHealthProtection = 1.0;
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
    float TileSurfaceAreaCM2 = 625.0;
};

class ArPenArmorProfileFile
{
    int Version = 9;
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

        bool migrated = ApplyVersion2TestValues();
        int added = AddVanillaTestProfiles();
        added += EnrollLoadedArmorClasses();
        if (!FileExist(FILE_PATH) || added > 0 || migrated)
            Save();

        Print("[ArPen] Loaded " + s_File.Profiles.Count().ToString() + " armor profiles; added " + added.ToString());
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
            data.ArmorSchemaHealthProtection = profile.ArmorSchemaHealthProtection;
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
            data.TileSurfaceAreaCM2 = profile.TileSurfaceAreaCM2;
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
        profile.ArmorSchemaHealthProtection = ReadArmorSchemaHealthProtection(profile.ArmorClass);
        if (profile.IsSoftArmor)
        {
            profile.MaterialID = "kevlar";
            profile.MaterialType = "SoftArmor";
            return;
        }

        // Unity test helmet: 0.01016 m = 10.16 mm.
        if (profile.ArmorClass.Contains("BallisticHelmet"))
        {
            profile.Krupp = 3600.0;
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
    }

    protected static bool ApplyVersion2TestValues()
    {
        if (s_File.Version >= 9)
            return false;

        foreach (ArPenArmorProfile profile : s_File.Profiles)
        {
            ApplyClassDefaults(profile);
            ApplyVanillaTestDefaults(profile);
        }

        s_File.Version = 9;
        Print("[ArPen] Migrated armor profiles to schema-health protection model v9");
        return true;
    }

    protected static int AddVanillaTestProfiles()
    {
        int added;
        added += AddVanillaArmor("BallisticVest_ColorBase", "III", 2000.0, 800.0, 24.0, "silicon_carbide", "Ceramic");
        added += AddVanillaArmor("BallisticVest_Black", "III", 2000.0, 800.0, 24.0, "silicon_carbide", "Ceramic");
        added += AddVanillaArmor("BallisticVest_Green", "III", 2000.0, 800.0, 24.0, "silicon_carbide", "Ceramic");
        added += AddVanillaArmor("BallisticVest_Olive", "III", 2000.0, 800.0, 24.0, "silicon_carbide", "Ceramic");
        added += AddVanillaArmor("PlateCarrierVest", "IV", 2400.0, 900.0, 25.0, "silicon_carbide", "Ceramic");
        added += AddVanillaArmor("BallisticHelmet_ColorBase", "IIIA", 3600.0, 100.0, 10.16, "uhmwpe", "Polymer");
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

    protected static float ReadArmorSchemaHealthProtection(string armorClass)
    {
        string healthPath = "CfgVehicles " + armorClass + " DamageSystem GlobalArmor Projectile Health damage";
        if (!GetGame().ConfigIsExisting(healthPath))
            return 1.0;
        return Math.Clamp(1.0 - GetGame().ConfigGetFloat(healthPath), 0.0, 1.0);
    }

    protected static bool IsSoftArmorClass(string armorClass)
    {
        return armorClass.Contains("Kevlar") || armorClass.Contains("SoftArmor") || armorClass.Contains("PressVest") || armorClass.Contains("PoliceVest");
    }

    protected static void ApplyVanillaTestDefaults(ArPenArmorProfile profile)
    {
        if (!profile)
            return;

        if (profile.ArmorClass.Contains("BallisticVest"))
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
        else if (profile.ArmorClass.Contains("PlateCarrierVest"))
        {
            profile.IsSoftArmor = false;
            profile.UseSimpleHealthScaling = false;
            profile.ArmorLevel = "IV";
            profile.Krupp = 2400.0;
            profile.ArmorHealth = 900.0;
            profile.ThicknessMM = 25.0;
            profile.MaterialID = "silicon_carbide";
            profile.MaterialType = "Ceramic";
        }
        else if (profile.ArmorClass.Contains("BallisticHelmet"))
        {
            profile.IsSoftArmor = false;
            profile.UseSimpleHealthScaling = false;
            profile.ArmorLevel = "IIIA";
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
