#pragma once

#include <cstdint>
#include <vector>
#include <functional>

#include "room.h"
#include "content_config.h"

class RoomManager
{
public:
	~RoomManager();

	void Init();

	// ------------------------------
	// Getter & Setter
	// ------------------------------

	int32_t getMaxRoomCount()
	{
		return kMAX_ROOM_CNT;
	}

	void SetSendPacket(std::function<void(uint32_t, char*, uint16_t)> send_packet);

	Room* GetRoomByIdx(const int32_t room_idx);

	// ------------------------------
	// RoomManager ·ÎÁ÷
	// ------------------------------

	ERROR_CODE EnterRoom(User* p_user, const int32_t room_idx);
	ERROR_CODE LeaveRoom(User* p_user);

private:
	std::function<void(uint32_t, char*, uint16_t)> SendPacketFunc;

	std::vector<Room*> room_list_;
};

