#include "room.h"

#include "content_constants.h"
#include "packet_generator.h"
#include "packet_id.h"
#include "content_config.h"

#include <Windows.h>
#include <iostream>

void Room::Init(int32_t room_idx)
{
	room_idx_ = room_idx;
	max_user_cnt_ = kROOM_MAX_USER_CNT;
}

void Room::SetSendPacket(std::function<void(uint32_t, char*, uint16_t)> send_packet)
{
	SendPacketFunc = send_packet;
}

ERROR_CODE Room::EnterUser(User* p_user)
{
	if (user_list_.size() >= max_user_cnt_)
	{
		return ERROR_CODE::kENTER_ROOM_FULL_USER;
	}

	user_list_.push_back(p_user);
	++current_user_cnt_;

	p_user->EnterRoom(room_idx_);

	NotifyNewUser(p_user);
	NotifyUserList(p_user);

	return ERROR_CODE::kNONE;
}

void Room::LeaveUser(User* p_leave_user)
{
	std::erase_if(user_list_,
		[leave_user_id = p_leave_user->GetUserID()](User* p_user) {
			return leave_user_id == p_user->GetUserID();
		});

	NotifyLeaveUser(p_leave_user);
}

void Room::NotifyChat(int32_t user_idx, const char* p_user_id, const char* p_msg)
{
	auto notify_pkt = SetPacketIdAndLen<ROOM_CHAT_NOTIFY_PACKET>(PACKET_ID::kROOM_CHAT_NOTIFY);
	CopyMemory(notify_pkt.user_id_, p_user_id, (kMAX_USER_ID_LEN + 1));
	CopyMemory(notify_pkt.msg_, p_msg, (kMAX_CHAT_MSG_SIZE + 1));

	BroadcastMsg(reinterpret_cast<char*>(&notify_pkt), sizeof(notify_pkt), user_idx, false);
}

void Room::NotifyNewUser(User* p_user)
{
	auto enter_user_pkt = SetPacketIdAndLen<ROOM_NEW_USER_NOTIFY_PACKET>(PACKET_ID::kROOM_NEW_USER_NOTIFY);
	enter_user_pkt.user_info_.user_index_ = p_user->GetSessionIdx();
	CopyMemory(enter_user_pkt.user_info_.user_id_, p_user->GetUserID().c_str(), (kMAX_USER_ID_LEN + 1));
	enter_user_pkt.user_info_.id_len_ = p_user->GetUserID().length();

	BroadcastMsg(reinterpret_cast<char*>(&enter_user_pkt), sizeof(enter_user_pkt), p_user->GetSessionIdx(), true);
}

void Room::NotifyUserList(User* p_user)
{
	auto user_list_pkt = SetPacketIdAndLen<ROOM_USER_LIST_NOTIFY_PACKET>
		(PACKET_ID::kROOM_USER_LIST_NOTIFY);

	user_list_pkt.user_count_ = user_list_.size();

	auto idx = 0;
	for (const auto& p_room_user : user_list_) {
		if (p_room_user == nullptr) {
			continue;
		}

		user_list_pkt.user_list_[idx].user_index_ = p_room_user->GetSessionIdx();
		CopyMemory(user_list_pkt.user_list_[idx].user_id_,
			p_room_user->GetUserID().c_str(),
			(kMAX_USER_ID_LEN + 1));
		user_list_pkt.user_list_[idx].id_len_ = p_room_user->GetUserID().length();
		idx++;
	}

	SendPacketFunc(p_user->GetSessionIdx(), (char*)&user_list_pkt, sizeof(user_list_pkt));
}

void Room::NotifyLeaveUser(User* p_user)
{
	auto leave_user_pkt = SetPacketIdAndLen<ROOM_LEAVE_USER_NOTIFY_PACKET>
		(PACKET_ID::kROOM_LEAVE_USER_NOTIFY);

	leave_user_pkt.user_index_ = p_user->GetSessionIdx();

	BroadcastMsg(reinterpret_cast<char*>(&leave_user_pkt), sizeof(leave_user_pkt), p_user->GetSessionIdx(), true);
}

void Room::BroadcastMsg(char* p_data, const uint16_t len, uint32_t pass_user_idx, bool except_me)
{
	for (auto& user : user_list_)
	{
		if (user == nullptr) {
			continue;
		}

		if (except_me && static_cast<int16_t>(user->GetSessionIdx()) == pass_user_idx) {
			continue;
		}

		SendPacketFunc(static_cast<uint32_t>(user->GetSessionIdx()), p_data, len);
	}
}
