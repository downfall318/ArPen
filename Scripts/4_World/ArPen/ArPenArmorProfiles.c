class ArPenArmorProfile
{
    string ArmorClass;
    bool Enabled = true;
    float Krupp = 350.0;
    float ArmorHealth = 100.0;
    float ThicknessMM = 4.0;
    float MinHealthFactor = 0.35;
    float HealthExponent = 1.25;
    float KruppLossPerAbsorbedDamage = 0.70;
    float ItemDamagePerAbsorbedDamage = 0.35;
};

class ArPenArmorProfileFile
{
    int Version = 4;
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
        int added = EnrollLoadedArmorClasses();
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
            data.BaseKrupp = profile.Krupp;
            data.BaseArmorHealth = profile.ArmorHealth;
            data.ThicknessMM = profile.ThicknessMM;
            data.MinHealthFactor = profile.MinHealthFactor;
            data.HealthExponent = profile.HealthExponent;
            data.KruppLossPerAbsorbedDamage = profile.KruppLossPerAbsorbedDamage;
            data.ItemDamagePerAbsorbedDamage = profile.ItemDamagePerAbsorbedDamage;
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
            s_File.Profiles.Insert(profile);
            added++;
        }

        return added;
    }

    protected static void ApplyClassDefaults(ArPenArmorProfile profile)
    {
        if (!profile)
            return;

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
        }
    }

    protected static bool ApplyVersion2TestValues()
    {
        if (s_File.Version >= 4)
            return false;

        foreach (ArPenArmorProfile profile : s_File.Profiles)
            ApplyClassDefaults(profile);

        s_File.Version = 4;
        Print("[ArPen] Migrated helmet and ballistic vest profiles to Unity test values");
        return true;
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
