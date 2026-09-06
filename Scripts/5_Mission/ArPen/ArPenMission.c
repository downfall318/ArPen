modded class MissionGameplay
{
    protected ref ArPenTestHUD m_ArPenTestHUD;
    protected float m_ArPenTestElapsed;
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

    m_ArPenTestEnabled = ArPenTestSpawner.Enabled();

    if (!m_ArPenTestEnabled)
    {
        return;
    }


    array<string> bodyArmors;
    array<string> helmets;

    ArPenTestSpawner.BuildBodyArmorList(bodyArmors);
    ArPenTestSpawner.BuildHelmetList(helmets);

    m_ArPenTestPanel = new ArPenTestPanel();
    m_ArPenTestHUD = new ArPenTestHUD();
}
    // ------------------------------------------------------------
    // F5 TOGGLE
    // ------------------------------------------------------------
    override void OnKeyPress(int key)
    {
        super.OnKeyPress(key);
        if (!m_ArPenTestEnabled)
            return;
        if (key == KeyCode.KC_F6 && m_ArPenTestHUD)
            m_ArPenTestHUD.Toggle();
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
    override void OnUpdate(float timeslice)
    {
        super.OnUpdate(timeslice);
        m_ArPenTestElapsed += timeslice;
        if (m_ArPenTestHUD && m_ArPenTestElapsed >= 0.1)
        {
            m_ArPenTestElapsed = 0;
            m_ArPenTestHUD.Update();
        }
    }

    override void OnMissionFinish()
    {
        if (m_ArPenTestPanel)
        {
            m_ArPenTestPanel.Close();
            m_ArPenTestPanel = NULL;
        }
        m_ArPenTestHUD = NULL;
        if (m_ArPenTestEnabled)
            ArPenTestSpawner.Cleanup();
        ArPenTestTelemetry.Observed = NULL;
        super.OnMissionFinish();
    }
}
