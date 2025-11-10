#pragma once

// ----- OFFSETS À METTRE À JOUR -----
#define OFF_VIEW_RENDER      0x40d8ab8
#define OFF_VIEW_MATRIX      0x11a350
#define OFF_LOCAL_PLAYER     0x2a7beb8
#define OFF_ENTITY_LIST      0x65fe758
#define OFF_NAME_LIST        0x9039140
#define OFF_OBSERVER_LIST    0x6600778

#define OFF_VecAbsOrigin     0x017c
#define OFF_iHealth          0x0324
#define OFF_iMaxHealth       0x0468
#define OFF_shieldHealth     0x1a0
#define OFF_shieldHealthMax  0x1a4
#define OFF_iTeamNum         0x334
#define OFF_lifeState        0x690
//#define OFF_IsDormant        0xED
#define OFF_NameIndex        0x38
#define OFF_BoneMatrix       0xda8 + 0x48

// Bone IDs pour Apex Legends
enum BoneId {
    Head = 0,
    Neck = 1,
    UpperChest = 2,
    LowerChest = 3,
    Stomach = 4,
    Hip = 5,
    Leftshoulder = 6,
    Leftelbow = 7,
    Lefthand = 8,
    Rightshoulder = 9,
    RightelbowBone = 10,
    Righthand = 11,
    LeftThighs = 12,
    Leftknees = 13,
    Leftleg = 14,
    RightThighs = 16,
    Rightknees = 17,
    Rightleg = 18,
};