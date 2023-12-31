#pragma once

#include <thread>
#include <mutex>
#include <queue>
#include <unordered_map>
#include <concurrent_queue.h>
#include <optional>
#include <functional>

#include "packet.h"
#include "user_manager.h"
#include "redis_manager.h"
#include "room_manager.h"

class PacketManager
{
public:
	void Start(RoomManager* p_ref_room_manager,
			   UserManager* p_ref_user_manager,
			   RedisManager* p_ref_redis_manager);
	void Terminate();

	bool EnqueuePacket(int32_t session_index, const char* p_data, DWORD len);
	void EnqueueSystemPacket(const PacketInfo& packet);
	void SetSendPacket(std::function<void(uint32_t, char*, uint16_t)> send_packet);

private:
	void Init(RoomManager* p_ref_room_manager, UserManager* p_ref_user_manager,
			  RedisManager* p_ref_redis_manager);
	void SetManagers(RoomManager* p_ref_room_manager, UserManager* p_ref_user_manager,
					 RedisManager* p_ref_redis_manager);
	void Run();
	std::function<void(uint32_t, char*, uint16_t)> SendPacketFunc;

	void BindHandler();

	bool ProcessPacket(PacketInfo& pkt, bool user_pkt = false);
	void PacketProcessThread();
	void DestroyPacketProcessThread();
	std::optional<PacketInfo> DequeuePacket();
	std::optional<PacketInfo> DequeueSystemPacket();

	// -----------------------------------------
	// Packet Handlers
	// -----------------------------------------

	void ConnectHandler(uint32_t session_idx, uint16_t data_size, char* p_data);
	void DisconnectHandler(uint32_t session_idx, uint16_t data_size, char* p_data);
	void LoginHandler(uint32_t session_idx, uint16_t data_size, char* p_data);
	void LoginDBResHandler(uint32_t session_idx, uint16_t data_size, char* p_data);
	void EnterRoomHandler(uint32_t session_idx, uint16_t data_size, char* p_data);
	void LeaveRoomHandler(uint32_t session_idx, uint16_t data_size, char* p_data);
	void RoomChatHandler(uint32_t session_idx, uint16_t data_size, char* p_data);

	bool			is_thread_running_ = false;
	std::thread		packet_process_thread_;

	typedef void(PacketManager::* PROCESS_RECV_PACKET_FUNCTION)(uint32_t, uint16_t, char*);
	std::unordered_map<int32_t, PROCESS_RECV_PACKET_FUNCTION> packet_handlers_;

	Concurrency::concurrent_queue<int32_t>		incoming_packet_user_index_queue_;
	Concurrency::concurrent_queue<PacketInfo>	system_packet_queue_;

	UserManager*	p_ref_user_manager_;
	RedisManager*	p_ref_redis_manager_;
	RoomManager*	p_ref_room_manager_;
};