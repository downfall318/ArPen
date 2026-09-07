modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        ArPenMaterialLibrary.Initialize();
        ArPenArmorProfiles.Initialize();
        ArPenAmmoProfiles.Initialize();
    }
};
