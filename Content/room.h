#pragma once

#include <cstdint>
#include <vector>
#include <functional>

#include "user.h"
#include "error_code.h"

class Room
{
public:

	void Init(int32_t room_idx);

	// ------------------------------
	// Getter & Setter
	// ------------------------------
	int32_t GetRoomIdx()
	{
		return room_idx_;
	}

	std::vector<User*>& GetUserList()
	{
		return user_list_;
	}

	void SetSendPacket(std::function<void(uint32_t, char*, uint16_t)> send_packet);

	// ------------------------------
	// Room ·ÎÁ÷
	// ------------------------------

	ERROR_CODE EnterUser(User* p_user);
	void LeaveUser(User* p_user);
	void NotifyChat(int32_t user_idx, const char* p_user_id, const char* p_msg);

private:
	void NotifyNewUser(User* p_user);
	void NotifyUserList(User* p_user);
	void NotifyLeaveUser(User* p_user);

	std::function<void(uint32_t, char*, uint16_t)> SendPacketFunc;

	void BroadcastMsg(char* p_data, const uint16_t len, uint32_t pass_user_idx, bool except_me);

	int32_t room_idx_ = -1;

	std::vector<User*> user_list_;

	int32_t max_user_cnt_ = 0;

	uint16_t current_user_cnt_ = 0;
};

