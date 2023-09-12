# 💬 IOCP Chatting Server

**IOCP 모델 기반 멀티스레드 채팅 서버**

---

# :computer: 구현 내용

- **IOCP 모델 기반** 멀티스레드 소켓 서버
- Windows Socket API 사용
- **RingBuffer**를 사용한 **I/O 최적화**
- Redis 연동
  - `hiredis`, `redis_client` 라이브러리 사용

---

# :seedling: 기술 스택

- C++
- Windows Socket API
- Visual Studio 2022
- Redis

---

# :chart_with_upwards_trend: 서버 구조

![](docs/001.png)

---

# :chart_with_upwards_trend: 패킷 시퀀스 다이어그램

## 시스템 패킷

유저의 요청이 아니라 Connect, Disconnect시에 서버가 생성하는 패킷.<br>
**System Packet Queue**에 Enqueue되어 처리된다.

### 접속

```mermaid
sequenceDiagram

Note over Client,Server:Connect
Server-)Server:kSYS_USER_CONNECT
Note over Server:유저 데이터 초기화
```

### 접속 종료

```mermaid
sequenceDiagram

Note over Client,Server:Disconnect
Server-)Server:kSYS_USER_DISCONNECT
alt 유저가 룸에 있는 경우
    Server--)Client:kROOM_LEAVE_USER_NOTIFY<br>(다른 클라이언트에게 퇴장 알림)
    Note over Server:룸에서 유저 데이터 삭제
end
```

## 일반 패킷

유저의 요청에 따라 처리되는 패킷.<br>
**User Packet Queue**에 Enqueue되어 처리된다.

### 로그인

```mermaid
sequenceDiagram

Client-)Server:kLOGIN_REQUEST
Note over Server: 현재 접속 유저수 체크
alt 최대 인원 도달
Server--)Client:kLOGIN_RESPONSE(실패)
end
Note over Server: 이미 접속한 유저인지 검증
alt 이미 접속한 유저
Server--)Client:kLOGIN_RESPONSE(실패)
end
Server-)Redis: kREQUEST_LOGIN
Note over Redis:유저 로그인 정보 검증
Redis--)Server: kRESPONSE_LOGIN
Server--)Client: kLOGIN_RESPONSE
```

### 방 입장

```mermaid
sequenceDiagram

Client-)Server:kROOM_ENTER_REQUEST
Note over Server: 로그인 후<br>로비에 있는 유저인지 검증
alt 로비에 있지 않음
Server--)Client:kROOM_ENTER_RESPONSE(실패)
else 로비에 있음
Note over Server: 방 입장 처리
Server--)Client:[다른 클라이언트에 Broadcast]<br>kROOM_NEW_USER_NOTIFY
Server--)Client:kROOM_USER_LIST_NOTIFY
Server--)Client:kROOM_ENTER_RESPONSE
end
```

### 방 퇴장

```mermaid
sequenceDiagram


Client-)Server:kROOM_LEAVE_REQUEST
Note over Server: 방 입장 상태인지 검증
alt 방 입장 상태가 아님
Server--)Client:kROOM_LEAVE_RESPONSE(실패)
else 방 입장 상태
Note over Server: 방 퇴장 처리
Server--)Client:[다른 클라이언트에 Broadcast]<br>kROOM_LEAVE_USER_NOTIFY
Server--)Client:kROOM_LEAVE_RESPONSE
end
```

### 채팅

```mermaid
sequenceDiagram

Client-)Server:kROOM_CHAT_REQUEST
Note over Server: 방 입장 상태인지 검증
alt 방 입장 상태가 아니거나<br>유효하지 않은 방 번호
Server--)Client:kROOM_CHAT_RESPONSE(실패)
else 방 입장 상태
Server--)Client:[다른 클라이언트에 Broadcast]<br>kROOM_CHAT_NOTIFY
Server--)Client:kROOM_CHAT_RESPONSE
end
```
