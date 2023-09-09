#pragma once

#include <cstdint>
#include <type_traits>

#include "redis_task.h"
#include "packet_id.h"

// Packet에 대한 concept 정의
template <typename T>
concept Packet = requires(T t) {
	{ t.id_ } -> std::convertible_to<uint16_t>;
	{ t.size_ } -> std::convertible_to<uint16_t>;
};

template <Packet Pkt>
Pkt SetPacketIdAndLen(PACKET_ID id)
{
	Pkt pkt;

	pkt.id_ = static_cast<uint16_t>(id);
	pkt.size_ = sizeof(Pkt);

	return pkt;
};

template <typename TaskBody>
RedisTask SetTaskBody(uint32_t session_idx, REDIS_TASK_ID id, TaskBody body)
{
	RedisTask task;

	task.user_idx_ = session_idx;
	task.id_ = id;
	task.size_ = sizeof(body);
	task.p_data_ = new char[task.size_];
	CopyMemory(task.p_data_, reinterpret_cast<char*>(&body), task.size_);

	return task;
};