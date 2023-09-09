#pragma once
#include "content_constants.h"
#include "content_config.h"

struct PacketInfo
{
	uint32_t session_index_;
	uint16_t id_;
	uint16_t size_;
	char* data_;
};

#pragma pack(push,1)
struct PACKET_HEADER
{
	uint16_t size_;
	uint16_t id_;
	uint8_t type_;
};

const uint32_t PACKET_HEADER_LENGTH = sizeof(PACKET_HEADER);

//- 로그인 요청

struct LOGIN_REQUEST_PACKET : public PACKET_HEADER
{
	char user_id_[kMAX_USER_ID_LEN + 1];
	char user_pw_[kMAX_USER_PW_LEN + 1];
};
const size_t LOGIN_REQUEST_PACKET_SZIE = sizeof(LOGIN_REQUEST_PACKET);


struct LOGIN_RESPONSE_PACKET : public PACKET_HEADER
{
	uint16_t result_;
};



//- 룸에 들어가기 요청
struct ROOM_ENTER_REQUEST_PACKET : public PACKET_HEADER
{
	int32_t room_number_;
};

struct ROOM_ENTER_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t result_;
};


//- 룸 나가기 요청
struct ROOM_LEAVE_REQUEST_PACKET : public PACKET_HEADER
{
};

struct ROOM_LEAVE_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t result_;
};



// 룸 채팅
struct ROOM_CHAT_REQUEST_PACKET : public PACKET_HEADER
{
	char msg_[kMAX_CHAT_MSG_SIZE + 1] = { 0, };
};

struct ROOM_CHAT_RESPONSE_PACKET : public PACKET_HEADER
{
	int16_t result_;
};

struct ROOM_CHAT_NOTIFY_PACKET : public PACKET_HEADER
{
	char user_id_[kMAX_USER_ID_LEN + 1] = { 0, };
	char msg_[kMAX_CHAT_MSG_SIZE + 1] = { 0, };
};


//- 룸 유저 리스트
struct USER_INFO
{
	uint64_t user_index_;
	int8_t id_len_;
	char user_id_[kMAX_USER_ID_LEN + 1] = { 0, };
};

struct ROOM_USER_LIST_NOTIFY_PACKET : public PACKET_HEADER
{
	int8_t user_count_;
	USER_INFO user_list_[kROOM_MAX_USER_CNT];
};

//- 룸 새 유저 알림
struct ROOM_NEW_USER_NOTIFY_PACKET : public PACKET_HEADER
{
	USER_INFO user_info_;
};

//- 룸 떠난 유저 알림
struct ROOM_LEAVE_USER_NOTIFY_PACKET : public PACKET_HEADER
{
	uint64_t user_index_;
};

#pragma pack(pop)