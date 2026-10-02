#include <plugin.h> // Plugin-SDK version 1005 from 2026-08-14 08:10:00
#include <extensions/ScriptCommands.h>
#include <CPad.h>
#include <mini/ini.h>
#include <CHud.h>
#include <CCheat.h>
#include <CWorld.h>
#include <CSprite.h>

using namespace plugin;

struct Execution {
    eWeaponType weapType;
    float radius, radiusSqr, dotProductLowLim, dotProductUpLim, howSlow;
    unsigned int shootDelay, giveBackControlDelay, slowDelay, slowDuration, targetBone, dieDelay;
    bool cinematicCam;
    float camOffsetX, camOffsetY, camOffsetZ;
    std::string shotAnimName, shotIfpName, playerIfpName, playerAnimName;
};

struct Main
{
    bool wasPressed = false, shoot = false, slowed = false, die = false, disableControls = false;
    unsigned int shootTime = 0, slowTime = 0, slowEnd = 0, giveBackTime = 0, indicatorBone = 0, dieTime = 0;
    CPed* savedPed = nullptr;
    float howSlow = 0.3f;

    float xOffset = 0.0, yOffset = 0.0, zOffset = 0.0, p1x = 0.0, p1y = 0.0, p2x = 0.0, p2y = 0.0, p3x = 0.0, p3y = 0.0, p4x = 0.0, p4y = 0.0;
    CRGBA col1, col2, col3, col4;

    unsigned int keyBind;
    std::string cheatCode;

    std::unordered_map<eWeaponType, std::vector<Execution>> weapExecutions;

    std::vector<std::string> split(const std::string& str, const std::string& del) {
        std::vector<std::string> elements;
        size_t start = 0;
        size_t end = 0;
        while ((end = str.find_first_of(del, start)) != std::string::npos) {
            if (start != end) elements.push_back(str.substr(start, end - start));
            start = end + 1;
        }
        if (start < str.length()) elements.push_back(str.substr(start));
        return elements;
    }

    std::vector<unsigned int> svtoi(const std::vector<std::string>& stringVec) {
        std::vector<unsigned int> output(stringVec.size());
        std::transform(stringVec.begin(), stringVec.end(), output.begin(), [](const std::string& s) {return std::stoi(s);});
        return output;
    }

    CRGBA getColors(const std::string& colorStr) {
        std::vector<std::string> colVals = split(colorStr, ",");
        std::vector<unsigned int> pCols = svtoi(colVals);
        return CRGBA(pCols[0], pCols[1], pCols[2], pCols[3]);
    }

    void readConfig() {
        mINI::INIFile file(PLUGIN_PATH("config.ini"));
        mINI::INIStructure ini;
        file.read(ini);
        keyBind = std::stoul(ini["settings"]["keyBind"], nullptr, 0);
        cheatCode = ini["settings"]["cheatCode"];
        std::transform(cheatCode.begin(), cheatCode.end(), cheatCode.begin(), ::toupper);
        indicatorBone = std::stoi(ini["indicator"]["bone"]);
        xOffset = std::stof(ini["indicator"]["x-offset"]);
        yOffset = std::stof(ini["indicator"]["y-offset"]);
        zOffset = std::stof(ini["indicator"]["z-offset"]);
        std::vector<std::string> rcol1, rcol2, rcol3, rcol4;
        col1 = getColors(ini["indicator"]["col1"]);
        col2 = getColors(ini["indicator"]["col2"]);
        col3 = getColors(ini["indicator"]["col3"]);
        col4 = getColors(ini["indicator"]["col4"]);
        p1x = std::stof(ini["indicator"]["p1x"]);
        p1y = std::stof(ini["indicator"]["p1y"]);
        p2x = std::stof(ini["indicator"]["p2x"]);
        p2y = std::stof(ini["indicator"]["p2y"]);
        p3x = std::stof(ini["indicator"]["p3x"]);
        p3y = std::stof(ini["indicator"]["p3y"]);
        p4x = std::stof(ini["indicator"]["p4x"]);
        p4y = std::stof(ini["indicator"]["p4y"]);

        weapExecutions.clear();

        std::ifstream weaponDetails;
        weaponDetails.open(PLUGIN_PATH("weapons.txt"));
        std::string line;
        if (weaponDetails.is_open()) {
            while (std::getline(weaponDetails, line)) {
                std::vector<std::string> values = split(line, " \t");
                if (values.size() != 19) continue;
                Execution newExecution;
                eWeaponType weapId = static_cast<eWeaponType>(std::stoi(values[0]));
                float radius = std::stof(values[1]);
                newExecution.weapType = weapId;
                newExecution.radius = radius;
                newExecution.radiusSqr = radius * radius;
                newExecution.dotProductLowLim = std::stof(values[2]);
                newExecution.dotProductUpLim = std::stof(values[3]);
                newExecution.howSlow = std::stof(values[4]);
                newExecution.shootDelay = std::stoi(values[5]);
                newExecution.giveBackControlDelay = std::stoi(values[6]);
                newExecution.slowDelay = std::stoi(values[7]);
                newExecution.slowDuration = std::stoi(values[8]);
                newExecution.cinematicCam = std::stoi(values[9]);
                newExecution.camOffsetX = std::stof(values[10]);
                newExecution.camOffsetY = std::stof(values[11]);
                newExecution.camOffsetZ = std::stof(values[12]);
                newExecution.shotIfpName = values[13];
                newExecution.shotAnimName = values[14];
                newExecution.playerIfpName = values[15];
                newExecution.playerAnimName = values[16];
                newExecution.targetBone = std::stoi(values[17]);
                newExecution.dieDelay = std::stoi(values[18]);
                weapExecutions[weapId].push_back(newExecution);
            }
            weaponDetails.close();
        }

        static char msg[1024];
        sprintf_s(msg, "Reloaded data!~n~~n~Keybind: %d~n~Cheat: %s~n~%zu weapons.", keyBind, cheatCode.c_str(), weapExecutions.size());
        CHud::SetHelpMessage(msg, true, false, false);
        std::reverse(cheatCode.begin(), cheatCode.end());
    }

    Main()
    {
        // register event callbacks
        readConfig();
        Events::gameProcessEvent += []{ gInstance.OnGameProcess(); };
        Events::drawHudEvent += [] { gInstance.OnDrawHud(); };
    }

    eWeaponType lastWeapType;
    Execution lastExec;

    bool pressed;

    void OnDrawHud() {
        if (pressed && savedPed!=nullptr) {
            CVector pedBone;
            savedPed->GetBonePosition(pedBone, indicatorBone, true);
            pedBone.x += xOffset;
            pedBone.y += yOffset;
            pedBone.z += zOffset;

            RwV3d screenPos;
            float w, h;

            if (CSprite::CalcScreenCoors(pedBone, &screenPos, &w, &h, true, true)) {
                CSprite2d::DrawAnyRect(
                    screenPos.x + p1x, screenPos.y + p1y,
                    screenPos.x + p2x, screenPos.y + p2y,
                    screenPos.x + p3x, screenPos.y + p3y,
                    screenPos.x + p4x, screenPos.y + p4y,
                    col1, col2, col3, col4
                );
            }
        }
    }

    Execution* currentExec = nullptr;

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

        eWeaponType currentWeapType = player->m_aWeapons[player->m_nSelectedWepSlot].m_eWeaponType;
        lastWeapType = currentWeapType;

        if(disableControls) CPad::GetPad(0)->DisablePlayerControls = true;

        unsigned int currentTime = CTimer::m_snTimeInMilliseconds;

        if (slowed && slowTime < currentTime && CTimer::ms_fTimeScale==1.0f) {
            CTimer::ms_fTimeScale = howSlow;
            slowEnd = currentTime + lastExec.slowDuration*howSlow;
        }

        if (slowed && slowEnd < currentTime) {
            CTimer::ms_fTimeScale = 1.0f;
            slowed = false;
        }

        if (disableControls && giveBackTime < currentTime) {
            disableControls = false;
            plugin::Command<0x015A>();
        }

        if (savedPed != nullptr) {
            if (shoot && shootTime!=0 && shootTime < currentTime) {
                shoot = false;
                CWeapon& weapon = player->m_aWeapons[player->m_nSelectedWepSlot];
                if (weapon.m_eWeaponType != lastExec.weapType) return;
                CVector targetPos;
                savedPed->GetBonePosition(targetPos, lastExec.targetBone, true);
                savedPed->UpdateRwMatrix();
                savedPed->UpdateRwFrame();
                CVector origin;
                player->GetBonePosition(origin, 25, true);
                weapon.Fire(player, &origin, &origin, savedPed, &targetPos, nullptr);
            }
            if (die && dieTime < currentTime) {
                die = false;
                plugin::Command<0x0829>(savedPed, lastExec.shotAnimName.c_str(), lastExec.shotIfpName.c_str(), 4.0f, -1);
                savedPed = nullptr;
                shoot = false;
            }
        }

        CVector playerPos = player->GetPosition();
        CVector pedPos;

        pressed = KeyPressed(keyBind);
        if (pressed) {
            std::unordered_map<eWeaponType, std::vector<Execution>>::iterator it = weapExecutions.find(lastWeapType);
            if (it == weapExecutions.end()) return;
            std::vector<Execution>& exec = it->second;
            float maxRadius = 0.0;
            for (Execution ex : exec) {
                if (ex.radius > maxRadius) maxRadius = ex.radius;
            }
            CVector playerOrigin;
            player->GetBonePosition(playerOrigin, 8, true);
            CVector forwardPlayer = player->GetForward();
            forwardPlayer.Normalize();
            CVector target = playerOrigin + (forwardPlayer * maxRadius);
            CColPoint colPoint;
            CEntity* hitEntity = nullptr;
            bool collision = CWorld::ProcessLineOfSight(playerOrigin, target, colPoint, hitEntity, true, true, true, true, false, true, false, true);

            CPed* foundPed = nullptr;

            if (collision && hitEntity != nullptr && hitEntity->m_nType == ENTITY_TYPE_PED) {
                CPed* hitPed = static_cast<CPed*>(hitEntity);
                if (hitPed != player) foundPed = hitPed;
            }

            if (foundPed == nullptr || !foundPed->IsAlive()) {
                if(!shoot && !die) savedPed = nullptr;
            }

            if (foundPed) {
                pedPos = foundPed->GetPosition();

                CVector dirToTarget;
                dirToTarget.x = pedPos.x - playerPos.x;
                dirToTarget.y = pedPos.y - playerPos.y;
                dirToTarget.z = pedPos.z - playerPos.z;

                dirToTarget.Normalize();

                CVector pedForward = foundPed->GetForward();

                float dotProduct = RwV3dDotProduct(&dirToTarget, &pedForward);

                currentExec = nullptr;

                for (Execution& ex : exec) {
                    if (ex.dotProductLowLim < dotProduct && dotProduct < ex.dotProductUpLim) {
                        currentExec = &ex;
                        break;
                    }
                }

                if (currentExec) {
                    lastExec = *currentExec;

                    float distance = (playerPos - pedPos).MagnitudeSqr();
                    if (distance < currentExec->radiusSqr) {
                        if(!shoot && !die) savedPed = foundPed;
                    }
                    else {
                        if(!shoot && !die) savedPed = nullptr;
                    }
                }
            }
        }
        if (!pressed && wasPressed) {
            if (savedPed!=nullptr && currentExec != nullptr) {
                Execution e = *currentExec;
                if (!shoot && !die) {
                    shoot = e.shootDelay != 0;
                    shootTime = shoot ? currentTime + e.shootDelay : 0;
                    die = true;
                    dieTime = currentTime + e.dieDelay;
                    unsigned int weapSlot = CWeaponInfo::GetWeaponInfo(e.weapType, 1)->m_nSlot;
                    CWeapon weap = player->m_aWeapons[weapSlot];
                    if (weap.m_nAmmoTotal != 0 || weap.m_eWeaponType == WEAPONTYPE_UNARMED) {
                        if (e.cinematicCam) {
                            CMatrix playerMatrix = player->GetMatrix();
                            CVector targetPos = playerPos
                                + (player->m_matrix->right * e.camOffsetX)
                                + (player->m_matrix->up * e.camOffsetY)
                                + (player->m_matrix->at * e.camOffsetZ);
                            plugin::Command<0x015f>(targetPos.x, targetPos.y, targetPos.z, 0.0f, 0.0f, 0.0f);
                            plugin::Command<0x0159>(savedPed, 15, 1);
                        }
                        plugin::Command<0x0792>(player);
                        plugin::Command<0x0812>(player, e.playerAnimName.c_str(), e.playerIfpName.c_str(), 4.0f, 0, 0, 0, 0, -1);
                        if (savedPed->m_pIntelligence) {
                            savedPed->m_pIntelligence->FlushImmediately(false);
                        }
                        disableControls = true;
                        slowTime = currentTime + e.slowDelay;
                        slowed = true;
                        giveBackTime = currentTime + e.giveBackControlDelay;
                    }
                }
            }

        }
        wasPressed = pressed;
    }
} gInstance;
