#include "session.h"

#include <iostream>

#include "srw_lock_guard.h"
#include "error_handler.h"

/// <summary>
/// ���� �ʱ�ȭ
/// </summary>
void Session::Init(int32_t idx)
{
	index_ = idx;

	ZeroMemory(&accept_overlapped_ex_, sizeof(OverlappedEx));
	ZeroMemory(&recv_overlapped_ex_, sizeof(OverlappedEx));
	ZeroMemory(&send_overlapped_ex_, sizeof(OverlappedEx));
	socket_ = INVALID_SOCKET;
	latest_conn_closed_ = 0;

	InitializeSRWLock(&send_lock_);
}

/// <summary>
/// ���� ���� �� ������ �ʱ�ȭ �� Ȱ�� ���� ON���� ����
/// </summary>
void Session::Activate()
{
	ZeroMemory(&recv_overlapped_ex_, sizeof(OverlappedEx));
	ZeroMemory(&send_overlapped_ex_, sizeof(OverlappedEx));
	is_activated_.store(true);
	send_buf_.ClearBuffer();
}

/// <summary>
/// AcceptEx �񵿱� ��û
/// </summary>
bool Session::BindAccept(SOCKET listen_socket)
{
	// �ߺ� ��û ������ ���� time�� ����
	latest_conn_closed_ = UINT64_MAX;

	// ���� ����
	socket_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (socket_ == INVALID_SOCKET) {
		return false;
	}

	// Accept ������ �ʱ�ȭ �� AcceptEx �ɱ�
	SetAcceptOverlapped();

	DWORD bytes_recv = 0;
	auto accept_ret = AcceptEx(listen_socket, socket_, accept_buf_, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, &bytes_recv, reinterpret_cast<LPOVERLAPPED>(&accept_overlapped_ex_));

	return ErrorHandler::Handle(accept_ret, WSAGetLastError, "AcceptEx", 1, WSA_IO_PENDING);
}

/// <summary>
/// Accept OverlappedEx �ʱ�ȭ
/// </summary>
void Session::SetAcceptOverlapped()
{
	ZeroMemory(&accept_overlapped_ex_, sizeof(OverlappedEx));
	accept_overlapped_ex_.session_idx_ = index_;
	accept_overlapped_ex_.socket_ = socket_;
	accept_overlapped_ex_.wsa_buf_.buf = NULL;
	accept_overlapped_ex_.wsa_buf_.len = 0;
	accept_overlapped_ex_.op_type_ = IOOperation::kACCEPT;
}

/// <summary>
/// ���� �����͸� ���� IP �ּҿ� ��Ʈ ��ȣ�� �����´�.
/// </summary>
bool Session::GetSessionIpPort(char* ip_dest, int32_t ip_len, uint16_t& port_dest)
{
	// Peer ���� ��������
	SOCKADDR_IN* local_addr = NULL, * session_addr = NULL;
	int32_t session_addr_len = sizeof(session_addr);

	GetAcceptExSockaddrs(accept_buf_, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		reinterpret_cast<SOCKADDR**>(&local_addr), &session_addr_len,
		reinterpret_cast<SOCKADDR**>(&session_addr), &session_addr_len);

	// IP �ּ� ���ڿ��� ��ȯ
	inet_ntop(AF_INET, &session_addr->sin_addr, ip_dest, ip_len);

	// ��Ʈ ����
	port_dest = session_addr->sin_port;

	return true;
}

/// <summary>
/// WSASend�� Call�Ͽ� �񵿱� Send ��û
/// </summary>
bool Session::BindSend()
{
	// CAS ������ ���� �̹� Send��û�� ������ Ȯ�� (1-Send)
	bool sending_expected = false;
	if (!is_sending_.compare_exchange_strong(sending_expected, true)) {
		return true;
	}

	// Send ������ �� -------------------------------------
	SRWWriteLockGuard lock(&send_lock_);

	DWORD sent_bytes = 0;
	WSABUF wsa_buf[2];
	int32_t buffer_cnt = 1;
	SetSendOverlapped(wsa_buf, buffer_cnt);

	auto send_ret = WSASend(socket_, wsa_buf, buffer_cnt, &sent_bytes, 0,
		reinterpret_cast<LPWSAOVERLAPPED>(&send_overlapped_ex_), NULL);

	if (!ErrorHandler::Handle(send_ret, &WSAGetLastError, "WSASend", 1, ERROR_IO_PENDING)) {
		return false;
	}

	// Send ��û �Ϸ�� ��ŭ �����ۿ��� ������ ����
	send_buf_.MoveFront(sent_bytes);

	return true;
}

/// <summary>
/// SendOverlappedEx ����
/// </summary>
void Session::SetSendOverlapped(WSABUF* wsa_buf, int32_t& buffer_cnt)
{
	// ����, ���� ���� �� I/O Operation Ÿ�� ����
	send_overlapped_ex_.socket_ = socket_;
	send_overlapped_ex_.op_type_ = IOOperation::kSEND;

	// WSABUF ���� : Send �����ۿ� �°� ����
	SetWsaBuf(wsa_buf, buffer_cnt);
}

/// <summary>
/// Send �� ���ۿ� �ִ� ������ ũ�⿡ ���� WSABUF ����
/// </summary>
void Session::SetWsaBuf(WSABUF* wsa_buf, int32_t& buffer_cnt)
{
	wsa_buf[0].buf = send_buf_.GetFrontBufferPtr();
	wsa_buf[0].len = send_buf_.GetContinuousDequeueSize();

	if (send_buf_.GetSizeInUse() > send_buf_.GetContinuousDequeueSize()) {
		wsa_buf[1].buf = send_buf_.GetBufferPtr();
		wsa_buf[1].len = send_buf_.GetSizeInUse() - send_buf_.GetContinuousDequeueSize();
		++buffer_cnt;
	}
}

/// <summary>
/// WSARecv Overlapped I/O �۾� ��û
/// </summary>
bool Session::BindRecv()
{
	DWORD flag = 0;
	DWORD recved_bytes = 0;

	SetRecvOverlapped();

	// Recv ��û
	auto recv_ret = WSARecv(socket_, &recv_overlapped_ex_.wsa_buf_, 1, &recved_bytes, &flag,
		&recv_overlapped_ex_.wsa_overlapped_, NULL);

	if (!ErrorHandler::Handle(recv_ret, &WSAGetLastError, "WSARecv", 1, ERROR_IO_PENDING)) {
		return false;
	}

	return true;
}

/// <summary>
/// RecvOverlappedEx�� �����Ѵ�.
/// </summary>
void Session::SetRecvOverlapped()
{
	recv_overlapped_ex_.socket_ = socket_;
	recv_overlapped_ex_.wsa_buf_.len = kSESSION_RECV_BUF_SIZE;
	recv_overlapped_ex_.wsa_buf_.buf = recv_buf_;
	recv_overlapped_ex_.op_type_ = IOOperation::kRECV;
}

/// <summary>
/// ���� �ɰ� ������ Send �� ���ۿ� �����͸� �ִ´�.
/// </summary>
int32_t Session::EnqueueSendData(char* data_, int32_t len)
{
	SRWWriteLockGuard lock(&send_lock_);

	// Enqueue�� ������ ũ�⸦ ����
	return send_buf_.Enqueue(data_, len);
}

/// <summary>
/// �� �ɰ� Send �����۸� �ʱ�ȭ
/// </summary>
void Session::ClearSendBuffer()
{
	SRWWriteLockGuard lock(&send_lock_);
	send_buf_.ClearBuffer();
}

/// <summary>
/// Send �� ���ۿ� ���� �����Ͱ� �ִ��� ���θ� ��ȯ�Ѵ�.
/// </summary>
bool Session::HasSendData()
{
	SRWWriteLockGuard lock(&send_lock_);

	// Send �����ۿ� �����Ͱ� �ִ��� Ȯ��
	return send_buf_.GetSizeInUse() > 0;
}