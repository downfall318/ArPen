modded class MissionGameplay
{
    protected ref ArPenTestPanel m_ArPenTestPanel;
    protected bool m_ArPenTestEnabled;
    // ------------------------------------------------------------
    // INITIALIZATION
    // ------------------------------------------------------------
    override void OnInit()
{
    super.OnInit();

    // DayZDiag local/offline missions do not instantiate MissionServer.
    if (GetGame().IsServer())
    {
        ArPenArmorProfiles.Initialize();
        ArPenAmmoProfiles.Initialize();
    }

    m_ArPenTestEnabled = FileExist("$mission:ArPenTest.enable");

    if (!m_ArPenTestEnabled)
    {
        Print("[ArPenTest] Test harness disabled for this mission");
        return;
    }

    Print("[ArPenTest] ==================================");
    Print("[ArPenTest] TEST MISSION DETECTED");
    Print("[ArPenTest] F5 opens the test panel");
    Print("[ArPenTest] ==================================");

    array<string> bodyArmors;
    array<string> helmets;

    ArPenTestSpawner.BuildBodyArmorList(bodyArmors);
    ArPenTestSpawner.BuildHelmetList(helmets);

    m_ArPenTestPanel = new ArPenTestPanel();
}
    // ------------------------------------------------------------
    // F5 TOGGLE
    // ------------------------------------------------------------
    override void OnKeyPress(int key)
    {
        super.OnKeyPress(key);
        if (!m_ArPenTestEnabled)
            return;
        if (key == KeyCode.KC_F5)
        {
            if (!m_ArPenTestPanel)
            {
                m_ArPenTestPanel = new ArPenTestPanel();
            }
            m_ArPenTestPanel.Toggle();
        }
    }
    // ------------------------------------------------------------
    // CLEANUP
    // ------------------------------------------------------------
    override void OnMissionFinish()
    {
        if (m_ArPenTestPanel)
        {
            m_ArPenTestPanel.Close();
            m_ArPenTestPanel = NULL;
        }
        super.OnMissionFinish();
    }
}
