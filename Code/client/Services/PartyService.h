#pragma once
#include "Events/InventoryChangeEvent.h"
#include "Structs/ItemDesync.h"

struct World;
struct ImguiService;
struct TransportService;
struct UpdateEvent;
struct DisconnectedEvent;
struct NotifyPlayerList;
struct NotifyPartyInfo;
struct NotifyPartyInvite;
struct NotifyPartyJoined;
struct NotifyPartyLeft;
struct InventoryChangeEvent;

/**
 * @brief Manages the party of the local player.
 */
struct PartyService
{
    PartyService(World& aWorld, entt::dispatcher& aDispatcher, TransportService& aTransportService) noexcept;
    ~PartyService() = default;

    TP_NOCOPYMOVE(PartyService);

    [[nodiscard]] bool IsInParty() const noexcept { return m_inParty; }
    [[nodiscard]] bool IsLeader() const noexcept { return m_isLeader; }
    [[nodiscard]] uint32_t GetLeaderPlayerId() const noexcept { return m_leaderPlayerId; }

    const Vector<uint32_t>& GetPartyMembers() const noexcept { return m_partyMembers; }
    const Map<uint32_t, String>& GetPlayers() const noexcept { return m_players; }
    Map<uint32_t, uint64_t>& GetInvitations() noexcept { return m_invitations; }

    [[nodiscard]] bool CanSendInventoryChangeRequest(const InventoryChangeEvent& acEvent) const noexcept;
    [[nodiscard]] bool IsDesyncedItem(uint32_t aId) const noexcept { return m_desyncedItems.find(aId) != m_desyncedItems.end(); }
    uint8_t GetItemDesyncFlags(uint32_t aId) const noexcept {return !IsDesyncedItem(aId) ? ItemDesync::kNone : m_desyncedItems.find(aId)->second; }

    void CreateParty() const noexcept;
    void LeaveParty() const noexcept;
    void CreateInvite(const uint32_t aPlayerId) const noexcept;
    void AcceptInvite(const uint32_t aInviterId) const noexcept;
    void KickPartyMember(const uint32_t aPlayerId) const noexcept;
    void ChangePartyLeader(const uint32_t aPlayerId) const noexcept;

protected:
    void OnUpdate(const UpdateEvent& acEvent) noexcept;
    void OnDisconnected(const DisconnectedEvent& acEvent) noexcept;
    void OnPlayerList(const NotifyPlayerList& acPlayerList) noexcept;
    void OnPartyInfo(const NotifyPartyInfo& acPartyInfo) noexcept;
    void OnPartyInvite(const NotifyPartyInvite& acPartyInvite) noexcept;
    void OnPartyJoined(const NotifyPartyJoined& acPartyJoined) noexcept;
    void OnPartyLeft(const NotifyPartyLeft& acPartyLeft) noexcept;

private:
    void DestroyParty() noexcept;

    // item ID, desynced item event flags
    Map<uint32_t, uint8_t> m_desyncedItems
    {
        std::pair<uint32_t, uint8_t>(0x39647, static_cast<uint8_t>(ItemDesync::kAdd | ItemDesync::kRemove)), // Golden Claw

    };

    Map<uint32_t, String> m_players;
    Map<uint32_t, uint64_t> m_invitations;
    uint64_t m_nextUpdate{0};

    bool m_inParty = false;
    bool m_isLeader = false;
    uint32_t m_leaderPlayerId;
    Vector<uint32_t> m_partyMembers;

    World& m_world;
    TransportService& m_transport;

    entt::scoped_connection m_updateConnection;
    entt::scoped_connection m_disconnectConnection;
    entt::scoped_connection m_playerListConnection;
    entt::scoped_connection m_partyInfoConnection;
    entt::scoped_connection m_partyInviteConnection;
    entt::scoped_connection m_partyJoinedConnection;
    entt::scoped_connection m_partyLeftConnection;
};
