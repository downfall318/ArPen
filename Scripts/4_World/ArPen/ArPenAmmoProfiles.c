class ArPenAmmoProfile
{
    string AmmoClass;
    string DisplayName;
    bool Enabled = true;
    bool UseLegacyFallback;
    float InitialVelocity;
    float BulletMassKG;
    float BallisticCoefficient;
    float CaliberMM;
    bool IsTracer;
    float BaseDamage;
    float Decibels;
    float PenetrationMultiplier = 1.0;
    float BloodDamageMultiplier = 0.5;
    float ShockDamageMultiplier = 1.0;
    float BluntShockMultiplier = 0.25;
    string ThreatLevel = "Unrated";
    float ReferenceThreatEnergyJ;
};

class ArPenAmmoProfileFile
{
    int Version = 4;
    ref array<ref ArPenAmmoProfile> Profiles;
    void ArPenAmmoProfileFile() { Profiles = new array<ref ArPenAmmoProfile>; }
};

class ArPenAmmoProfiles
{
    protected static const string DIRECTORY = "$profile:ArPen";
    protected static const string FILE_PATH = "$profile:ArPen/AmmoProfiles.json";
    protected static ref ArPenAmmoProfileFile s_File;
    protected static bool s_Loaded;

    static void Initialize()
    {
        if (s_Loaded)
            return;
        s_Loaded = true;
        s_File = new ArPenAmmoProfileFile();

        if (FileExist(FILE_PATH))
        {
            string loadError;
            ArPenAmmoProfileFile loadedFile;
            if (JsonFileLoader<ArPenAmmoProfileFile>.LoadFile(FILE_PATH, loadedFile, loadError) && loadedFile)
                s_File = loadedFile;
            else
                ErrorEx("[ArPen] Ammo profile load failed: " + loadError);
        }

        if (!s_File.Profiles)
            s_File.Profiles = new array<ref ArPenAmmoProfile>;

        bool migrated = ApplyThreatMetadata();
        int added = AddTestProfiles();
        if (!FileExist(FILE_PATH) || added > 0 || migrated)
            Save();
        Print("[ArPen] Loaded " + s_File.Profiles.Count().ToString() + " ammo profiles; added " + added.ToString());
    }

    static bool GetAmmoData(string ammoClass, out ArPenAmmoData data)
    {
        Initialize();
        foreach (ArPenAmmoProfile profile : s_File.Profiles)
        {
            if (!profile || profile.AmmoClass != ammoClass || !profile.Enabled || profile.UseLegacyFallback)
                continue;
            data = new ArPenAmmoData();
            data.Enabled = profile.Enabled;
            data.UseLegacyFallback = profile.UseLegacyFallback;
            data.InitialVelocity = profile.InitialVelocity;
            data.BulletMassKG = profile.BulletMassKG;
            data.BallisticCoefficient = profile.BallisticCoefficient;
            data.CaliberMM = profile.CaliberMM;
            data.IsTracer = profile.IsTracer;
            data.BaseDamage = profile.BaseDamage;
            data.Decibels = profile.Decibels;
            data.PenetrationMultiplier = profile.PenetrationMultiplier;
            data.BloodDamageMultiplier = profile.BloodDamageMultiplier;
            data.ShockDamageMultiplier = profile.ShockDamageMultiplier;
            data.BluntShockMultiplier = profile.BluntShockMultiplier;
            data.ThreatLevel = profile.ThreatLevel;
            data.ReferenceThreatEnergyJ = profile.ReferenceThreatEnergyJ;
            return data.InitialVelocity > 0.0 && data.BulletMassKG > 0.0 && data.CaliberMM > 0.0;
        }
        return false;
    }

    protected static int AddTestProfiles()
    {
        int added;
        added += AddProfile("Bullet_556x45", "5.56x45mm NATO", 940.0, 0.004, 0.349, 5.71, false, 45.0, 160.0, 1.0);
        added += AddProfile("Bullet_556x45_Tracer", "5.56x45mm NATO Tracer", 940.0, 0.004, 0.349, 5.71, true, 45.0, 160.0, 1.0);
        added += AddProfile("Bullet_308Win", "7.62x51mm NATO", 853.0, 0.00947, 0.393, 7.82, false, 65.0, 170.0, 1.0);
        added += AddProfile("Bullet_308Win_Tracer", "7.62x51mm NATO Tracer", 853.0, 0.00947, 0.393, 7.82, true, 65.0, 170.0, 1.0);
        added += AddProfile("Bullet_308Win_Subsonic", "7.62x51mm SubSonic", 325.0, 0.013, 0.518, 7.82, false, 25.0, 157.0, 1.0);
        added += AddProfile("Bullet_308Win_AP", "7.62x51mm M993 AP", 930.0, 0.0082, 0.400, 7.82, false, 70.0, 170.0, 1.75);
        added += AddProfile("Bullet_556x45_AP", "5.56x45mm M995 AP", 1000.0, 0.0034, 0.304, 5.71, false, 50.0, 170.0, 2.10);
        added += AddProfile("Bullet_50AE", ".50 AE", 450.0, 0.019, 0.12, 12.71, false, 50.0, 170.0, 1.0);
        added += AddProfile("Bullet_762x39", "7.62x39mm FMJ", 715.0, 0.008, 0.295, 7.62, false, 55.0, 165.0, 1.0);
        added += AddProfile("Bullet_22", ".22 LR", 370.0, 0.0026, 0.13, 5.7, false, 12.0, 140.0, 0.35);
        added += AddProfile("Bullet_380", ".380 ACP", 300.0, 0.00615, 0.16, 9.0, false, 20.0, 150.0, 0.70);
        added += AddProfile("Bullet_9x19", "9x19mm", 375.0, 0.0080, 0.16, 9.01, false, 26.0, 159.0, 0.80);
        added += AddProfile("Bullet_45ACP", ".45 ACP", 260.0, 0.0149, 0.20, 11.43, false, 34.0, 158.0, 0.80);
        added += AddProfile("Bullet_357", ".357 Magnum", 440.0, 0.0102, 0.20, 9.07, false, 45.0, 164.0, 0.95);
        added += AddProfile("Bullet_545x39", "5.45x39mm", 880.0, 0.00343, 0.30, 5.62, false, 42.0, 160.0, 1.0);
        added += AddProfile("Bullet_545x39_Tracer", "5.45x39mm Tracer", 880.0, 0.00343, 0.30, 5.62, true, 42.0, 160.0, 1.0);
        added += AddProfile("Bullet_762x54", "7.62x54mmR", 830.0, 0.0096, 0.40, 7.92, false, 70.0, 170.0, 1.10);
        added += AddProfile("Bullet_762x54_Tracer", "7.62x54mmR Tracer", 830.0, 0.0096, 0.40, 7.92, true, 70.0, 170.0, 1.10);
        added += AddProfile("Bullet_9x39", "9x39mm", 295.0, 0.0160, 0.31, 9.25, false, 55.0, 155.0, 1.10);
        added += AddProfile("Bullet_9x39AP", "9x39mm AP", 310.0, 0.0160, 0.31, 9.25, false, 58.0, 155.0, 1.30);
        // Pellet profile values are per pellet, not the mass/caliber of the shell.
        added += AddProfile("Bullet_12GaugePellets", "12 Gauge Buckshot Pellet", 400.0, 0.0035, 0.045, 8.38, false, 34.0, 165.0, 0.65);
        added += AddProfile("Bullet_12GaugeSlug", "12 Gauge Slug", 450.0, 0.0280, 0.07, 18.5, false, 110.0, 170.0, 0.90);
        added += AddProfile("Bullet_12GaugeRubberSlug", "12 Gauge Rubber Slug", 220.0, 0.0060, 0.05, 18.5, false, 10.0, 145.0, 0.20);
        return added;
    }

    protected static int AddProfile(string ammoClass, string displayName, float velocity, float mass, float bc, float caliberMM, bool tracer, float damage, float decibels, float penMultiplier)
    {
        foreach (ArPenAmmoProfile existing : s_File.Profiles)
        {
            if (existing && existing.AmmoClass == ammoClass)
                return 0;
        }
        ArPenAmmoProfile profile = new ArPenAmmoProfile();
        profile.AmmoClass = ammoClass;
        profile.DisplayName = displayName;
        profile.InitialVelocity = velocity;
        profile.BulletMassKG = mass;
        profile.BallisticCoefficient = bc;
        profile.CaliberMM = caliberMM;
        profile.IsTracer = tracer;
        profile.BaseDamage = damage;
        profile.Decibels = decibels;
        profile.PenetrationMultiplier = penMultiplier;
        AssignThreatMetadata(profile);
        s_File.Profiles.Insert(profile);
        return 1;
    }

    protected static bool ApplyThreatMetadata()
    {
        if (s_File.Version >= 4)
            return false;

        foreach (ArPenAmmoProfile profile : s_File.Profiles)
        {
            ApplyCorrectedVanillaValues(profile);
            AssignThreatMetadata(profile);
        }

        s_File.Version = 4;
        Print("[ArPen] Migrated vanilla ammunition values and threat metadata v4");
        return true;
    }

    protected static void AssignThreatMetadata(ArPenAmmoProfile profile)
    {
        if (!profile)
            return;

        profile.ReferenceThreatEnergyJ = 0.5 * profile.BulletMassKG * profile.InitialVelocity * profile.InitialVelocity;
        profile.ThreatLevel = "Unrated";

        if (profile.AmmoClass.Contains("556x45_AP") || profile.AmmoClass.Contains("308Win_AP"))
            profile.ThreatLevel = "IV";
        else if (profile.AmmoClass == "Bullet_22")
            profile.ThreatLevel = "Sub-IIA";
        else if (profile.AmmoClass.Contains("RubberSlug"))
            profile.ThreatLevel = "Sub-IIA";
        else if (profile.AmmoClass.Contains("380"))
            profile.ThreatLevel = "IIA";
        else if (profile.AmmoClass.Contains("9x19") || profile.AmmoClass.Contains("45ACP") || profile.AmmoClass.Contains("357"))
            profile.ThreatLevel = "II";
        else if (profile.AmmoClass.Contains("12Gauge"))
            profile.ThreatLevel = "Unrated-Shotgun";
        else if (profile.AmmoClass.Contains("50AE"))
            profile.ThreatLevel = "IIIA+";
        else if (profile.AmmoClass.Contains("308Win_Subsonic"))
            profile.ThreatLevel = "Special";
        else if (profile.AmmoClass.Contains("762x54"))
            profile.ThreatLevel = "III+";
        else if (profile.AmmoClass.Contains("556x45") || profile.AmmoClass.Contains("545x39") || profile.AmmoClass.Contains("308Win") || profile.AmmoClass.Contains("762x39") || profile.AmmoClass.Contains("9x39"))
            profile.ThreatLevel = "III";
    }

    protected static void ApplyCorrectedVanillaValues(ArPenAmmoProfile profile)
    {
        if (profile.AmmoClass == "Bullet_308Win_AP")
        {
            profile.DisplayName = "7.62x51mm M993 AP";
            profile.InitialVelocity = 930.0;
            profile.BulletMassKG = 0.0082;
            profile.BallisticCoefficient = 0.400;
            profile.PenetrationMultiplier = 1.75;
        }
        else if (profile.AmmoClass == "Bullet_556x45_AP")
        {
            profile.DisplayName = "5.56x45mm M995 AP";
            profile.InitialVelocity = 1000.0;
            profile.BulletMassKG = 0.0034;
            profile.BallisticCoefficient = 0.304;
            profile.PenetrationMultiplier = 2.10;
        }
        else if (profile.AmmoClass == "Bullet_12GaugePellets")
        {
            profile.DisplayName = "12 Gauge Buckshot Pellet";
            profile.BulletMassKG = 0.0035;
            profile.BallisticCoefficient = 0.045;
            profile.CaliberMM = 8.38;
        }
    }

    protected static void Save()
    {
        MakeDirectory(DIRECTORY);
        string saveError;
        if (!JsonFileLoader<ArPenAmmoProfileFile>.SaveFile(FILE_PATH, s_File, saveError))
            ErrorEx("[ArPen] Ammo profile save failed: " + saveError);
    }
};
