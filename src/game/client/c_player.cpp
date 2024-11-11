#include "input.h"
#include "edict.h"
#include "r1/c_weapon_x.h"
#include "c_player.h"
#include "cliententitylist.h"

bool C_Player::CheckMeleeWeapon()
{
    const C_EntInfo* pInfo = g_clientEntityList->GetEntInfoPtr(m_latestMeleeWeapon);
    const C_WeaponX* pWeapon = (C_WeaponX*)pInfo->m_pEntity;

    return (pInfo->m_SerialNumber == m_latestMeleeWeapon.GetSerialNumber())
        && (pWeapon != NULL)
        && *(float*)&pWeapon->m_modVars[600] > (float)(m_currentFramePlayer__timeBase - m_melee.attackLastHitNonWorldEntity);
}

static void ApplyPerOpticScalars(C_Player* const player, JoyAngle_t* const joyAngle)
{
    if (!GamePad_UseAdvancedAdsScalarsPerScope())
        return;

    const bool isZooming = player->IsZooming();

    if (!isZooming)
        return;

    const C_WeaponX* const activeWeapon = C_BaseCombatCharacter__GetActiveWeapon(player);

    if (!activeWeapon)
        return;

    const bool fullADS = activeWeapon->m_modVars[3100];

    if (!fullADS)
        return;

    const float interpAmount = activeWeapon->HasTargetZoomFOV()
        ? activeWeapon->GetZoomFOVInterpAmount(g_ClientGlobalVariables->exactCurTime)
        : 1.0f - activeWeapon->GetZoomFOVInterpAmount(g_ClientGlobalVariables->exactCurTime);

    const float baseScalar1 = GamePad_GetAdvancedAdsScalarForOptic((WeaponScopeZoomLevel_e)activeWeapon->m_modVars[0xA0C]);
    const float baseScalar2 = GamePad_GetAdvancedAdsScalarForOptic((WeaponScopeZoomLevel_e)activeWeapon->m_modVars[0xA10]);

    const float adsScalar = ((baseScalar2 - baseScalar1) * interpAmount) + baseScalar1;

    joyAngle->rotation.x *= adsScalar;
    joyAngle->rotation.y *= adsScalar;
}

void C_Player::CurveLook(C_Player* player, CInput::UserInput_t* input, float a3, float a4, float a5, int a6, float inputSampleFrametime, bool runAimAssist, JoyAngle_t* joyAngle)
{
    C_Player__CurveLook(player, input, a3, a4, a5, a6, inputSampleFrametime, runAimAssist, joyAngle);
    ApplyPerOpticScalars(player, joyAngle);

    return;
}

void V_Player::Detour(const bool bAttach) const
{
    DetourSetup(&C_Player__CurveLook, C_Player::CurveLook, bAttach);
}
