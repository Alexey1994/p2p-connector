#ifndef WINAPI_WS2_32_INCLUDED
#define WINAPI_WS2_32_INCLUDED


#include <types.c>


//=============================== WSA ================================


typedef struct {
	Number16 version;
	Number16 high_version;
	Byte     description[257];
	Byte     system_status[129];
	Number32 maximum_sockets;
	Number32 maximum_UDP_size;
	Byte*    vendor_info;
}
Windows_Sockets;

import stdcall Number32 WSAStartup(Number16 version, Windows_Sockets* sockets);
import stdcall Number32 WSACleanup();
import stdcall Number32 WSAGetLastError();


//=============================== Address Info =======================


typedef struct {
	Byte  family;
	Byte  zero;
	Number16 port;

	union {
		Number32 ip_v4;
		Number32 ip_v6_flow_info;
	};

	Number8 ip_v6[16];

	union {
		Number32 ip_v6_scope_id;

		union {
			struct {
				Number32 zone: 28;
				Number32 level: 4;
			};

			Number32 value;
		};
	};
}
Windows_Address;


typedef struct {
	Number32             flags;
	Number32             family;
	Number32             socket_type;
	Number32             protocol;
	Number               address_length;
	Byte*                name;
	Windows_Address*     address;
	struct Address_Info* next;
}
Address_Info;

import stdcall Address_Info* getaddrinfo  (Byte* host, Byte* port, Address_Info* hints, Address_Info** result);
import stdcall void          freeaddrinfo (Address_Info* address_info);


//=============================== Socket =============================

//FIONBIO
#define WS_NON_BLOCKING_MODE 0x8004667E

#define WS_RESOURCE_BLOCKED 10035
#define WS_SOCKET_ALREADY   10037
#define WS_SOCKET_CONNECTED 10056

#define WS_INVALID_SOCKET   10038


#define SOL_SOCKET 0xFFFF


#define SO_ERROR 0x1007


typedef Signed_Number32 Socket;

import stdcall Socket          socket      (Number32 family, Number32 type, Number32 protocol);
import stdcall Signed_Number32 closesocket (Socket socket);
import stdcall Signed_Number32 connect     (Socket socket, Windows_Address* address, Number32 address_size);
import stdcall Signed_Number32 bind        (Socket socket, Windows_Address* address, Number32 address_size);
import stdcall Signed_Number32 send        (Socket socket, Byte* buffer, Number32 buffer_length, Number32 flags);
import stdcall Signed_Number32 sendto      (Socket socket, Byte* buffer, Number32 buffer_length, Number32 flags, Windows_Address* destination_address, Number32 destination_address_size);
import stdcall Signed_Number32 recv        (Socket socket, Byte* buffer, Number32 buffer_length, Number32 flags);
import stdcall Signed_Number32 recvfrom    (Socket socket, Byte* buffer, Number32 buffer_length, Number32 flags, Windows_Address* destination_address, Number32* destination_address_size);
import stdcall Socket          accept      (Socket socket, Windows_Address* address, Number32* address_size);
import stdcall Signed_Number32 listen      (Socket socket, Number32 maximum_pending_connections);
import stdcall Signed_Number32 ioctlsocket (Socket socket, Number32 command, Number32* arguments);
import stdcall Signed_Number32 getsockopt  (Socket socket, Signed_Number32 level, Signed_Number32 option_name, Byte* option, Signed_Number32* option_size);


#endif //WINAPI_WS2_32_INCLUDED