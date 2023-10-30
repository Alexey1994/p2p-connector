#include <network/p2p-connect2.c>
#include <network/packet-connect3.c>
#include <thread.c>


#define QUEUE_SIZE 32


Packet_Connection address_connection;

Address proxy_address;
Address client_address;

Byte*   connect_to;

Number  unusage_time;


Boolean send_address_using_stdout(P2P_Connection* connection)
{
	print("%d.%d.%d.%d:%d\n",
		connection->mapped_address.ip_v4 & 0xFF,
		(connection->mapped_address.ip_v4 >> 8) & 0xFF,
		(connection->mapped_address.ip_v4 >> 16) & 0xFF,
		(connection->mapped_address.ip_v4 >> 24) & 0xFF,
		((connection->mapped_address.port >> 8) & 0xFF) | ((connection->mapped_address.port << 8) & 0xFF00)
	);
	
	return 1;
}

Boolean receive_address_using_arguments(P2P_Connection* connection)
{
	initialize_address_by_url(&connection->udp_connection.destination_address, connect_to);
	
	return 1;
}


Boolean send_address_using_address_connection(P2P_Connection* connection)
{
	Number time;
	
	print_error("sending ");
	print_address(&connection->mapped_address);
	print_error("\n");
	
	for(time = 0; time < 5000; ++time) {
		if(write_bytes_in_packet_connection(&address_connection, &connection->mapped_address, sizeof(connection->mapped_address))) {
			return 1;
		}
	
		sleep_thread(1);
	}
	
	return 0;
}

Boolean receive_address_using_address_connection(P2P_Connection* connection)
{
	copy_bytes(&connection->udp_connection.destination_address, &client_address, sizeof(Address));
	
	return 1;
}


typedef struct {
	struct Client_Connection* next;
	
	TCP_Connection    tcp_connection;
	P2P_Connection    p2p_connection;
	Packet_Connection packet_connection;
	Number            unusage_time;
	Number            receive_time;
	Number            send_time;
	Signed_Number     receive_buffer_size;
	Signed_Number     send_buffer_size;
	Byte              receive_buffer[QUEUE_SIZE * (MAX_PACKET_SIZE - sizeof(Packet_Head))];
	Byte              send_buffer[QUEUE_SIZE * (MAX_PACKET_SIZE - sizeof(Packet_Head))];
}
Client_Connection;


Client_Connection* client_connections = 0;


void connect_client()
{
	Client_Connection* client_connection;
	
	client_connection = allocate_memory(sizeof(Client_Connection));
	
	client_connection->unusage_time = 0;
	client_connection->receive_buffer_size = 0;
	client_connection->send_buffer_size = 0;
	
	copy_bytes(&client_connection->tcp_connection.destination_address, &proxy_address, sizeof(Address));
	
	if(!initialize_TCP_connection(&client_connection->tcp_connection)) {
		goto error;
	}
	
	client_connection->next = 0;
	
	Client_Connection** connection_pointer;
	for(connection_pointer = &client_connections; *connection_pointer; connection_pointer = &(*connection_pointer)->next) {
		
	}
	
	*connection_pointer = client_connection;
	
	
	if(
		!initialize_P2P_connection(
			&client_connection->p2p_connection,
			"stun.l.google.com", 19302,
			&send_address_using_address_connection,
			&receive_address_using_address_connection
		)
	) {
		goto error_and_close_tcp;
	}
	
	initialize_packet_connection(&client_connection->packet_connection, QUEUE_SIZE, &client_connection->p2p_connection, &read_bytes_from_P2P_connection, &write_bytes_in_P2P_connection);
	
	return;
	
	error_and_close_tcp: {
		*connection_pointer = (*connection_pointer)->next;
		deinitialize_TCP_connection(&client_connection->tcp_connection);
	}
	
	error: {
		free_memory(client_connection);
		print_error("client not connected\n");
	}
}


void close_client(Client_Connection* connection)
{
	deinitialize_TCP_connection(&connection->tcp_connection);
	free_memory(connection);
}


Number main(Number number_of_arguments, Byte** arguments)
{
	connect_to = arguments[1];
	
	if(!initialize_address(&proxy_address, "127.0.0.1", 80, TCP_PROTOCOL)) {
		print_error("proxy адрес не доступен\n");
		goto error;
	}
	
	
	P2P_Connection p2p_connection;
	
	if(
		!initialize_P2P_connection(
			&p2p_connection,
			"stun.l.google.com", 19302,
			&send_address_using_stdout,
			&receive_address_using_arguments
		)
	) {
		goto error;
	}
	
	initialize_packet_connection(&address_connection, 1, &p2p_connection, &read_bytes_from_P2P_connection, &write_bytes_in_P2P_connection);
	
	
	Signed_Number bytes_readed;
	Signed_Number bytes_writed;
	
	for(unusage_time = 0; unusage_time < 300000; ++unusage_time) {
		bytes_readed = read_bytes_from_packet_connection(&address_connection, &client_address, sizeof(client_address));
		
		if(bytes_readed) {
			print_error("connecting to ");
			print_address(&client_address);
			print_error("\n");
			
			connect_client();
		}
		
		
		Client_Connection** client_connection_pointer;
		Client_Connection*  client_connection;
		
		for(client_connection_pointer = &client_connections; *client_connection_pointer; client_connection_pointer = &(*client_connection_pointer)->next) {
			client_connection = *client_connection_pointer;
			++client_connection->unusage_time;
			
			if(client_connection->unusage_time >= 300000) {
				print_error("client connection closed: unusage timeout\n");
				*client_connection_pointer = (*client_connection_pointer)->next;
				close_client(client_connection);
				break;
			}
			
			
			if(client_connection->receive_buffer_size > 0) {
				client_connection->unusage_time = 0;
				unusage_time = 0;
				bytes_writed = write_bytes_in_TCP_connection(&client_connection->tcp_connection, client_connection->receive_buffer, client_connection->receive_buffer_size);
				
				if(bytes_writed > 0) {
					client_connection->receive_buffer_size -= bytes_writed;
				}
				else if(bytes_writed < 0) {
					print_error("client connection closed: TCP error2\n");
					*client_connection_pointer = (*client_connection_pointer)->next;
					close_client(client_connection);
					break;
				}
				else {
					if(client_connection->receive_time >= 5000) {
						print_error("client connection closed: write in TCP timeout\n");
						*client_connection_pointer = (*client_connection_pointer)->next;
						close_client(client_connection);
						break;
					}
					
					++client_connection->receive_time;
				}
			}
			else if(client_connection->receive_buffer_size < 0) {
				
			}
			else {
				client_connection->receive_time = 0;
				client_connection->receive_buffer_size = read_bytes_from_packet_connection(&client_connection->packet_connection, client_connection->receive_buffer, sizeof(client_connection->receive_buffer));
			}
			
			
			if(client_connection->send_buffer_size > 0) {
				client_connection->unusage_time = 0;
				unusage_time = 0;
				bytes_writed = write_bytes_in_packet_connection(&client_connection->packet_connection, client_connection->send_buffer, client_connection->send_buffer_size);
			
				if(bytes_writed > 0) {
					client_connection->send_buffer_size -= bytes_writed;
				}
				else {
					if(client_connection->send_time >= 5000) {
						print_error("client connection closed: write packets timeout\n");
						*client_connection_pointer = (*client_connection_pointer)->next;
						close_client(client_connection);
						break;
					}
					
					++client_connection->send_time;
				}
			}
			else if(client_connection->send_buffer_size < 0) {
				print_error("client connection closed: TCP error\n");
				*client_connection_pointer = (*client_connection_pointer)->next;
				close_client(client_connection);
				break;
			}
			else {
				client_connection->send_time = 0;
				client_connection->send_buffer_size = read_bytes_from_TCP_connection(&client_connection->tcp_connection, client_connection->send_buffer, sizeof(client_connection->send_buffer));
			}
		}
		
		
		sleep_thread(1);
	}
	
	
	deinitialize_packet_connection(&address_connection);
	deinitialize_P2P_connection(&p2p_connection);
	
	return 0;

	error: {
		return 1;
	}
}