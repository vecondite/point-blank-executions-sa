#include <plugin.h> // Plugin-SDK version 1005 from 2026-08-14 08:10:00
#include <extensions/ScriptCommands.h>
#include <CPad.h>
#include <mini/ini.h>
#include <CHud.h>
#include <CCheat.h>

using namespace plugin;

struct Main
{
    bool wasPressed = false;
    bool shoot = false;
    unsigned int shootTime;
    bool slowed = false;
    unsigned int slowTime;
    unsigned int slowEnd;
    bool disableControls = false;
    CPed* savedPed = nullptr;
    float howSlow = 0.3f;
    bool switchPistol = false;
    unsigned int switchPistolTime;
    bool switchToUnarmed = false;
    unsigned int switchToUnarmedTime;
    unsigned int giveBackTime;

    float radius, dotProductLowLim, dotProductUpLim;
    unsigned int shootDelay, switchPistolDelay, switchUnarmedDelay, giveBackControlDelay, slowDelay, slowDuration, keyBind;
    bool cinematicCam;
    float camOffsetX, camOffsetY, camOffsetZ;

    std::string cheatCode, shotAnimName, shotIfpName;

    void readConfig() {
        mINI::INIFile file(PLUGIN_PATH("config.ini"));
        mINI::INIStructure ini;
        file.read(ini);
        keyBind = std::stoul(ini["settings"]["keyBind"], nullptr, 0);
        howSlow = std::stof(ini["settings"]["slowness"]);
        shootDelay = std::stoi(ini["settings"]["shootDelay"]);
        switchPistolDelay = std::stoi(ini["settings"]["switchPistolDelay"]);
        switchUnarmedDelay = std::stoi(ini["settings"]["switchUnarmedDelay"]);
        giveBackControlDelay = std::stoi(ini["settings"]["giveBackControlDelay"]);
        slowDelay = std::stoi(ini["settings"]["slowDelay"]);
        slowDuration = std::stoi(ini["settings"]["slowDuration"]);
        radius = std::stof(ini["settings"]["radius"])*std::stof(ini["settings"]["radius"]);
        shotAnimName = ini["settings"]["shot-anim"];
        shotIfpName = ini["settings"]["shot-ifp"];
        cheatCode = ini["settings"]["cheatCode"];
        dotProductLowLim = std::stof(ini["settings"]["dotproduct-low-lim"]);
        dotProductUpLim = std::stof(ini["settings"]["dotproduct-up-lim"]);
        cinematicCam = std::stoi(ini["camera"]["enable"]);
        camOffsetX = std::stof(ini["camera"]["offset-x"]);
        camOffsetY = std::stof(ini["camera"]["offset-y"]);
        camOffsetZ = std::stof(ini["camera"]["offset-z"]);
        static char msg[1024];
        sprintf_s(msg, "REFRESHED CONFIG OF POINT-BLANK EXECUTIONS");
        CHud::SetHelpMessage(msg, true, false, false);
        std::reverse(cheatCode.begin(), cheatCode.end());
    }

    Main()
    {
        // register event callbacks
        readConfig();
        Events::gameProcessEvent += []{ gInstance.OnGameProcess(); };
    }

    void OnGameProcess()
    {
        CPlayerPed* player = FindPlayerPed();
        if (!player) return;

        if (!plugin::Command<0x04EE>("execute")) {
            plugin::Command<0x04ED>("execute");
            return;
        }

        if (strncmp(CCheat::m_CheatString, cheatCode.c_str(), cheatCode.length()) == 0) {
            CCheat::m_CheatString[0] = '\0';
            readConfig();
        }

        unsigned int pistolSlot = CWeaponInfo::GetWeaponInfo(WEAPONTYPE_PISTOL, 1)->m_nSlot;
        CWeapon currentPistol = player->m_aWeapons[pistolSlot];

        if(disableControls) CPad::GetPad(0)->DisablePlayerControls = true;

        unsigned int currentTime = CTimer::m_snTimeInMilliseconds;

        if (slowed && slowTime < currentTime && CTimer::ms_fTimeScale==1.0f) {
            CTimer::ms_fTimeScale = howSlow;
            slowEnd = currentTime + slowDuration*howSlow;
        }

        if (slowed && slowEnd < currentTime) {
            CTimer::ms_fTimeScale = 1.0f;
            slowed = false;
        }

        if (switchPistol && switchPistolTime < currentTime) {
            switchPistol = false;
            player->SetCurrentWeapon(currentPistol.m_eWeaponType);
        }

        if (switchToUnarmed && switchToUnarmedTime < currentTime) {
            switchToUnarmed = false;
            player->SetCurrentWeapon(WEAPONTYPE_UNARMED);
        }


        if (disableControls && giveBackTime < currentTime) {
            disableControls = false;
            plugin::Command<0x015A>();
        }

        CPed* targetPed = player->m_pPlayerTargettedPed;
        if (!targetPed) {
            if (savedPed != nullptr && savedPed && savedPed->IsAlive()) {
                targetPed = savedPed;
            }
            else {
                targetPed = nullptr;
                return;
            }
        }

        if (shoot && shootTime < currentTime) {
            shoot = false;
            CWeapon& weapon = player->m_aWeapons[player->m_nSelectedWepSlot];
            eWeaponType weapType = weapon.m_eWeaponType;
            if (!(weapType == WEAPONTYPE_PISTOL || weapType == WEAPONTYPE_PISTOL_SILENCED || weapType == WEAPONTYPE_DESERT_EAGLE)) return;
            CVector pedHeadPos;
            targetPed->GetBonePosition(pedHeadPos, 8, true);
            CVector origin;
            player->GetBonePosition(origin, 25, true);
            targetPed->UpdateRwMatrix();
            targetPed->UpdateRwFrame();
            weapon.Fire(player, &origin, &origin, targetPed, &pedHeadPos, nullptr);   
            plugin::Command<0x0829>(targetPed, shotAnimName.c_str(), shotIfpName.c_str(), 4.0f, -1);
        }
            
        bool pressed = KeyPressed(keyBind);
        if ((pressed && !wasPressed && player->m_aWeapons[player->m_nSelectedWepSlot].m_eWeaponType == WEAPONTYPE_UNARMED) && currentPistol.m_nAmmoTotal!=0) {
            CVector playerPos = player->GetPosition();
            CVector pedPos = targetPed->GetPosition();

            CVector dirToTarget;
            dirToTarget.x = pedPos.x - playerPos.x;
            dirToTarget.y = pedPos.y - playerPos.y;
            dirToTarget.z = pedPos.z - playerPos.z;

            dirToTarget.Normalize();

            CVector pedForward = targetPed->GetForward();

            float dotProduct = RwV3dDotProduct(&dirToTarget, &pedForward);

            if (dotProductLowLim < dotProduct && dotProduct < dotProductUpLim) {
                float distance = (playerPos - pedPos).MagnitudeSqr();
                if (distance < radius) {
                    if (!shoot) {
                        shoot = true;
                        shootTime = currentTime + shootDelay;
                        if (currentPistol.m_nAmmoTotal != 0) {
                            savedPed = targetPed;
                            if (cinematicCam) {
                                CMatrix playerMatrix = player->GetMatrix();
                                CVector targetPos = playerPos
                                    + (player->m_matrix->right * camOffsetX)
                                    + (player->m_matrix->up * camOffsetY)
                                    + (player->m_matrix->at * camOffsetZ);
                                plugin::Command<0x015f>(targetPos.x, targetPos.y, targetPos.z, 0.0f, 0.0f, 0.0f);
                                plugin::Command<0x0159>(targetPed, 15, 1);
                            }
                            plugin::Command<0x0792>(player);
                            plugin::Command<0x0812>(player, "execute", "execute", 4.0f, 0, 0, 0, 0, -1);
                            disableControls = true;
                            slowTime = currentTime + slowDelay;
                            slowed = true;
                            switchPistolTime = currentTime + switchPistolDelay;
                            switchPistol = true;
                            switchToUnarmedTime = currentTime + switchUnarmedDelay;
                            switchToUnarmed = true;
                            giveBackTime = currentTime + giveBackControlDelay;
                        }
                    }
                }
            }

        }
        wasPressed = pressed;
    }
} gInstance;
