#pragma once

enum ClientOpcode : unsigned char
{
    kAuthenticationRequest = 0,
    kCancelAssignmentRequest,
    kAssignCharacterRequest,
    kClientReferencesMoveRequest,
    kEnterExteriorCellRequest,
    kEnterInteriorCellRequest,
    kRequestFactionsChanges,
    kRequestQuestUpdate,
    kRequestSetWaypoint, 
    kRequestDelWaypoint,
    kPartyInviteRequest,
    kPartyAcceptInviteRequest,
    kPartyLeaveRequest,
    kPartyCreateRequest,
    kPartyChangeLeaderRequest,
    kPartyKickRequest,
    kRequestActorValueChanges,
    kRequestActorMaxValueChanges,
    kRequestHealthChangeBroadcast,
    kActivateRequest,
    kLockChangeRequest,
    kAssignObjectsRequest,
    kRequestSpawnData,
    kRequestDeathStateChange,
    kShiftGridCellRequest,
    kRequestOwnershipTransfer,
    kRequestOwnershipClaim,
    kRequestObjectInventoryChanges,
    kRequestInventoryChanges,
    kSpellCastRequest,
    kInterruptCastRequest,
    kAddTargetRequest,
    kProjectileLaunchRequest,
    kScriptAnimationRequest,
    kDrawWeaponRequest,
    kMountRequest,
    kNewPackageRequest,
    kRequestRespawn,
    kSyncExperienceRequest,
    kRequestEquipmentChanges,
    kSendChatMessageRequest,
    kTeleportCommandRequest,
    kPlayerRespawnRequest,
    kDialogueRequest,
    kSubtitleRequest,
    kPlayerDialogueRequest,
    kPlayerLevelRequest,
    kTeleportRequest,
    kClientOpcodeMax
};

enum ServerOpcode : unsigned char
{
    kAuthenticationResponse = 0,
    kAssignCharacterResponse,
    kServerReferencesMoveRequest,
    kServerTimeSettings,
    kCharacterSpawnRequest,
    kNotifyFactionsChanges,
    kNotifyRemoveCharacter,
    kNotifyQuestUpdate,
    kNotifySetWaypoint,
    kNotifyDelWaypoint,
    kNotifyPlayerList,
    kNotifyPartyInfo,
    kNotifyPartyInvite,
    kNotifyPartyJoined,
    kNotifyPartyLeft,
    kNotifyActorValueChanges,
    kNotifyActorMaxValueChanges,
    kNotifyHealthChangeBroadcast,
    kNotifySpawnData,
    kNotifyActivate,
    kNotifyLockChange,
    kAssignObjectsResponse,
    kNotifyDeathStateChange,
    kNotifyOwnershipTransfer,
    kNotifyObjectInventoryChanges,
    kNotifyInventoryChanges,
    kNotifySpellCast,
    kNotifyInterruptCast,
    kNotifyAddTarget,
    kNotifyProjectileLaunch,
    kNotifyScriptAnimation,
    kNotifyDrawWeapon,
    kNotifyMount,
    kNotifyNewPackage,
    kNotifyRespawn,
    kNotifySyncExperience,
    kNotifyEquipmentChanges,
    kNotifyChatMessageBroadcast,
    kTeleportCommandResponse,
    kStringCacheUpdate,
    kNotifyPlayerRespawn,
    kNotifyDialogue,
    kNotifySubtitle,
    kNotifyPlayerDialogue,
    kNotifyActorTeleport,
    kNotifyRelinquishControl,
    kNotifyPlayerLeft,
    kNotifyPlayerJoined,
    kNotifyPlayerLevel,
    kNotifyPlayerCellChanged,
    kNotifyTeleport,
    kNotifyPlayerPosition,
    kServerOpcodeMax
};
