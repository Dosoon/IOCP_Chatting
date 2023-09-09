#include "chat_server.h"

#include <iostream>
#include <mutex>

#include "packet_id.h"

/// <summary> <para>
/// �����ص� �ݹ��Լ��� ��Ʈ��ũ Ŭ������ �����ϰ�, </para> <para>
/// ���� ������ ���� �� ��Ʈ��ũ�� �����Ѵ�. </para>
/// </summary>
bool ChatServer::Start()
{
	is_server_running_ = true;

	SetDelegate();

	if (!network_.Start()) {
		std::cout << "[StartServer] Failed to Start\n";
		return false;
	}

	packet_manager_.Start(&room_manager_, &user_manager_, &redis_manager_);

	redis_manager_.Start(&user_manager_);

	std::cout << "[StartServer] Server Started\n";
	return true;
}

/// <summary>
/// ��Ʈ��ũ Ŭ������ ��Ŷ �Ŵ����� �����Ų ��, ������ �����Ѵ�.
/// </summary>
void ChatServer::Terminate()
{
	network_.Terminate();
	packet_manager_.Terminate();
	is_server_running_ = false;
}

/// <summary>
/// ��Ʈ��ũ �ܿ��� ���� Delegate �Լ��� ����
/// </summary>
void ChatServer::SetDelegate()
{
	// ��Ʈ��ũ���� Accept, Recv, Disconnect �߻� �� ���� �������� ������ Delegate ����
	network_.SetOnConnect(std::bind(&ChatServer::OnConnect, this, std::placeholders::_1));
	network_.SetOnRecv(std::bind(&ChatServer::OnRecv, this,
		std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	network_.SetOnDisconnect(std::bind(&ChatServer::OnDisconnect, this, std::placeholders::_1));

	// ��Ŷ �Ŵ������� ���� SendPacket �Լ� ����
	packet_manager_.SetSendPacket(std::bind(&Network::SendPacket, &network_,
		std::placeholders::_1, std::placeholders::_2,
		std::placeholders::_3));

	// �� �Ŵ������� ���� SendPacket �Լ� ����
	room_manager_.SetSendPacket(std::bind(&Network::SendPacket, &network_,
		std::placeholders::_1, std::placeholders::_2,
		std::placeholders::_3));
}

/// <summary>
/// ��Ʈ��ũ �ܿ��� Accept�ÿ� �����
/// </summary>
void ChatServer::OnConnect(int32_t session_idx)
{
	// ��Ŷ �Ŵ����� �ý��� ��Ŷ ����
	PacketInfo pkt{
		.session_index_ = static_cast<uint16_t>(session_idx),
		.id_ = static_cast<int32_t>(PACKET_ID::kSYS_USER_CONNECT),
		.size_ = 0,
		.data_ = nullptr
	};

	packet_manager_.EnqueueSystemPacket(pkt);
}

/// <summary>
/// ��Ʈ��ũ �ܿ��� Recv�ÿ� �����
/// </summary>
void ChatServer::OnRecv(int32_t session_idx, const char* p_data, DWORD len)
{
	// ��Ŷ �Ŵ����� ����
	packet_manager_.EnqueuePacket(session_idx, p_data, len);
}

/// <summary>
/// ��Ʈ��ũ �ܿ��� ���� Disconnect�ÿ� �����
/// </summary>
void ChatServer::OnDisconnect(int32_t session_idx)
{
	// ��Ŷ �Ŵ����� �ý��� ��Ŷ ����
	PacketInfo pkt{
		.session_index_ = static_cast<uint16_t>(session_idx),
		.id_ = static_cast<int32_t>(PACKET_ID::kSYS_USER_DISCONNECT),
		.size_ = 0,
		.data_ = nullptr
	};

	packet_manager_.EnqueueSystemPacket(pkt);
}