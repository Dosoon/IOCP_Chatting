#pragma once

constexpr uint16_t kPORT = 7777;

constexpr uint16_t kBACKLOG_QUEUE_SIZE = 5;

constexpr auto kDEFAULT_RINGBUFFER_SIZE = 1024 * 16;

constexpr auto kSESSION_RECV_BUF_SIZE = 1024 * 16;

constexpr auto kMAX_SESSION_CNT = 1000;

constexpr auto kSESSION_REUSE_SECONDS = 5;

constexpr auto kREDIS_THREAD_CNT = 1;