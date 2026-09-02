modded class MissionServer
{
    override void OnInit()
    {
        super.OnInit();
        ArPenArmorProfiles.Initialize();
        ArPenAmmoProfiles.Initialize();
    }
};
