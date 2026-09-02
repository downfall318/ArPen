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

        units[] = {};
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

class ArPenSettings
{
    // Emergency switch: 1 makes every ammunition type use DayZ damage.
    useLegacyFallback = 0;
};
