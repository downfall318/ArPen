class ArPenMaterialData
{
    string ID;
    string DisplayName;
    string Family;
    float DensityGCM3;
    float AcousticImpedance;
    float BrinellHardness;
    float PlateToughnessJ;
    float BaseCrackRadiusMM;
    float GlobalCouplingLow;
    float GlobalCouplingSpread;
    float MultiHitSpread;
    float FirstHitResidualIntegrity;
    float SubsequentHitIntegrityRatio;
    float CeramicDamageExponent;
    float CrackInitiationEnergyFraction;
    float SubfloorDamageScale;
};

class ArPenMaterialProfile
{
    string ID;
    string DisplayName;
    string Family;
    float DensityGCM3;
    float AcousticImpedance;
    float BrinellHardness;
    float PlateToughnessJ = 1000.0;
    float BaseCrackRadiusMM = 25.0;
    float GlobalCouplingLow = 0.05;
    float GlobalCouplingSpread = 0.25;
    float MultiHitSpread = 1.0;
    float FirstHitResidualIntegrity = 0.75;
    float SubsequentHitIntegrityRatio = 0.333333;
    float CeramicDamageExponent = 1.75;
    float CrackInitiationEnergyFraction = 0.18;
    float SubfloorDamageScale = 0.02;
};

class ArPenMaterialProfileFile
{
    int Version = 1;
    ref array<ref ArPenMaterialProfile> Materials;

    void ArPenMaterialProfileFile()
    {
        Materials = new array<ref ArPenMaterialProfile>;
    }
};

class ArPenMaterialLibrary
{
    protected static const string DIRECTORY = "$profile:ArPen";
    protected static const string FILE_PATH = "$profile:ArPen/Materials.json";
    protected static ref ArPenMaterialProfileFile s_File;
    protected static bool s_Loaded;

    static void Initialize()
    {
        if (s_Loaded)
            return;

        s_Loaded = true;
        s_File = new ArPenMaterialProfileFile();

        if (FileExist(FILE_PATH))
        {
            string loadError;
            ArPenMaterialProfileFile loadedFile;
            if (JsonFileLoader<ArPenMaterialProfileFile>.LoadFile(FILE_PATH, loadedFile, loadError) && loadedFile)
                s_File = loadedFile;
            else
                ErrorEx("[ArPen] Material library load failed: " + loadError);
        }

        if (!s_File.Materials)
            s_File.Materials = new array<ref ArPenMaterialProfile>;

        int added;
        added += AddMaterial("boron_carbide", "Boron carbide", "Ceramic", 2.515, 35.25, 0.0, 900.0);
        added += AddMaterial("silicon_nitride", "Silicon nitride", "Ceramic", 3.05, 33.5, 0.0, 1150.0);
        added += AddMaterial("aluminum_nitride", "Aluminum nitride", "Ceramic", 3.26, 35.0, 0.0, 950.0);
        added += AddMaterial("silicon_carbide", "Silicon carbide", "Ceramic", 3.16, 37.5, 0.0, 1000.0);
        added += AddMaterial("aluminum_oxide", "Aluminum oxide", "Ceramic", 3.98, 43.0, 0.0, 1050.0);
        added += AddMaterial("titanium_diboride", "Titanium diboride", "Ceramic", 4.5, 51.3, 0.0, 1000.0);
        added += AddMaterial("uhmwpe", "UHMWPE", "Polymer", 0.95, 2.0, 0.0, 1400.0);
        added += AddMaterial("ceramic_uhmwpe", "Ceramic/UHMWPE composite", "Ceramic", 2.25, 37.5, 0.0, 1250.0);
        added += AddMaterial("ar500_steel", "AR500 steel", "Steel", 7.85, 0.0, 514.0, 1600.0);
        added += AddMaterial("ar600_steel", "600 BHN armor steel", "Steel", 7.85, 0.0, 600.0, 1200.0);
        added += AddMaterial("mild_steel", "Mild steel", "Steel", 7.85, 0.0, 150.0, 1800.0);
        added += AddMaterial("titanium_alloy", "Titanium alloy", "Metal", 4.43, 0.0, 334.0, 1500.0);
        added += AddMaterial("aluminum_alloy", "Armor aluminum", "Metal", 2.70, 0.0, 120.0, 1300.0);

        if (!FileExist(FILE_PATH) || added > 0)
            Save();

        Print("[ArPen] Loaded " + s_File.Materials.Count().ToString() + " material profiles; added " + added.ToString());
    }

    static bool Get(string id, out ArPenMaterialData data)
    {
        Initialize();
        foreach (ArPenMaterialProfile material : s_File.Materials)
        {
            if (!material || material.ID != id)
                continue;

            data = new ArPenMaterialData();
            data.ID = material.ID;
            data.DisplayName = material.DisplayName;
            data.Family = material.Family;
            data.DensityGCM3 = material.DensityGCM3;
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
            return true;
        }
        return false;
    }

    protected static int AddMaterial(string id, string displayName, string family, float density, float impedance, float hardness, float toughness)
    {
        foreach (ArPenMaterialProfile existing : s_File.Materials)
        {
            if (existing && existing.ID == id)
                return 0;
        }

        ArPenMaterialProfile material = new ArPenMaterialProfile();
        material.ID = id;
        material.DisplayName = displayName;
        material.Family = family;
        material.DensityGCM3 = density;
        material.AcousticImpedance = impedance;
        material.BrinellHardness = hardness;
        material.PlateToughnessJ = toughness;

        if (family != "Ceramic")
        {
            material.BaseCrackRadiusMM = 0.0;
            material.GlobalCouplingLow = 0.02;
            material.GlobalCouplingSpread = 0.03;
            material.MultiHitSpread = 0.05;
            material.FirstHitResidualIntegrity = 1.0;
            material.SubsequentHitIntegrityRatio = 1.0;
            material.CeramicDamageExponent = 1.0;
            material.CrackInitiationEnergyFraction = 0.0;
            material.SubfloorDamageScale = 1.0;
        }

        s_File.Materials.Insert(material);
        return 1;
    }

    protected static void Save()
    {
        MakeDirectory(DIRECTORY);
        string saveError;
        if (!JsonFileLoader<ArPenMaterialProfileFile>.SaveFile(FILE_PATH, s_File, saveError))
            ErrorEx("[ArPen] Material library save failed: " + saveError);
    }
};
