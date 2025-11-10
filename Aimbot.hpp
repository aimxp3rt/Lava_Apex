#define Assert( _exp ) ((void)0)
#pragma once
#include "Structs.hpp"
#include "Interface.hpp"
#include "globals.hpp"
#include "Offsets.hpp"
#include "Config.hpp"
#include "OS-ImGui/imgui/imgui.h"
#include <Windows.h>
#include <cmath>
#include <algorithm>

namespace Aimbot {
    // Offsets importants pour l'aimbot
    constexpr uint64_t OFF_VIEW_ANGLES = 0x25f0 - 0x14;        // m_localAngles
    constexpr uint64_t OFF_BREATH_ANGLES(OFF_VIEW_ANGLES - 0x10);
    constexpr uint64_t OFF_VELOCITY = 0x0170;        // m_vecVelocity
    constexpr uint64_t OFF_LAST_VISIBLE_TIME = 0x1a54;       // lastVisibleTime
    constexpr uint64_t OFF_GLOBAL_VARS = 0x1f59cc0;    // GlobalVars structure
    constexpr uint64_t OFF_BLEEDOUT_STATE = 0x2920;       // m_bleedoutState

    // FIXED: Fonction correcte pour récupérer une entité par ID
    inline uint64_t GetEntityById(int index, uint64_t base) {
        if (index < 0 || index >= 100) return 0;

        try {
            // Lire la liste d'entités
            const uint64_t entityList = mem::Read<uint64_t>(base_address + OFF_ENTITY_LIST);
            if (!entityList) return 0;

            // Lire l'entité à l'index (chaque entrée fait 0x20 bytes)
            const uint64_t entity = mem::Read<uint64_t>(entityList + (static_cast<uint64_t>(index) * 0x20));

            return entity;
        }
        catch (...) {
            return 0;
        }
    }

    // Conversion de BoneId vers les IDs corrects
    inline int GetTargetBoneId(int targetBone) {
        switch (targetBone) {
        case 0: return Head;    // Head
        case 1: return Neck;    // Neck
        case 2: return LowerChest;   // Chest (default)
        default: return LowerChest;
        }
    }

    inline uintptr_t GetEntityBoneArray(uintptr_t entity)
    {

        return mem::Read<uintptr_t>(entity + OFF_BoneMatrix);
    }

    struct matrix3x4_t
    {
        matrix3x4_t() {}
        matrix3x4_t(
            float m00, float m01, float m02, float m03,
            float m10, float m11, float m12, float m13,
            float m20, float m21, float m22, float m23)
        {
            m_flMatVal[0][0] = m00;	m_flMatVal[0][1] = m01; m_flMatVal[0][2] = m02; m_flMatVal[0][3] = m03;
            m_flMatVal[1][0] = m10;	m_flMatVal[1][1] = m11; m_flMatVal[1][2] = m12; m_flMatVal[1][3] = m13;
            m_flMatVal[2][0] = m20;	m_flMatVal[2][1] = m21; m_flMatVal[2][2] = m22; m_flMatVal[2][3] = m23;
        }

        float* operator[](int i) { Assert((i >= 0) && (i < 3)); return m_flMatVal[i]; }
        const float* operator[](int i) const { Assert((i >= 0) && (i < 3)); return m_flMatVal[i]; }
        float* Base() { return &m_flMatVal[0][0]; }
        const float* Base() const { return &m_flMatVal[0][0]; }

        float m_flMatVal[3][4];
    };

    constexpr uint64_t OFF_STUDIO_HDR = 0xff0;
    inline Vector3 GetAimbotBonePosition(uintptr_t entity, int id, Vector3 origin)
    {

        //BoneByHitBox
        uint64_t Model = mem::Read<uint64_t>(entity + OFF_STUDIO_HDR);

        //get studio hdr
        uint64_t StudioHdr = mem::Read<uint64_t>(Model + 0x8);

        //get hitbox array
        uint16_t HitboxCache = mem::Read<uint16_t>(StudioHdr + 0x34);
        uint64_t HitBoxsArray = StudioHdr + ((uint16_t)(HitboxCache & 0xFFFE) << (4 * (HitboxCache & 1)));

        uint16_t IndexCache = mem::Read<uint16_t>(HitBoxsArray + 0x4);
        int HitboxIndex = ((uint16_t)(IndexCache & 0xFFFE) << (4 * (IndexCache & 1)));

        uint16_t Bone = mem::Read<uint16_t>(HitBoxsArray + HitboxIndex + (id * 0x20));

        if (Bone < 0 || Bone > 255)
            return Vector3();

        //hitpos
        uint64_t BoneArray = GetEntityBoneArray(entity);

        matrix3x4_t Matrix = mem::Read<matrix3x4_t>(BoneArray + Bone * sizeof(matrix3x4_t));

        return Vector3(Matrix.m_flMatVal[0][3] + origin.x, Matrix.m_flMatVal[1][3] + origin.y, Matrix.m_flMatVal[2][3] + origin.z);
    }

    // Calculate angle between two vectors
    inline Vector2 CalcAngle(const Vector3& from, const Vector3& to) {
        Vector3 delta;
        delta.x = to.x - from.x;
        delta.y = to.y - from.y;
        delta.z = to.z - from.z;

        const float hyp = std::sqrt(delta.x * delta.x + delta.y * delta.y);

        Vector2 angle;
        angle.x = std::atan2(-delta.z, hyp) * (180.0f / 3.14159265f);
        angle.y = std::atan2(delta.y, delta.x) * (180.0f / 3.14159265f);

        return angle;
    }

    // Normalize angle to [-180, 180]
    inline float NormalizeAngle(float angle) {
        while (angle > 180.0f) angle -= 360.0f;
        while (angle < -180.0f) angle += 360.0f;
        return angle;
    }

    // Calculate FOV distance
    inline float GetFov(const Vector2& viewAngles, const Vector2& aimAngles) {
        Vector2 delta;
        delta.x = NormalizeAngle(aimAngles.x - viewAngles.x);
        delta.y = NormalizeAngle(aimAngles.y - viewAngles.y);

        return std::sqrt(delta.x * delta.x + delta.y * delta.y);
    }

    // Check if entity is visible
    inline bool IsVisible(uint64_t entity) {
        if (!cfg.aimbotVisibleOnly) return true;

        try {
            const float lastVisTime = mem::Read<float>(entity + OFF_LAST_VISIBLE_TIME);
            const float currentTime = mem::Read<float>(base_address + OFF_GLOBAL_VARS);
            return (currentTime - lastVisTime) < 2.0f;
        }
        catch (...) {
            return true;
        }
    }

    // Check if entity is knocked
    inline bool IsKnocked(uint64_t entity) {
        try {
            const int bleedoutState = mem::Read<int>(entity + OFF_BLEEDOUT_STATE);
            return bleedoutState > 0;
        }
        catch (...) {
            return false;
        }
    }

    // Smooth angle interpolation
    inline Vector2 SmoothAngles(const Vector2& current, const Vector2& target, float smoothness) {
        if (smoothness < 1.0f) smoothness = 1.0f;

        Vector2 delta;
        delta.x = NormalizeAngle(target.x - current.x);
        delta.y = NormalizeAngle(target.y - current.y);

        Vector2 result;
        result.x = current.x + delta.x / smoothness;
        result.y = current.y + delta.y / smoothness;

        return result;
    }

    // ULTRA DEBUG: Find best target avec TOUS les détails
    inline uint64_t FindBestTarget(uint64_t localPlayer, const Vector3& localPos,
        const Vector2& viewAngles, float& outDistance) {

        printf("\n========================================\n");
        printf("AIMBOT SCAN START\n");
        printf("========================================\n");

        if (!localPlayer) {
            printf("ERROR: No local player!\n");
            return 0;
        }
        printf("LocalPlayer: 0x%llX\n", localPlayer);

        // Lire EntityList
        const uint64_t entityList = mem::Read<uint64_t>(base_address + OFF_ENTITY_LIST);
        printf("EntityList: 0x%llX\n", entityList);
        if (!entityList) {
            printf("ERROR: EntityList is NULL!\n");
            return 0;
        }

        const int localTeam = mem::Read<int>(localPlayer + OFF_iTeamNum);
        printf("Local Team: %d\n", localTeam);
        printf("Local Pos: (%.1f, %.1f, %.1f)\n", localPos.x, localPos.y, localPos.z);
        printf("\nSettings:\n");
        printf("- FOV: %.0f°\n", cfg.aimbotFOV);
        printf("- Max Distance: %.0fm\n", cfg.maxDistance);
        printf("- Team Check: %s\n", cfg.aimbotTeamCheck ? "ON" : "OFF");
        printf("- Visibility Check: %s\n", cfg.aimbotVisibleOnly ? "ON" : "OFF");
        printf("- Target Bone: %d\n", cfg.targetBone);
        printf("\n");

        uint64_t bestTarget = 0;
        float bestFov = cfg.aimbotFOV;
        outDistance = 0.0f;

        int totalScanned = 0;
        int validEntities = 0;
        int sameTeam = 0;
        int dead = 0;
        int knocked = 0;
        int notVisible = 0;
        int tooFar = 0;
        int outsideFOV = 0;

        // Scanner TOUS les slots
        for (int i = 0; i < 100; i++) {
            const uint64_t entity = GetEntityById(i, base_address);
            if (!entity || entity == 0) continue;

            totalScanned++;

            // Ne pas viser soi-même
            if (entity == localPlayer) {
                printf("Slot %d: SELF (skip)\n", i);
                continue;
            }

            // Team check
            int entityTeam = -1;
            try {
                entityTeam = mem::Read<int>(entity + OFF_iTeamNum);
            }
            catch (...) {
                printf("Slot %d: FAILED to read team\n", i);
                continue;
            }

            if (cfg.aimbotTeamCheck && entityTeam == localTeam) {
                sameTeam++;
                continue;
            }

            // Alive check
            int lifeState = -1;
            try {
                lifeState = mem::Read<int>(entity + OFF_lifeState);
            }
            catch (...) {
                printf("Slot %d: FAILED to read lifeState\n", i);
                continue;
            }

            if (lifeState != 0) {
                dead++;
                continue;
            }

            // Knocked check
            if (IsKnocked(entity)) {
                knocked++;
                continue;
            }

            validEntities++;

            // Get position
            Vector3 entityPos;
            try {
                entityPos = mem::Read<Vector3>(entity + OFF_VecAbsOrigin);
            }
            catch (...) {
                printf("Slot %d: FAILED to read position\n", i);
                continue;
            }

            // Distance check
            const float distance = localPos.Distance(entityPos) / 39.37f;

            printf("Slot %d: VALID - Team=%d, Dist=%.1fm, Pos=(%.0f,%.0f,%.0f)\n",
                i, entityTeam, distance, entityPos.x, entityPos.y, entityPos.z);

            if (distance > cfg.maxDistance) {
                printf("  -> TOO FAR (max: %.0fm)\n", cfg.maxDistance);
                tooFar++;
                continue;
            }

            if (distance < 1.0f) {
                printf("  -> TOO CLOSE\n");
                continue;
            }

            // Visibility check
            if (!IsVisible(entity)) {
                printf("  -> NOT VISIBLE\n");
                notVisible++;
                continue;
            }

            // Get target bone
            const int targetBoneId = GetTargetBoneId(cfg.targetBone);
            const Vector3 bonePos = GetAimbotBonePosition(entity, targetBoneId, entityPos); // <- fixed

            if (bonePos.x == 0.0f && bonePos.y == 0.0f && bonePos.z == 0.0f) {
                printf("  -> INVALID BONE POSITION\n");
                continue;
            }

            printf("  -> Bone Pos: (%.0f,%.0f,%.0f)\n", bonePos.x, bonePos.y, bonePos.z);

            // Calculate FOV
            const Vector2 aimAngles = CalcAngle(localPos, bonePos);
            const float fov = GetFov(viewAngles, aimAngles);

            printf("  -> FOV: %.2f° (max: %.0f°)\n", fov, cfg.aimbotFOV);

            if (fov > cfg.aimbotFOV) {
                printf("  -> OUTSIDE FOV\n");
                outsideFOV++;
                continue;
            }

            if (fov < bestFov) {
                bestFov = fov;
                bestTarget = entity;
                outDistance = distance;
                printf("  -> ★★★ NEW BEST TARGET! ★★★\n");
            }
        }

        printf("\n========================================\n");
        printf("SCAN RESULTS:\n");
        printf("========================================\n");
        printf("Total Scanned: %d\n", totalScanned);
        printf("Valid Entities: %d\n", validEntities);
        printf("Rejected:\n");
        printf("  - Same Team: %d\n", sameTeam);
        printf("  - Dead: %d\n", dead);
        printf("  - Knocked: %d\n", knocked);
        printf("  - Not Visible: %d\n", notVisible);
        printf("  - Too Far: %d\n", tooFar);
        printf("  - Outside FOV: %d\n", outsideFOV);
        printf("\n");

        if (bestTarget) {
            printf("✓ BEST TARGET FOUND!\n");
            printf("  - Distance: %.2fm\n", outDistance);
            printf("  - FOV: %.2f°\n", bestFov);
        }
        else {
            printf("✗ NO TARGET FOUND\n");
            printf("\nTROUBLESHOOTING:\n");
            if (totalScanned == 0) {
                printf("  ! No entities scanned - Check if you're in a game\n");
            }
            if (validEntities == 0 && totalScanned > 0) {
                printf("  ! All entities rejected - Try disabling filters:\n");
                printf("    - Set aimbotTeamCheck = false\n");
                printf("    - Set aimbotVisibleOnly = false\n");
            }
            if (tooFar > 0) {
                printf("  ! %d targets too far - Increase maxDistance\n", tooFar);
            }
            if (outsideFOV > 0) {
                printf("  ! %d targets outside FOV - Increase aimbotFOV\n", outsideFOV);
            }
        }
        printf("========================================\n\n");

        return bestTarget;
    }

    // Aim at target
    inline void AimAtTarget(uint64_t localPlayer, uint64_t target) {
        if (!localPlayer || !target) return;

        try {
            const Vector3 localPos = mem::Read<Vector3>(localPlayer + OFF_VecAbsOrigin);
            const int targetBoneId = GetTargetBoneId(cfg.targetBone);
            const Vector3 targetOrigin = mem::Read<Vector3>(target + OFF_VecAbsOrigin);
            const Vector3 targetPos = GetAimbotBonePosition(target, targetBoneId, targetOrigin);

            if (targetPos.x == 0.0f && targetPos.y == 0.0f && targetPos.z == 0.0f) {
                return;
            }

            const Vector2 targetAngles = CalcAngle(localPos, targetPos);
            const uint64_t viewAnglesPtr = localPlayer + OFF_VIEW_ANGLES;

            Vector2 currentAngles;
            currentAngles.x = mem::Read<float>(viewAnglesPtr);
            currentAngles.y = mem::Read<float>(viewAnglesPtr);

            Vector2 finalAngles = SmoothAngles(currentAngles, targetAngles, cfg.aimbotSmooth);
            finalAngles.x = std::clamp(finalAngles.x, -89.0f, 89.0f);
            finalAngles.y = NormalizeAngle(finalAngles.y);

            mem::write<float>(viewAnglesPtr, finalAngles.x);
            mem::write<float>(viewAnglesPtr, finalAngles.y);
        }
        catch (...) {
            // Silent fail
        }
    }

    // Main aimbot update
    inline void Update() {
        if (!cfg.aimbotEnabled) return;

        // Check aim key
        if (!(GetAsyncKeyState(VK_RBUTTON) & 0x8000)) {
            return;
        }

        try {
            const uint64_t localPlayer = mem::Read<uint64_t>(base_address + OFF_LOCAL_PLAYER);
            if (!localPlayer) {
                printf("Aimbot: No local player\n");
                return;
            }

            const int lifeState = mem::Read<int>(localPlayer + OFF_lifeState);
            if (lifeState != 0) return;

            const Vector3 localPos = mem::Read<Vector3>(localPlayer + OFF_VecAbsOrigin);
            const uint64_t viewAnglesPtr = localPlayer + OFF_VIEW_ANGLES;

            Vector2 viewAngles;
            viewAngles.x = mem::Read<float>(viewAnglesPtr);
            viewAngles.y = mem::Read<float>(viewAnglesPtr);

            float targetDistance = 0.0f;
            const uint64_t target = FindBestTarget(localPlayer, localPos, viewAngles, targetDistance);

            if (target) {
                AimAtTarget(localPlayer, target);
            }
        }
        catch (...) {
            printf("Aimbot: Exception caught\n");
        }
    }

    // Draw FOV circle
    inline void DrawFOVCircle() {
        if (!cfg.aimbotEnabled) return;

        const float screenCenterX = ScreenSize.x / 2.0f;
        const float screenCenterY = ScreenSize.y / 2.0f;
        const float radius = (cfg.aimbotFOV / 90.0f) * (ScreenSize.y / 4.0f);

        ImGui::GetForegroundDrawList()->AddCircle(
            ImVec2(screenCenterX, screenCenterY),
            radius,
            IM_COL32(255, 255, 255, 80),
            64,
            1.0f
        );
    }
}