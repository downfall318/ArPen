modded class MissionGameplay
{
    override void OnInit()
    {
        super.OnInit();

        // DayZDiag local/offline missions do not instantiate MissionServer.
        if (GetGame().IsServer())
        {
            ArPenArmorProfiles.Initialize();
            ArPenAmmoProfiles.Initialize();
        }
    }
};
