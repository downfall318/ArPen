class CfgPatches
{
    class ArPen
    {
        requiredAddons[] =
        {
            "DZ_Data",
            "DZ_Characters_Vests",
            "DZ_Characters_Headgear",
            "DZ_Weapons_Ammunition"
        };

        units[] = {"ArPen_TiledPlateCarrierVest"};
        weapons[] = {};
    };
};

class CfgMods
{
    class ArPen
    {
        dir = "ArPen";
        name = "ArPen";
        type = "mod";

        dependencies[] =
        {
            "World",
            "Mission"
        };

        class defs
        {
            class worldScriptModule
            {
                value = "";
                files[] =
                {
                    "ArPen/Scripts/4_World"
                };
            };

            class missionScriptModule
            {
                value = "";
                files[] =
                {
                    "ArPen/Scripts/5_Mission"
                };
            };
        };
    };
};

// Optional tiled variant. Existing plate carriers retain their monolithic profile.
class CfgVehicles
{
    class PlateCarrierVest;
    class ArPen_TiledPlateCarrierVest : PlateCarrierVest
    {
        scope = 2;
        displayName = "Tiled Ceramic Plate Carrier";
        descriptionShort = "Ceramic body armor with independently damaged tiles.";
        class ArPen
        {
            enabled = 1;
            krupp = 1500;
            armorHealth = 800;
            tileDamageMultiplier = 3.0;
            thicknessMM = 24;
            armorLevel = "III";
            materialID = "silicon_carbide";
            materialType = "Ceramic";
            tiles[] = {800,800,800,800,800,800,800,800,800,800,800,800,800,800,800,800};
        };
    };
};
