#pragma once
#include <deque>
#include <mutex>

#include "packet.h"
#include "network.h"
#include "packet_manager.h"

class ChatServer
{
public:
	bool Start();
	void Terminate();
	bool IsServerRunning()
	{
		return is_server_running_;
	}

private:
	void SetDelegate();
	void OnConnect(int32_t session_idx);
	void OnRecv(int32_t session_idx, const char* p_data, DWORD len);
	void OnDisconnect(int32_t session_idx);

	Network network_;
	bool is_server_running_ = false;

	PacketManager	packet_manager_;
	RoomManager		room_manager_;
	UserManager		user_manager_;
	RedisManager	redis_manager_;
};