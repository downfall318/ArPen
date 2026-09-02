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
};

class ArPenAmmoProfileFile
{
    int Version = 1;
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

        int added = AddTestProfiles();
        if (!FileExist(FILE_PATH) || added > 0)
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
        added += AddProfile("Bullet_308Win_AP", "7.62x51mm M993", 930.0, 0.0127006, 0.419, 7.82, false, 70.0, 170.0, 1.35);
        added += AddProfile("Bullet_556x45_AP", "5.56x45mm M996 NATO", 1014.984, 0.0034, 0.260, 5.71, false, 50.0, 170.0, 1.25);
        added += AddProfile("Bullet_50AE", ".50 AE", 450.0, 0.019, 0.12, 12.71, false, 50.0, 170.0, 1.0);
        added += AddProfile("Bullet_762x39", "7.62x39mm FMJ", 715.0, 0.008, 0.295, 7.62, false, 55.0, 165.0, 1.0);
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
        s_File.Profiles.Insert(profile);
        return 1;
    }

    protected static void Save()
    {
        MakeDirectory(DIRECTORY);
        string saveError;
        if (!JsonFileLoader<ArPenAmmoProfileFile>.SaveFile(FILE_PATH, s_File, saveError))
            ErrorEx("[ArPen] Ammo profile save failed: " + saveError);
    }
};
