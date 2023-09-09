#pragma once

#include <thread>
#include <vector>
#include <functional>

#include "session.h"
#include "network_config.h"

class Network
{
public:
	Network() { }
	~Network()
	{
		WSACleanup();
	}

	// ------------------------------
	// ���� �� ����
	// ------------------------------

	bool Start();
	void Terminate();

	// ------------------------------
	// ���� �ܿ��� ������ Callback Setter
	// ------------------------------

	void SetOnConnect(std::function<void(int32_t)> on_connect)
	{
		OnConnect = on_connect;
	}

	void SetOnRecv(std::function<void(int32_t, const char*, DWORD)> on_recv)
	{
		OnRecv = on_recv;
	}

	void SetOnDisconnect(std::function<void(int32_t)> on_disconnect)
	{
		OnDisconnect = on_disconnect;
	}

	// ------------------------------
	// �����ܿ����� Send ȣ���� ���� Public Send Func
	// ------------------------------

	void SendPacket(int32_t session_idx, const char* p_data, const uint16_t len)
	{
		if (session_idx < 0 || session_idx >= kMAX_SESSION_CNT)
		{
			return;
		}

		Session& Session = session_list_[session_idx];

		Session.EnqueueSendData(const_cast<char*>(p_data), len);
	}

	/// <summary>
	/// ���� �ε����� ���� �����͸� ��ȯ�Ѵ�.
	/// </summary>
	Session* GetSessionByIdx(int32_t session_idx)
	{
		if (session_idx < 0 || session_idx >= kMAX_SESSION_CNT)
		{
			return nullptr;
		}

		return &session_list_[session_idx];
	}

private:
	bool InitListenSocket();
	bool BindAndListen();
	bool CreateIOCP();
	bool CreateWorkerAndIOThread();

	void InitSessionPool();
	bool CreateWorkerThread();
	bool CreateAccepterThread();
	bool CreateSenderThread();

	bool BindIOCompletionPort(Session* p_session);

	void WorkerThread();
	void AccepterThread();
	void SenderThread();

	void CloseSocket(Session* p_session, bool is_force = false);
	void ClearSession(Session* p_session);

	void DispatchOverlapped(Session* p_session, DWORD io_size, LPOVERLAPPED p_overlapped);
	bool CheckGQCSResult(Session* p_session, bool gqcs_ret, DWORD io_size, LPOVERLAPPED p_overlapped);

	void DestroyThread();
	void DestroyWorkerThread();
	void DestroyAccepterThread();
	void DestroySenderThread();

	/// <summary>
	/// ������鿡�� ���� �޽����� ������.
	/// </summary>
	void PostTerminateMsg()
	{
		PostQueuedCompletionStatus(iocp_, 0, NULL, NULL);
	}

	/// <summary>
	/// ������ ���� ó�� �޽������� Ȯ���Ѵ�.
	/// </summary>
	bool TerminateMsg(bool gqcs_ret, DWORD io_size, LPOVERLAPPED p_overlapped)
	{
		return (gqcs_ret == true && io_size == 0 && p_overlapped == NULL);
	}

	/// <summary>
	/// Ŭ���̾�Ʈ�� ���� �����ߴ��� Ȯ���Ѵ�.
	/// </summary>
	bool SessionExited(bool gqcs_ret, DWORD io_size, LPOVERLAPPED p_overlapped)
	{
		// GQCS false -> Ŭ���̾�Ʈ ������ ����(graceful shutdown�� �ƴ� ���) Ȥ�� GQCS Ÿ�Ӿƿ�
		// GQCS true, IO Size 0 -> graceful shutdown
		return (gqcs_ret == false ||
			(p_overlapped != NULL && io_size == 0 && reinterpret_cast<OverlappedEx*>(p_overlapped)->op_type_ != IOOperation::kACCEPT));
	}

	/// <summary> <para>
	/// �ھ� ���� ������� Max Worker Thread ������ ����� ��ȯ�մϴ�. </para> <para>
	/// ���� ��� ��� : �ھ� �� * 2 + 1 </para>
	/// </summary>
	int32_t GetMaxWorkerThread()
	{
		return std::thread::hardware_concurrency() * 2 + 1;
	}

	SOCKET						listen_socket_ = INVALID_SOCKET;

	std::atomic<uint32_t>		current_session_cnt_ = 0;
	Session*					session_list_ = nullptr;

	std::vector<std::thread>	worker_thread_list_;
	std::thread					accepter_thread_;
	std::thread					sender_thread_;

	HANDLE						iocp_ = INVALID_HANDLE_VALUE;
	bool						is_worker_running_ = true;
	bool						is_accepter_running_ = true;
	bool						is_sender_running_ = true;

	std::function<void(int32_t)>							OnConnect = NULL;
	std::function<void(int32_t, const char*, DWORD)>		OnRecv = NULL;
	std::function<void(int32_t)>							OnDisconnect = NULL;
};