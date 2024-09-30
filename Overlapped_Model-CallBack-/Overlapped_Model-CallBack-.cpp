#pragma once
#include <WinSock2.h>
#include <mswsock.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#pragma comment(lib,"ws2_32.lib")

using namespace std;

void HandleError(const char* cause)
{
	int32_t errCode = ::WSAGetLastError();
	cout << "ErrorCode : " << errCode << endl;
}

const int32_t BUFSIZE = 1000;

struct Session
{
	SOCKET socket = INVALID_SOCKET;
	char recvBuffer[BUFSIZE] = {};
	int32_t recvBytes = 0;
	int32_t sendBytes = 0;
	WSAOVERLAPPED overlapped = {};
};

void CALLBACK RecvCallback(DWORD error, DWORD recvLen, LPWSAOVERLAPPED overlapped, DWORD flags)
{
	cout << "Data Recv Len Callback =" << recvLen << endl;
	//TODO: 에코 서버일 경우 WSASend()등을 호출
}

int main()
{

	WSAData wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
		cout << "start up 에러" << endl;
		return 0;
	}

	//논블로킹(non-blocking)
	SOCKET listenSocket = ::socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket == INVALID_SOCKET)
		return 0;
	u_long on = 1;
	if (::ioctlsocket(listenSocket, FIONBIO, &on) == INVALID_SOCKET)
		return 0;

	SOCKADDR_IN serverAddr;
	::memset(&serverAddr, 0, sizeof(serverAddr));
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = ::htonl(INADDR_ANY);
	serverAddr.sin_port = ::htons(5252);

	if (::bind(listenSocket, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR)
		return 0;

	if (::listen(listenSocket, SOMAXCONN) == SOCKET_ERROR)
		return 0;

	cout << "Accept" << endl;
	//Overlapped IO (비동기 + 논블로킹)
	// - Overlapped 함수를 건다(WSARecv,WSASend)
	// - Overlapped 함수가 성공했는지 확인 후
	// -> 성공시 결과 얻어서 처리
	// -> 실패시 사유를 확인

	//1) 비동기 입출력 소켓
	//2) WSABUF 라는 배열의 시작주소 +개수
	//3) 보내고/받은 바이트 수
	//4) 상세 옵션 
	//5) WSAOVERLAPPED구조체 주소값
	//6) 입출력이 완료되면 OS가 호출할 콜백 함수 - 이프로젝트에선 skip -
	//비동기식이라 내가 호출을했다고 바로 내부에서 이 함수가 호출을 하였다고 볼 수 없다.
	//그렇기 때문에 만약 호출 후 내가 보내야할 버퍼를 건드리거나하면 원하지 않은 값이 전달되거나 할 수 있다.
	//WSASend
	//WSARecv

	//Overlapped 모델 (Completion Routin 콜백 기반)
	// - 비동기 입출력 지원하는 소켓 생성 
	// - 비동기 입출력 함수 호출 (완료 루틴의 시작주소를 넘겨준다.)
	// - 비동기 작업이 바로 완료되지 않으면 , WSA_IO_PENDING 오류 코드
	// - 비동기 입출력 함수 호출한 쓰레드를 -> Alertable Wait상태로 만든다.
	// ex) WaitForSingleObjectEX,WaitForMultipleObjectsEx,SleepEx,WSAWAitForMultipleEvents
	// 비동기IO 완료되면,운영체제는 완료 루틴 호출 
	// - 완료 루틴 호출이 모두 끝나면 , 쓰레드는 Alertable Wait상태에서 빠저나온다.


	//1) 오류 발생시 0 아닌 값
	//2) 전송 바이트 수
	//3) 비동기 입출력 함수 호출시 넘겨준 WSAOVERLAPPED구조체의 주소값
	//4)0
	//void CompletionRoutine()
	//WSARecv()의 마지막인자에서 콜백 함수를 받아주는데 함수형포인터를 받아준다. 정의를 자세히보면 아무 함수나 받는게 아니라 어느정도 규격이있다.
	while (true)
	{
		SOCKADDR_IN clientAddr;
		int32_t addrLen = sizeof(clientAddr);
		SOCKET clientSocket;

		while (true)
		{
			clientSocket = ::accept(listenSocket, (SOCKADDR*)&clientAddr, &addrLen);
			if (clientSocket != INVALID_SOCKET)
				break;
			if (::WSAGetLastError() == WSAEWOULDBLOCK)
				continue;
			//문제가 있는 상황
			return 0;
		}

		Session session = Session{ clientSocket };
		//WSAEVENT wsaEvent = ::WSACreateEvent();


		cout << "Client Connected !" << endl;

		while (true)
		{
			//wsaBuf의 메모리는 일이끝나고 날려도되지만 recvBuffer는 절대 안됨.
			WSABUF wsaBuf;
			wsaBuf.buf = session.recvBuffer;
			wsaBuf.len = BUFSIZE;

			DWORD recvLen = 0;
			DWORD flags = 0;
			if (::WSARecv(clientSocket, &wsaBuf, 1, &recvLen, &flags, &session.overlapped, RecvCallback) == SOCKET_ERROR)
			{
				if (::WSAGetLastError() == WSA_IO_PENDING)
				{
					//pending 상태
					//Alertable Wait상태로 만들어야함
					SleepEx(INFINITE, TRUE);
					//::WSAWaitForMultipleEvents(1, &wsaEvent, TRUE, WSA_INFINITE, FALSE);					
				}
				else
				{
					//TODO: 진짜 문제있는 상황
					break;
				}
			}
			else
			{
				cout << "Data Recv Len = " << recvLen << endl;
			}
		}
		::closesocket(session.socket);
		//WSACloseEvent(wsaEvent);
	}
	//윈속 종료
	::WSACleanup();
}
