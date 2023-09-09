#pragma once
#pragma comment(lib, "ws2_32")
#pragma comment(lib, "mswsock")
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <MSWSock.h>

#include <cstdint>
#include <mutex>

#include "overlapped_ex_define.h"
#include "ring_buffer.h"
#include "network_config.h"

// ���� ���� ����
class Session
{
public:
	Session() {
		recv_buf_ = new char[kSESSION_RECV_BUF_SIZE];
	}
	~Session() {
		delete[] recv_buf_;
	}
	void Init(int32_t idx);
	void Activate();

	// ------------------------------
	// Accept
	// ------------------------------

	bool BindAccept(SOCKET listen_socket);
	void SetAcceptOverlapped();
	bool GetSessionIpPort(char* ip_dest, int32_t ip_len, uint16_t& port_dest);

	// ------------------------------
	// Send & Recv
	// ------------------------------

	bool BindSend();
	void SetSendOverlapped(WSABUF* wsa_buf, int32_t& buffer_cnt);
	void SetWsaBuf(WSABUF* wsa_buf, int32_t& buffer_cnt);

	bool BindRecv();
	void SetRecvOverlapped();

	// ------------------------------
	// Send ���� ����
	// ------------------------------

	int32_t EnqueueSendData(char* data_, int32_t data_len);
	void ClearSendBuffer();
	bool HasSendData();

	// ------------------------------
	// Getter & Setter
	// ------------------------------

	void SetConnectionClosedTime(uint64_t time)
	{
		latest_conn_closed_.store(time);
	}

	uint64_t GetConnectionClosedTime()
	{
		return latest_conn_closed_;
	}

	void SetSocket(SOCKET sock)
	{
		socket_ = sock;
	}

	SOCKET GetSocket()
	{
		return socket_;
	}



	int32_t				index_ = -1;							// ���� �ε���
	SOCKET				socket_ = INVALID_SOCKET;				// Ŭ���̾�Ʈ ����
	char*				recv_buf_;								// ���� ������ ����
	RingBuffer			send_buf_;								// �۽� �����Ϳ� �� ����

	std::atomic<bool>	is_activated_ = false;					// ���� ������ ��������� ����
	std::atomic<bool>	is_sending_ = false;					// ���� �۽� ������ ����
	SRWLOCK				send_lock_;								// �۽� �����Ϳ� ���� ��

	OverlappedEx		accept_overlapped_ex_;					// Accept I/O�� ���� OverlappedEx ����ü
	OverlappedEx		recv_overlapped_ex_;					// Recv I/O�� ���� OverlappedEx ����ü
	OverlappedEx		send_overlapped_ex_;					// Send I/O�� ���� OverlappedEx ����ü

	char					accept_buf_[64] = { 0, };			// ������ ������ �������� ���� ����
	std::atomic<uint64_t>	latest_conn_closed_ = UINT64_MAX;	// ���� �ֱٿ� ������ ���� �ð�
};