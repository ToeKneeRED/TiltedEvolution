#include <Services/CharacterService.h>
#include <Components.h>
#include <GameServer.h>
#include <World.h>

#include <Events/CharacterSpawnedEvent.h>
#include <Events/CharacterExteriorCellChangeEvent.h>
#include <Events/CharacterInteriorCellChangeEvent.h>
#include <Events/PlayerEnterWorldEvent.h>
#include <Events/UpdateEvent.h>
#include <Events/CharacterRemoveEvent.h>
#include <Events/OwnershipTransferEvent.h>

#include <Game/OwnerView.h>

#include <Messages/AssignCharacterRequest.h>
#include <Messages/AssignCharacterResponse.h>
#include <Messages/ServerReferencesMoveRequest.h>
#include <Messages/ClientReferencesMoveRequest.h>
#include <Messages/CharacterSpawnRequest.h>
#include <Messages/RequestFactionsChanges.h>
#include <Messages/NotifyFactionsChanges.h>
#include <Messages/NotifyRemoveCharacter.h>
#include <Messages/RequestOwnershipTransfer.h>
#include <Messages/NotifyOwnershipTransfer.h>
#include <Messages/RequestOwnershipClaim.h>
#include <Messages/MountRequest.h>
#include <Messages/NotifyMount.h>
#include <Messages/NewPackageRequest.h>
#include <Messages/NotifyNewPackage.h>
#include <Messages/RequestRespawn.h>
#include <Messages/NotifyRespawn.h>
#include <Messages/SyncExperienceRequest.h>
#include <Messages/NotifySyncExperience.h>
#include <Messages/DialogueRequest.h>
#include <Messages/NotifyDialogue.h>
#include <Messages/SubtitleRequest.h>
#include <Messages/NotifySubtitle.h>
#include <Messages/NotifyActorTeleport.h>

#include <Setting.h>
namespace
{
Console::Setting bEnableXpSync{"Gameplay:bEnableXpSync", "Syncs combat XP within the party", true};
}

CharacterService::CharacterService(World& aWorld, entt::dispatcher& aDispatcher) noexcept
    : m_world(aWorld)
    , m_updateConnection(aDispatcher.sink<UpdateEvent>().connect<&CharacterService::OnUpdate>(this))
    , m_interiorCellChangeEventConnection(aDispatcher.sink<CharacterInteriorCellChangeEvent>().connect<&CharacterService::OnCharacterInteriorCellChange>(this))
    , m_exteriorCellChangeEventConnection(aDispatcher.sink<CharacterExteriorCellChangeEvent>().connect<&CharacterService::OnCharacterExteriorCellChange>(this))
    , m_characterAssignRequestConnection(aDispatcher.sink<PacketEvent<AssignCharacterRequest>>().connect<&CharacterService::OnAssignCharacterRequest>(this))
    , m_transferOwnershipConnection(aDispatcher.sink<PacketEvent<RequestOwnershipTransfer>>().connect<&CharacterService::OnOwnershipTransferRequest>(this))
    , m_ownershipTransferEventConnection(aDispatcher.sink<OwnershipTransferEvent>().connect<&CharacterService::OnOwnershipTransferEvent>(this))
    , m_claimOwnershipConnection(aDispatcher.sink<PacketEvent<RequestOwnershipClaim>>().connect<&CharacterService::OnOwnershipClaimRequest>(this))
    , m_removeCharacterConnection(aDispatcher.sink<CharacterRemoveEvent>().connect<&CharacterService::OnCharacterRemoveEvent>(this))
    , m_characterSpawnedConnection(aDispatcher.sink<CharacterSpawnedEvent>().connect<&CharacterService::OnCharacterSpawned>(this))
    , m_referenceMovementSnapshotConnection(aDispatcher.sink<PacketEvent<ClientReferencesMoveRequest>>().connect<&CharacterService::OnReferencesMoveRequest>(this))
    , m_factionsChangesConnection(aDispatcher.sink<PacketEvent<RequestFactionsChanges>>().connect<&CharacterService::OnFactionsChanges>(this))
    , m_mountConnection(aDispatcher.sink<PacketEvent<MountRequest>>().connect<&CharacterService::OnMountRequest>(this))
    , m_newPackageConnection(aDispatcher.sink<PacketEvent<NewPackageRequest>>().connect<&CharacterService::OnNewPackageRequest>(this))
    , m_requestRespawnConnection(aDispatcher.sink<PacketEvent<RequestRespawn>>().connect<&CharacterService::OnRequestRespawn>(this))
    , m_syncExperienceConnection(aDispatcher.sink<PacketEvent<SyncExperienceRequest>>().connect<&CharacterService::OnSyncExperienceRequest>(this))
    , m_dialogueConnection(aDispatcher.sink<PacketEvent<DialogueRequest>>().connect<&CharacterService::OnDialogueRequest>(this))
    , m_subtitleConnection(aDispatcher.sink<PacketEvent<SubtitleRequest>>().connect<&CharacterService::OnSubtitleRequest>(this))
{
}

void CharacterService::Serialize(World& aRegistry, entt::entity aEntity, CharacterSpawnRequest* apSpawnRequest) noexcept
{
    const auto& characterComponent = aRegistry.get<CharacterComponent>(aEntity);

    apSpawnRequest->ServerId = World::ToInteger(aEntity);
    apSpawnRequest->AppearanceBuffer = characterComponent.SaveBuffer;
    apSpawnRequest->ChangeFlags = characterComponent.ChangeFlags;
    apSpawnRequest->FaceTints = characterComponent.FaceTints;
    apSpawnRequest->FactionsContent = characterComponent.FactionsContent;
    apSpawnRequest->IsDead = characterComponent.IsDead();
    apSpawnRequest->IsPlayer = characterComponent.IsPlayer();
    apSpawnRequest->IsWeaponDrawn = characterComponent.IsWeaponDrawn();
    apSpawnRequest->IsPlayerSummon = characterComponent.IsPlayerSummon();
    apSpawnRequest->PlayerId = characterComponent.PlayerId;

    const auto* pOwnerComponent = aRegistry.try_get<OwnerComponent>(aEntity);
    if (pOwnerComponent)
    {
        apSpawnRequest->OwnershipEpoch = pOwnerComponent->OwnershipEpoch;
    }

    const auto* pFormIdComponent = aRegistry.try_get<FormIdComponent>(aEntity);
    if (pFormIdComponent)
    {
        apSpawnRequest->FormId = pFormIdComponent->Id;
    }

    const auto* pInventoryComponent = aRegistry.try_get<InventoryComponent>(aEntity);
    if (pInventoryComponent)
    {
        apSpawnRequest->InventoryContent = pInventoryComponent->Content;
    }

    const auto* pActorValuesComponent = aRegistry.try_get<ActorValuesComponent>(aEntity);
    if (pActorValuesComponent)
    {
        apSpawnRequest->InitialActorValues = pActorValuesComponent->CurrentActorValues;
    }

    if (characterComponent.BaseId)
    {
        apSpawnRequest->BaseId = characterComponent.BaseId.Id;
    }

    const auto* pMovementComponent = aRegistry.try_get<MovementComponent>(aEntity);
    if (pMovementComponent)
    {
        apSpawnRequest->Position = pMovementComponent->Position;
        apSpawnRequest->Rotation.x = pMovementComponent->Rotation.x;
        apSpawnRequest->Rotation.y = pMovementComponent->Rotation.z;
    }

    const auto* pCellIdComponent = aRegistry.try_get<CellIdComponent>(aEntity);
    if (pCellIdComponent)
    {
        apSpawnRequest->CellId = pCellIdComponent->Cell;
    }

    auto& animationComponent = aRegistry.get<AnimationComponent>(aEntity);
    apSpawnRequest->ActionsToReplay = animationComponent.ActionsReplayCache.FormRefinedReplayChain();
}

void CharacterService::OnUpdate(const UpdateEvent&) const noexcept
{
    ProcessFactionsChanges();
    ProcessMovementChanges();
}

void CharacterService::OnCharacterExteriorCellChange(const CharacterExteriorCellChangeEvent& acEvent) const noexcept
{
    CharacterSpawnRequest spawnMessage;
    Serialize(m_world, acEvent.Entity, &spawnMessage);

    NotifyRemoveCharacter removeMessage;
    removeMessage.ServerId = World::ToInteger(acEvent.Entity);

    for (auto pPlayer : m_world.GetPlayerManager())
    {
        if (acEvent.Owner == pPlayer)
            continue;

        if (pPlayer->GetCellComponent().WorldSpaceId != acEvent.WorldSpaceId || pPlayer->GetCellComponent().WorldSpaceId == acEvent.WorldSpaceId && !GridCellCoords::IsCellInGridCell(acEvent.CurrentCoords, pPlayer->GetCellComponent().CenterCoords, false))
        {
            pPlayer->Send(removeMessage);
        }
        else if (pPlayer->GetCellComponent().WorldSpaceId == acEvent.WorldSpaceId && GridCellCoords::IsCellInGridCell(acEvent.CurrentCoords, pPlayer->GetCellComponent().CenterCoords, false))
        {
            pPlayer->Send(spawnMessage);
        }
    }
}

void CharacterService::OnCharacterInteriorCellChange(const CharacterInteriorCellChangeEvent& acEvent) const noexcept
{
    CharacterSpawnRequest spawnMessage;
    Serialize(m_world, acEvent.Entity, &spawnMessage);

    NotifyRemoveCharacter removeMessage;
    removeMessage.ServerId = World::ToInteger(acEvent.Entity);

    for (auto pPlayer : m_world.GetPlayerManager())
    {
        if (acEvent.Owner == pPlayer)
            continue;

        if (acEvent.NewCell == pPlayer->GetCellComponent().Cell)
            pPlayer->Send(spawnMessage);
        else
            pPlayer->Send(removeMessage);
    }
}

void CharacterService::OnAssignCharacterRequest(const PacketEvent<AssignCharacterRequest>& acMessage) const noexcept
{
    auto& message = acMessage.Packet;
    const auto& refId = message.ReferenceId;

    const auto isPlayer = (refId.ModId == 0 && refId.BaseId == 0x14);
    const auto isCustom = isPlayer || refId.ModId == std::numeric_limits<uint32_t>::max();

    // Check if id is the player
    if (!isCustom)
    {
        // Look for the character
        auto view = m_world.view<FormIdComponent, ActorValuesComponent, CharacterComponent, MovementComponent, CellIdComponent, OwnerComponent, InventoryComponent>();

        const auto itor = std::find_if(
            std::begin(view), std::end(view),
            [view, refId](auto entity)
            {
                const auto& formIdComponent = view.get<FormIdComponent>(entity);

                return formIdComponent.Id == refId;
            });

        if (itor != std::end(view))
        {
            spdlog::debug("FormId: {:x}:{:x} is already managed", refId.ModId, refId.BaseId);

            auto& ownerComponent = view.get<OwnerComponent>(*itor);
            const bool isOwner = ownerComponent.GetOwner() == acMessage.pPlayer;
            const bool transferToLeader = !isOwner && CanClaimOwnership(acMessage.pPlayer, *itor, ownerComponent.OwnershipEpoch, "leader_assignment");

            AssignCharacterResponse response{};
            response.Cookie = message.Cookie;
            response.Owner = isOwner;
            PopulateAssignmentResponse(*itor, response);
            acMessage.pPlayer->Send(response);

            // The assignment response establishes a remote component before the grant arrives.
            if (transferToLeader)
                TransferOwnership(acMessage.pPlayer, *itor, "leader_assignment");

            return;
        }
    }

    // This entity has no owner create it
    CreateCharacter(acMessage);
}

void CharacterService::OnOwnershipTransferRequest(const PacketEvent<RequestOwnershipTransfer>& acMessage) const noexcept
{
    const auto& message = acMessage.Packet;

    const entt::entity cEntity = static_cast<entt::entity>(message.ServerId);
    const auto view = m_world.view<OwnerComponent, CharacterComponent, CellIdComponent, MovementComponent>();
    const auto it = view.find(cEntity);
    if (it == view.end())
    {
        spdlog::debug("ownership_relinquish_rejected reason=missing_entity player={:X} server_id={:X} epoch={}", acMessage.pPlayer->GetId(), message.ServerId, message.OwnershipEpoch);
        return;
    }

    auto& ownerComponent = view.get<OwnerComponent>(*it);
    if (ownerComponent.GetOwner() != acMessage.pPlayer || ownerComponent.OwnershipEpoch != message.OwnershipEpoch)
    {
        const uint32_t ownerId = ownerComponent.GetOwner() ? ownerComponent.GetOwner()->GetId() : 0;
        spdlog::debug(
            "ownership_relinquish_rejected reason=stale_or_non_owner player={:X} server_id={:X} requested_epoch={} owner={:X} current_epoch={}", acMessage.pPlayer->GetId(), message.ServerId, message.OwnershipEpoch, ownerId, ownerComponent.OwnershipEpoch);
        return;
    }

    auto& characterComponent = view.get<CharacterComponent>(*it);
    if (characterComponent.IsPlayerSummon())
    {
        spdlog::info("ownership_relinquish reason=orphaned_summon player={:X} server_id={:X} epoch={}", acMessage.pPlayer->GetId(), message.ServerId, message.OwnershipEpoch);
        m_world.GetDispatcher().trigger(CharacterRemoveEvent(message.ServerId));
        return;
    }

    if (message.WorldSpaceId || message.CellId)
    {
        const auto* pFormIdComponent = m_world.try_get<FormIdComponent>(cEntity);
        if (pFormIdComponent)
        {
            NotifyActorTeleport notify{};
            notify.FormId = pFormIdComponent->Id;
            notify.WorldSpaceId = message.WorldSpaceId;
            notify.CellId = message.CellId;
            notify.Position = message.Position;

            GameServer::Get()->SendToPlayers(notify, acMessage.pPlayer);
        }

        auto& cellIdComponent = view.get<CellIdComponent>(*it);
        cellIdComponent.WorldSpaceId = message.WorldSpaceId;
        cellIdComponent.Cell = message.CellId;
        cellIdComponent.CenterCoords = GridCellCoords::CalculateGridCellCoords(message.Position);

        auto& movementComponent = view.get<MovementComponent>(*it);
        movementComponent.Position = message.Position;
        movementComponent.Sent = true;
    }

    ownerComponent.InvalidOwners.push_back(acMessage.pPlayer);

    TransferToNextOwner(cEntity, "relinquish");
}

void CharacterService::OnOwnershipTransferEvent(const OwnershipTransferEvent& acEvent) const noexcept
{
    TransferToNextOwner(acEvent.Entity, "owner_unavailable");
}

void CharacterService::OnCharacterRemoveEvent(const CharacterRemoveEvent& acEvent) const noexcept
{
    const auto view = m_world.view<OwnerComponent>();
    const auto it = view.find(static_cast<entt::entity>(acEvent.ServerId));
    if (it == view.end())
        return;

    GameServer::Get()->GetWorld().GetScriptService().HandleCharacterDestoy(*it);

    NotifyRemoveCharacter response;
    response.ServerId = acEvent.ServerId;

    for (auto pPlayer : m_world.GetPlayerManager())
        pPlayer->Send(response);

    m_world.destroy(*it);
    spdlog::debug("Character destroyed {:X}", acEvent.ServerId);
}

void CharacterService::OnOwnershipClaimRequest(const PacketEvent<RequestOwnershipClaim>& acMessage) const noexcept
{
    const auto& message = acMessage.Packet;
    const entt::entity cEntity = static_cast<entt::entity>(message.ServerId);

    if (!CanClaimOwnership(acMessage.pPlayer, cEntity, message.ExpectedOwnershipEpoch, "leader_claim"))
        return;

    TransferOwnership(acMessage.pPlayer, cEntity, "leader_claim");
}

void CharacterService::OnCharacterSpawned(const CharacterSpawnedEvent& acEvent) const noexcept
{
    CharacterSpawnRequest message;
    Serialize(m_world, acEvent.Entity, &message);

    const auto& ownerComp = m_world.get<OwnerComponent>(acEvent.Entity);
    if (!GameServer::Get()->SendToPlayersInRange(message, acEvent.Entity, ownerComp.GetOwner()))
        spdlog::error("{}: SendToPlayersInRange failed", __FUNCTION__);

    GameServer::Get()->GetWorld().GetScriptService().HandleCharacterSpawn(acEvent.Entity);
}

void CharacterService::OnReferencesMoveRequest(const PacketEvent<ClientReferencesMoveRequest>& acMessage) const noexcept
{
    OwnerView<AnimationComponent, MovementComponent, CellIdComponent> view(m_world, acMessage.GetSender());

    auto& message = acMessage.Packet;

    for (auto& entry : message.Updates)
    {
        const auto entity = static_cast<entt::entity>(entry.first);

        auto itor = view.find(entity);
        if (itor == std::end(view))
        {
            spdlog::debug("{:x} requested move of {:x} but does not exist", acMessage.pPlayer->GetConnectionId(), World::ToInteger(*itor));
            continue;
        }

        auto& movementComponent = view.get<MovementComponent>(*itor);
        auto& cellIdComponent = view.get<CellIdComponent>(*itor);
        auto& animationComponent = view.get<AnimationComponent>(*itor);

        movementComponent.Tick = message.Tick;

        const auto movementCopy = movementComponent;

        auto& update = entry.second;
        auto& movement = update.UpdatedMovement;

        movementComponent.Position = movement.Position;
        movementComponent.Rotation = glm::vec3(movement.Rotation.x, 0.f, movement.Rotation.y);
        movementComponent.Variables = movement.Variables;
        movementComponent.Direction = movement.Direction;

        cellIdComponent.Cell = movement.CellId;
        cellIdComponent.WorldSpaceId = movement.WorldSpaceId;
        cellIdComponent.CenterCoords = GridCellCoords::CalculateGridCellCoords(movement.Position.x, movement.Position.y);

        for (auto& action : update.ActionEvents)
        {
            auto [canceled, reason] = GameServer::Get()->GetWorld().GetScriptService().HandleCharacterMove(entity);
            if (canceled)
                continue;

            animationComponent.CurrentAction = action;

            animationComponent.Actions.push_back(animationComponent.CurrentAction);
        }

        animationComponent.ActionsReplayCache.AppendAll(update.ActionEvents);

        movementComponent.Sent = false;
    }
}

void CharacterService::OnFactionsChanges(const PacketEvent<RequestFactionsChanges>& acMessage) const noexcept
{
    OwnerView<CharacterComponent> view(m_world, acMessage.GetSender());

    auto& message = acMessage.Packet;

    for (auto& [id, factions] : message.Changes)
    {
        auto it = view.find(static_cast<entt::entity>(id));

        if (it == std::end(view) || view.get<OwnerComponent>(*it).GetOwner() != acMessage.pPlayer)
            continue;

        auto& characterComponent = view.get<CharacterComponent>(*it);
        characterComponent.FactionsContent = factions;
        characterComponent.SetDirtyFactions(true);
    }
}

void CharacterService::OnMountRequest(const PacketEvent<MountRequest>& acMessage) const noexcept
{
    const auto& message = acMessage.Packet;
    const entt::entity cRiderEntity = static_cast<entt::entity>(message.RiderId);
    const entt::entity cMountEntity = static_cast<entt::entity>(message.MountId);
    const auto view = m_world.view<OwnerComponent, CharacterComponent, CellIdComponent>();
    const auto riderIt = view.find(cRiderEntity);
    const auto mountIt = view.find(cMountEntity);

    if (riderIt == view.end() || mountIt == view.end() || cRiderEntity == cMountEntity)
    {
        spdlog::debug("mount_rejected reason=missing_or_invalid_entity player={:X} rider={:X} mount={:X}", acMessage.pPlayer->GetId(), message.RiderId, message.MountId);
        return;
    }

    if (!view.get<CharacterComponent>(*mountIt).IsMount())
    {
        spdlog::debug("mount_rejected reason=target_not_mount player={:X} rider={:X} mount={:X}", acMessage.pPlayer->GetId(), message.RiderId, message.MountId);
        return;
    }

    const auto& riderOwner = view.get<OwnerComponent>(*riderIt);
    const auto& mountOwner = view.get<OwnerComponent>(*mountIt);
    if (riderOwner.GetOwner() != acMessage.pPlayer || riderOwner.OwnershipEpoch != message.RiderOwnershipEpoch || mountOwner.OwnershipEpoch != message.MountOwnershipEpoch)
    {
        spdlog::debug(
            "mount_rejected reason=stale_or_non_owner player={:X} rider={:X} rider_epoch={}/{} mount={:X} mount_epoch={}/{}", acMessage.pPlayer->GetId(), message.RiderId, message.RiderOwnershipEpoch, riderOwner.OwnershipEpoch, message.MountId, message.MountOwnershipEpoch,
            mountOwner.OwnershipEpoch);
        return;
    }

    const auto& mountCell = view.get<CellIdComponent>(*mountIt);
    if (!acMessage.pPlayer->GetCellComponent().IsInRange(mountCell, view.get<CharacterComponent>(*mountIt).IsDragon()))
    {
        spdlog::debug("mount_rejected reason=out_of_range player={:X} rider={:X} mount={:X}", acMessage.pPlayer->GetId(), message.RiderId, message.MountId);
        return;
    }

    if (!TransferOwnership(acMessage.pPlayer, *mountIt, "mount"))
        return;

    NotifyMount notify;
    notify.RiderId = message.RiderId;
    notify.MountId = message.MountId;

    if (!GameServer::Get()->SendToPlayersInRange(notify, cMountEntity, acMessage.GetSender()))
        spdlog::error("{}: SendToPlayersInRange failed", __FUNCTION__);
}

void CharacterService::OnNewPackageRequest(const PacketEvent<NewPackageRequest>& acMessage) const noexcept
{
    auto& message = acMessage.Packet;

    NotifyNewPackage notify;
    notify.ActorId = message.ActorId;
    notify.PackageId = message.PackageId;

    const entt::entity cEntity = static_cast<entt::entity>(message.ActorId);
    if (!GameServer::Get()->SendToPlayersInRange(notify, cEntity, acMessage.GetSender()))
        spdlog::error("{}: SendToPlayersInRange failed", __FUNCTION__);
}

void CharacterService::OnRequestRespawn(const PacketEvent<RequestRespawn>& acMessage) const noexcept
{
    auto view = m_world.view<OwnerComponent, CharacterComponent>();
    auto it = view.find(static_cast<entt::entity>(acMessage.Packet.ActorId));
    if (it == view.end())
    {
        spdlog::warn("No OwnerComponent found for actor id {:X}", acMessage.Packet.ActorId);
        return;
    }

    auto& ownerComponent = view.get<OwnerComponent>(*it);

    // Replay cache needs to be cleared when a character respawns
    m_world.try_get<AnimationComponent>(*it)->ActionsReplayCache.Clear();

    if (ownerComponent.GetOwner() == acMessage.pPlayer)
    {
        if (!acMessage.Packet.AppearanceBuffer.empty())
        {
            auto& characterComponent = view.get<CharacterComponent>(*it);
            characterComponent.SaveBuffer = acMessage.Packet.AppearanceBuffer;
            characterComponent.ChangeFlags = acMessage.Packet.ChangeFlags;
        }

        NotifyRespawn notify;
        notify.ActorId = acMessage.Packet.ActorId;

        if (!GameServer::Get()->SendToPlayersInRange(notify, *it, acMessage.GetSender()))
            spdlog::error("{}: SendToPlayersInRange failed", __FUNCTION__);
    }
    else
    {
        CharacterSpawnRequest message;
        Serialize(m_world, *it, &message);

        acMessage.GetSender()->Send(message);
    }
}

void CharacterService::OnSyncExperienceRequest(const PacketEvent<SyncExperienceRequest>& acMessage) const noexcept
{
    if (!bEnableXpSync)
        return;

    NotifySyncExperience notify;
    notify.Experience = acMessage.Packet.Experience;

    const auto& partyComponent = acMessage.pPlayer->GetParty();
    GameServer::Get()->SendToParty(notify, partyComponent, acMessage.GetSender());
}

void CharacterService::OnDialogueRequest(const PacketEvent<DialogueRequest>& acMessage) const noexcept
{
    auto& message = acMessage.Packet;

    NotifyDialogue notify{};
    notify.ServerId = message.ServerId;
    notify.SoundFilename = message.SoundFilename;

    const entt::entity cEntity = static_cast<entt::entity>(message.ServerId);
    if (!GameServer::Get()->SendToPlayersInRange(notify, cEntity, acMessage.GetSender()))
        spdlog::error("{}: SendToPlayersInRange failed", __FUNCTION__);
}

void CharacterService::OnSubtitleRequest(const PacketEvent<SubtitleRequest>& acMessage) const noexcept
{
    auto& message = acMessage.Packet;

    NotifySubtitle notify{};
    notify.ServerId = message.ServerId;
    notify.Text = message.Text;

    const entt::entity cEntity = static_cast<entt::entity>(message.ServerId);
    if (!GameServer::Get()->SendToPlayersInRange(notify, cEntity, acMessage.GetSender()))
        spdlog::error("{}: SendToPlayersInRange failed", __FUNCTION__);
}

void CharacterService::CreateCharacter(const PacketEvent<AssignCharacterRequest>& acMessage) const noexcept
{
    auto& message = acMessage.Packet;

    const auto gameId = message.ReferenceId;
    const auto baseId = message.FormId;

    const auto cEntity = m_world.create();
    const auto isTemporary = gameId.ModId == std::numeric_limits<uint32_t>::max();
    const auto isPlayer = (gameId.ModId == 0 && gameId.BaseId == 0x14);
    const auto isCustom = isPlayer || isTemporary;

    // For player characters and temporary forms
    if (!isCustom)
    {
        m_world.emplace<FormIdComponent>(cEntity, gameId.BaseId, gameId.ModId);
    }
    else if (baseId != GameId{} && !isTemporary)
    {
        m_world.destroy(cEntity);
        spdlog::warn("Unexpected NpcId, player {:x} might be forging packets", acMessage.pPlayer->GetConnectionId());
        return;
    }

    auto* const pServer = GameServer::Get();

    m_world.emplace<OwnerComponent>(cEntity, acMessage.pPlayer);

    auto& cellIdComponent = m_world.emplace<CellIdComponent>(cEntity, message.CellId);
    if (message.WorldSpaceId != GameId{})
    {
        cellIdComponent.WorldSpaceId = message.WorldSpaceId;
        cellIdComponent.CenterCoords = GridCellCoords::CalculateGridCellCoords(message.Position);
    }

    auto& characterComponent = m_world.emplace<CharacterComponent>(cEntity);
    characterComponent.ChangeFlags = message.ChangeFlags;
    characterComponent.SaveBuffer = std::move(message.AppearanceBuffer);
    characterComponent.BaseId = FormIdComponent(message.FormId);
    characterComponent.FaceTints = message.FaceTints;
    characterComponent.FactionsContent = message.FactionsContent;
    characterComponent.SetDead(message.CurrentActorData.IsDead);
    characterComponent.SetPlayer(isPlayer);
    characterComponent.SetWeaponDrawn(message.CurrentActorData.IsWeaponDrawn);
    characterComponent.SetDragon(message.IsDragon);
    characterComponent.SetMount(message.IsMount);
    characterComponent.SetPlayerSummon(message.IsPlayerSummon);

    auto& inventoryComponent = m_world.emplace<InventoryComponent>(cEntity);
    inventoryComponent.Content = message.CurrentActorData.InitialInventory;

    auto& actorValuesComponent = m_world.emplace<ActorValuesComponent>(cEntity);
    actorValuesComponent.CurrentActorValues = message.CurrentActorData.InitialActorValues;

    spdlog::debug("FormId: {:x}:{:x} - NpcId: {:x}:{:x} assigned to {:x}", gameId.ModId, gameId.BaseId, baseId.ModId, baseId.BaseId, acMessage.pPlayer->GetConnectionId());

    auto& movementComponent = m_world.emplace<MovementComponent>(cEntity);
    movementComponent.Tick = pServer->GetTick();
    movementComponent.Position = message.Position;
    movementComponent.Rotation = {message.Rotation.x, 0.f, message.Rotation.y};
    movementComponent.Sent = false;

    m_world.emplace<AnimationComponent>(cEntity);

    // If this is a player character store a ref and trigger an event
    if (isPlayer)
    {
        const auto pPlayer = acMessage.pPlayer;

        pPlayer->SetCharacter(cEntity);
        pPlayer->GetQuestLogComponent().QuestContent = message.QuestContent;
        characterComponent.PlayerId = pPlayer->GetId();

        auto& dispatcher = m_world.GetDispatcher();
        dispatcher.trigger(PlayerEnterWorldEvent(pPlayer));
    }

    AssignCharacterResponse response{};
    response.Cookie = message.Cookie;
    response.Owner = true;
    PopulateAssignmentResponse(cEntity, response);

    pServer->Send(acMessage.pPlayer->GetConnectionId(), response);

    auto& dispatcher = m_world.GetDispatcher();
    dispatcher.trigger(CharacterSpawnedEvent(cEntity));
}

void CharacterService::PopulateAssignmentResponse(const entt::entity aEntity, AssignCharacterResponse& aResponse) const noexcept
{
    aResponse.ServerId = World::ToInteger(aEntity);

    if (const auto* pOwnerComponent = m_world.try_get<OwnerComponent>(aEntity))
        aResponse.OwnershipEpoch = pOwnerComponent->OwnershipEpoch;

    if (const auto* pActorValuesComponent = m_world.try_get<ActorValuesComponent>(aEntity))
        aResponse.AllActorValues = pActorValuesComponent->CurrentActorValues;

    if (const auto* pInventoryComponent = m_world.try_get<InventoryComponent>(aEntity))
        aResponse.CurrentInventory = pInventoryComponent->Content;

    if (const auto* pCharacterComponent = m_world.try_get<CharacterComponent>(aEntity))
    {
        aResponse.PlayerId = pCharacterComponent->PlayerId;
        aResponse.IsDead = pCharacterComponent->IsDead();
        aResponse.IsWeaponDrawn = pCharacterComponent->IsWeaponDrawn();
    }

    if (const auto* pMovementComponent = m_world.try_get<MovementComponent>(aEntity))
        aResponse.Position = pMovementComponent->Position;

    if (const auto* pCellIdComponent = m_world.try_get<CellIdComponent>(aEntity))
    {
        aResponse.CellId = pCellIdComponent->Cell;
        aResponse.WorldSpaceId = pCellIdComponent->WorldSpaceId;
    }

    if (auto* pAnimationComponent = m_world.try_get<AnimationComponent>(aEntity))
        aResponse.ActionsToReplay = pAnimationComponent->ActionsReplayCache.FormRefinedReplayChain();
}

bool CharacterService::CanClaimOwnership(Player* apPlayer, const entt::entity aEntity, const uint32_t aExpectedOwnershipEpoch, const char* apContext) const noexcept
{
    const uint32_t serverId = World::ToInteger(aEntity);
    const auto view = m_world.view<OwnerComponent, CharacterComponent, CellIdComponent, FormIdComponent>();
    const auto it = view.find(aEntity);
    if (it == view.end())
    {
        spdlog::debug("ownership_claim_rejected context={} reason=missing_or_temporary player={:X} server_id={:X} expected_epoch={}", apContext, apPlayer->GetId(), serverId, aExpectedOwnershipEpoch);
        return false;
    }

    const auto& ownerComponent = view.get<OwnerComponent>(*it);
    const auto& characterComponent = view.get<CharacterComponent>(*it);
    const auto& cellIdComponent = view.get<CellIdComponent>(*it);
    Player* const pCurrentOwner = ownerComponent.GetOwner();
    const uint32_t currentOwnerId = pCurrentOwner ? pCurrentOwner->GetId() : 0;

    const auto reject = [&](const char* apReason)
    {
        spdlog::debug(
            "ownership_claim_rejected context={} reason={} player={:X} server_id={:X} expected_epoch={} owner={:X} current_epoch={}", apContext, apReason, apPlayer->GetId(), serverId, aExpectedOwnershipEpoch, currentOwnerId, ownerComponent.OwnershipEpoch);
        return false;
    };

    if (aExpectedOwnershipEpoch == 0 || ownerComponent.OwnershipEpoch != aExpectedOwnershipEpoch)
        return reject("stale_epoch");

    if (!pCurrentOwner || pCurrentOwner == apPlayer)
        return reject("already_owner");

    if (characterComponent.IsMount() || characterComponent.IsPlayer())
        return reject("ineligible_actor");

    if (!apPlayer->GetCellComponent().IsInRange(cellIdComponent, characterComponent.IsDragon()))
        return reject("out_of_range");

    auto& partyService = m_world.GetPartyService();
    if (!partyService.IsPlayerInParty(apPlayer) || !partyService.IsPlayerLeader(apPlayer))
        return reject("not_party_leader");

    PartyService::Party* const pParty = partyService.GetPlayerParty(apPlayer);
    if (!pParty || std::find(pParty->Members.begin(), pParty->Members.end(), pCurrentOwner) == pParty->Members.end())
        return reject("owner_not_in_party");

    return true;
}

bool CharacterService::TransferOwnership(Player* apPlayer, const entt::entity aEntity, const char* apReason, const bool aResetInvalidOwners) const noexcept
{
    const auto view = m_world.view<OwnerComponent, CharacterComponent, CellIdComponent>();
    const auto it = view.find(aEntity);
    if (!apPlayer || it == view.end())
    {
        spdlog::debug("ownership_transfer_rejected reason=invalid_target transfer_reason={} server_id={:X}", apReason, World::ToInteger(aEntity));
        return false;
    }

    auto& ownerComponent = view.get<OwnerComponent>(*it);
    Player* const pOldOwner = ownerComponent.GetOwner();
    if (pOldOwner == apPlayer)
    {
        spdlog::debug("ownership_transfer_unchanged reason={} server_id={:X} owner={:X} epoch={}", apReason, World::ToInteger(aEntity), apPlayer->GetId(), ownerComponent.OwnershipEpoch);
        return true;
    }

    const uint32_t oldOwnerId = pOldOwner ? pOldOwner->GetId() : 0;
    const uint32_t oldEpoch = ownerComponent.OwnershipEpoch;
    uint32_t newEpoch = oldEpoch + 1;
    if (newEpoch == 0)
        newEpoch = 1;

    NotifyOwnershipTransfer notify{};
    notify.ServerId = World::ToInteger(aEntity);
    notify.OwnerPlayerId = apPlayer->GetId();
    notify.OwnershipEpoch = newEpoch;
    notify.CurrentActorData = BuildActorData(aEntity);

    ownerComponent.SetOwner(apPlayer);
    ownerComponent.OwnershipEpoch = newEpoch;
    if (aResetInvalidOwners)
        ownerComponent.InvalidOwners.clear();

    if (!GameServer::Get()->SendToPlayersInRange(notify, aEntity))
        spdlog::error("{}: SendToPlayersInRange failed", __FUNCTION__);

    spdlog::info(
        "ownership_transfer reason={} server_id={:X} old_owner={:X} new_owner={:X} old_epoch={} new_epoch={}", apReason, notify.ServerId, oldOwnerId, notify.OwnerPlayerId, oldEpoch, newEpoch);

    return true;
}

void CharacterService::TransferToNextOwner(const entt::entity aEntity, const char* apReason) const noexcept
{
    const auto view = m_world.view<OwnerComponent, CharacterComponent, CellIdComponent>();
    const auto it = view.find(aEntity);
    if (it == view.end())
    {
        spdlog::debug("ownership_transfer_rejected reason=missing_entity transfer_reason={} server_id={:X}", apReason, World::ToInteger(aEntity));
        return;
    }

    auto& ownerComponent = view.get<OwnerComponent>(*it);
    const auto& characterComponent = view.get<CharacterComponent>(*it);
    const auto& cellIdComponent = view.get<CellIdComponent>(*it);

    for (Player* pPlayer : m_world.GetPlayerManager())
    {
        if (pPlayer == ownerComponent.GetOwner())
            continue;

        if (std::find(ownerComponent.InvalidOwners.begin(), ownerComponent.InvalidOwners.end(), pPlayer) != ownerComponent.InvalidOwners.end())
            continue;

        if (!pPlayer->GetCellComponent().IsInRange(cellIdComponent, characterComponent.IsDragon()))
            continue;

        // Retain every owner that declined this handoff chain so the actor cannot bounce between unloaded clients.
        if (TransferOwnership(pPlayer, aEntity, apReason, false))
            return;
    }

    spdlog::info("ownership_transfer reason={} server_id={:X} result=no_eligible_owner", apReason, World::ToInteger(aEntity));
    m_world.GetDispatcher().trigger(CharacterRemoveEvent(World::ToInteger(aEntity)));
}

ActorData CharacterService::BuildActorData(const entt::entity acEntity) const noexcept
{
    ActorData actorData{};

    const auto* pActorValuesComponent = m_world.try_get<ActorValuesComponent>(acEntity);
    if (pActorValuesComponent)
    {
        actorData.InitialActorValues = pActorValuesComponent->CurrentActorValues;
    }

    const auto* pInventoryComponent = m_world.try_get<InventoryComponent>(acEntity);
    if (pInventoryComponent)
    {
        actorData.InitialInventory = pInventoryComponent->Content;
    }

    actorData.IsDead = false;
    const auto* pCharacterComponent = m_world.try_get<CharacterComponent>(acEntity);
    if (pCharacterComponent)
    {
        actorData.IsDead = pCharacterComponent->IsDead();
        actorData.IsWeaponDrawn = pCharacterComponent->IsWeaponDrawn();
    }

    return actorData;
}

void CharacterService::ProcessFactionsChanges() const noexcept
{
    static std::chrono::steady_clock::time_point lastSendTimePoint;
    constexpr auto cDelayBetweenSnapshots = 2000ms;

    const auto now = std::chrono::steady_clock::now();
    if (now - lastSendTimePoint < cDelayBetweenSnapshots)
        return;

    lastSendTimePoint = now;

    const auto characterView = m_world.view<CellIdComponent, CharacterComponent, OwnerComponent>();

    TiltedPhoques::Map<Player*, NotifyFactionsChanges> messages;

    for (auto entity : characterView)
    {
        auto& characterComponent = characterView.get<CharacterComponent>(entity);
        auto& cellIdComponent = characterView.get<CellIdComponent>(entity);
        auto& ownerComponent = characterView.get<OwnerComponent>(entity);

        // If we have nothing new to send skip this
        if (!characterComponent.IsDirtyFactions())
            continue;

        for (auto pPlayer : m_world.GetPlayerManager())
        {
            if (pPlayer == ownerComponent.GetOwner())
                continue;

            if (!cellIdComponent.IsInRange(pPlayer->GetCellComponent(), characterComponent.IsDragon()))
                continue;

            auto& message = messages[pPlayer];
            auto& change = message.Changes[World::ToInteger(entity)];

            change = characterComponent.FactionsContent;
        }

        characterComponent.SetDirtyFactions(false);
    }

    for (auto [pPlayer, message] : messages)
    {
        if (!message.Changes.empty())
            pPlayer->Send(message);
    }
}

void CharacterService::ProcessMovementChanges() const noexcept
{
    static std::chrono::steady_clock::time_point lastSendTimePoint;
    constexpr auto cDelayBetweenSnapshots = 1000ms / 50;

    const auto now = std::chrono::steady_clock::now();
    if (now - lastSendTimePoint < cDelayBetweenSnapshots)
        return;

    lastSendTimePoint = now;

    const auto characterView = m_world.view<CharacterComponent, CellIdComponent, MovementComponent, AnimationComponent, OwnerComponent>();

    TiltedPhoques::Map<Player*, ServerReferencesMoveRequest> messages;

    for (auto pPlayer : m_world.GetPlayerManager())
    {
        auto& message = messages[pPlayer];

        message.Tick = GameServer::Get()->GetTick();
    }

    for (auto entity : characterView)
    {
        auto& characterComponent = characterView.get<CharacterComponent>(entity);
        auto& movementComponent = characterView.get<MovementComponent>(entity);
        auto& cellIdComponent = characterView.get<CellIdComponent>(entity);
        auto& ownerComponent = characterView.get<OwnerComponent>(entity);
        auto& animationComponent = characterView.get<AnimationComponent>(entity);

        // If we have nothing new to send skip this
        if (movementComponent.Sent == true)
            continue;

        for (auto pPlayer : m_world.GetPlayerManager())
        {
            if (pPlayer == ownerComponent.GetOwner())
                continue;

            if (!cellIdComponent.IsInRange(pPlayer->GetCellComponent(), characterComponent.IsDragon()))
                continue;

            auto& message = messages[pPlayer];
            auto& update = message.Updates[World::ToInteger(entity)];
            auto& movement = update.UpdatedMovement;

            movement.Position = movementComponent.Position;

            movement.Rotation.x = movementComponent.Rotation.x;
            movement.Rotation.y = movementComponent.Rotation.z;

            movement.Direction = movementComponent.Direction;
            movement.Variables = movementComponent.Variables;

            update.ActionEvents = animationComponent.Actions;
        }
    }

    m_world.view<AnimationComponent>().each([](AnimationComponent& animationComponent)
    {
        // Remove actions we've sent
        animationComponent.Actions.clear();
    });

    m_world.view<MovementComponent>().each([](MovementComponent& movementComponent) { movementComponent.Sent = true; });

    for (auto& [pPlayer, message] : messages)
    {
        if (!message.Updates.empty())
            pPlayer->Send(message);
    }
}
