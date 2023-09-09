#include "session.h"

#include <iostream>

#include "srw_lock_guard.h"
#include "error_handler.h"

/// <summary>
/// 세션 초기화
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
/// 세션 재사용 전 데이터 초기화 및 활성 상태 ON으로 변경
/// </summary>
void Session::Activate()
{
	ZeroMemory(&recv_overlapped_ex_, sizeof(OverlappedEx));
	ZeroMemory(&send_overlapped_ex_, sizeof(OverlappedEx));
	is_activated_.store(true);
	send_buf_.ClearBuffer();
}

/// <summary>
/// AcceptEx 비동기 요청
/// </summary>
bool Session::BindAccept(SOCKET listen_socket)
{
	// 중복 요청 방지를 위한 time값 조정
	latest_conn_closed_ = UINT64_MAX;

	// 소켓 생성
	socket_ = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
	if (socket_ == INVALID_SOCKET) {
		return false;
	}

	// Accept 오버랩 초기화 후 AcceptEx 걸기
	SetAcceptOverlapped();

	DWORD bytes_recv = 0;
	auto accept_ret = AcceptEx(listen_socket, socket_, accept_buf_, 0, sizeof(SOCKADDR_IN) + 16,
		sizeof(SOCKADDR_IN) + 16, &bytes_recv, reinterpret_cast<LPOVERLAPPED>(&accept_overlapped_ex_));

	return ErrorHandler::Handle(accept_ret, WSAGetLastError, "AcceptEx", 1, WSA_IO_PENDING);
}

/// <summary>
/// Accept OverlappedEx 초기화
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
/// 세션 데이터를 토대로 IP 주소와 포트 번호를 가져온다.
/// </summary>
bool Session::GetSessionIpPort(char* ip_dest, int32_t ip_len, uint16_t& port_dest)
{
	// Peer 정보 가져오기
	SOCKADDR_IN* local_addr = NULL, * session_addr = NULL;
	int32_t session_addr_len = sizeof(session_addr);

	GetAcceptExSockaddrs(accept_buf_, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16,
		reinterpret_cast<SOCKADDR**>(&local_addr), &session_addr_len,
		reinterpret_cast<SOCKADDR**>(&session_addr), &session_addr_len);

	// IP 주소 문자열로 변환
	inet_ntop(AF_INET, &session_addr->sin_addr, ip_dest, ip_len);

	// 포트 정보
	port_dest = session_addr->sin_port;

	return true;
}

/// <summary>
/// WSASend를 Call하여 비동기 Send 요청
/// </summary>
bool Session::BindSend()
{
	// CAS 연산을 통해 이미 Send요청이 갔는지 확인 (1-Send)
	bool sending_expected = false;
	if (!is_sending_.compare_exchange_strong(sending_expected, true)) {
		return true;
	}

	// Send 링버퍼 락 -------------------------------------
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

	// Send 요청 완료된 만큼 링버퍼에서 데이터 제거
	send_buf_.MoveFront(sent_bytes);

	return true;
}

/// <summary>
/// SendOverlappedEx 설정
/// </summary>
void Session::SetSendOverlapped(WSABUF* wsa_buf, int32_t& buffer_cnt)
{
	// 소켓, 버퍼 정보 및 I/O Operation 타입 설정
	send_overlapped_ex_.socket_ = socket_;
	send_overlapped_ex_.op_type_ = IOOperation::kSEND;

	// WSABUF 설정 : Send 링버퍼에 맞게 설정
	SetWsaBuf(wsa_buf, buffer_cnt);
}

/// <summary>
/// Send 링 버퍼에 있는 데이터 크기에 따라 WSABUF 설정
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
/// WSARecv Overlapped I/O 작업 요청
/// </summary>
bool Session::BindRecv()
{
	DWORD flag = 0;
	DWORD recved_bytes = 0;

	SetRecvOverlapped();

	// Recv 요청
	auto recv_ret = WSARecv(socket_, &recv_overlapped_ex_.wsa_buf_, 1, &recved_bytes, &flag,
		&recv_overlapped_ex_.wsa_overlapped_, NULL);

	if (!ErrorHandler::Handle(recv_ret, &WSAGetLastError, "WSARecv", 1, ERROR_IO_PENDING)) {
		return false;
	}

	return true;
}

/// <summary>
/// RecvOverlappedEx를 설정한다.
/// </summary>
void Session::SetRecvOverlapped()
{
	recv_overlapped_ex_.socket_ = socket_;
	recv_overlapped_ex_.wsa_buf_.len = kSESSION_RECV_BUF_SIZE;
	recv_overlapped_ex_.wsa_buf_.buf = recv_buf_;
	recv_overlapped_ex_.op_type_ = IOOperation::kRECV;
}

/// <summary>
/// 락을 걸고 세션의 Send 링 버퍼에 데이터를 넣는다.
/// </summary>
int32_t Session::EnqueueSendData(char* data_, int32_t len)
{
	SRWWriteLockGuard lock(&send_lock_);

	// Enqueue에 성공한 크기를 리턴
	return send_buf_.Enqueue(data_, len);
}

/// <summary>
/// 락 걸고 Send 링버퍼를 초기화
/// </summary>
void Session::ClearSendBuffer()
{
	SRWWriteLockGuard lock(&send_lock_);
	send_buf_.ClearBuffer();
}

/// <summary>
/// Send 링 버퍼에 보낼 데이터가 있는지 여부를 반환한다.
/// </summary>
bool Session::HasSendData()
{
	SRWWriteLockGuard lock(&send_lock_);

	// Send 링버퍼에 데이터가 있는지 확인
	return send_buf_.GetSizeInUse() > 0;
}