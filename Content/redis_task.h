#pragma once
#include "error_code.h"
#include "content_constants.h"


enum class REDIS_TASK_ID : uint16_t
{
	kINVALID = 0,

	kREQUEST_LOGIN = 1001,
	kRESPONSE_LOGIN = 1002,
};

struct RedisTask
{
	uint32_t user_idx_ = 0;
	REDIS_TASK_ID id_ = REDIS_TASK_ID::kINVALID;
	uint16_t size_ = 0;
	char* p_data_ = nullptr;

	void Release()
	{
		if (p_data_ != nullptr)
		{
			delete[] p_data_;
		}
	}
};

#pragma pack(push,1)

struct RedisLoginReq
{
	char user_id_[kMAX_USER_ID_LEN + 1];
	char user_pw_[kMAX_USER_PW_LEN + 1];
};

struct RedisLoginRes
{
	uint16_t result_ = (uint16_t)ERROR_CODE::kNONE;
};

#pragma pack(pop)