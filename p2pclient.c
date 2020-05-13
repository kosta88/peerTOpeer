/*		NAME: sklonnie konstantin
 * 		ID: 310496781
 * 		linux h.w 4
 * 		Tel-Hai cs 2019
 */
#include "p2p.h"
#include <sys/stat.h>
#include <fcntl.h>

typedef enum {seed,leech,shutdownAll} clientType;
int closeClient;

/******************************************************/
void sendFile(int leech_fd)
{
	msg_filereq_t   msg_filereq;
	msg_filesrv_t   msg_filesrv;
	ssize_t size = 0;
	int timesOfBuffer,fd, i;
	char buff[P_BUFF_SIZE];
	struct stat fileStat;

	printf("Peer - start_server : starting peer server\n");
	if(recv(leech_fd,(char*)&msg_filereq, sizeof(msg_filereq),0) == -1)
	{
		perror("Peer - recv()");
		close(leech_fd);
		exit(EXIT_FAILURE);
	}

	printf("Peer - start_server : received MSG_FILEREQ for file : < %s >\n", msg_filereq.m_name);
	printf("Peer - start_server : listening on socket\n");

	if(((fd = open( msg_filereq.m_name , O_RDONLY )) == -1))
	{
		perror("Peer - cannot open() the file in sendFile");
		close(leech_fd);
		exit(EXIT_FAILURE);
	}
	if(fstat(fd, &fileStat) == -1)
	{
		perror("Peer - failed to get fstat()");
		close(leech_fd);
		close(fd);
		exit(EXIT_FAILURE);
	}

	msg_filesrv.m_file_size = (int)fileStat.st_size;
	msg_filesrv.m_type = MSG_FILESRV;

	printf("Peer - sending_file: sending MSG_FILESRV with file '%s', size : %d\n",msg_filereq.m_name, msg_filesrv.m_file_size );
	if((send(leech_fd,(char*)&msg_filesrv, sizeof(msg_filesrv),0)) == -1 )
	{
		perror("Peer - send()");
		close(fd);
		exit(EXIT_FAILURE);
	}


	printf("Peer - sending_file: sending content of file\n");
	timesOfBuffer = (msg_filesrv.m_file_size / P_BUFF_SIZE) + 1;

	for ( i = 0 ; i < timesOfBuffer ; ++i )
	{
		size = read(fd,buff,P_BUFF_SIZE);
		if(size == -1)
		{
			perror("Peer -failed on read()");
			close(leech_fd);
			exit(EXIT_FAILURE);
		}
		printf("Peer - sending_file: sending a full buffer to client\n");
		if(( send(leech_fd,&buff, sizeof(buff),0))== -1 )
		{
			perror("Peer - send()");
			close(leech_fd);
			exit(EXIT_FAILURE);
		}
	}

	printf("Peer- sending_file: finished sending file\n");
	close(fd);

}
/******************************************************/
int peerPortExists(struct sockaddr_in peers[],int size, msg_dirent_t msg_dirent)
{
	int i ;

	for (i = 0; i < size; i++)
		if(peers[i].sin_port == msg_dirent.m_port)
			return true;
	return false;
}
/******************************************************/
void shutDownClients(int socket_fd)
{
	int i, tmp;
	int idx = 0;
	msg_dirreq_t msg_dirreq;
	msg_dirhdr_t msg_dirhdr;
	msg_dirent_t msg_dirent;
	msg_shutdown_t msg_shutdown;
	struct sockaddr_in peers[P_NAME_LEN ], tmp_addr;

	closeClient= true;

	printf("Client - shut_down: receiving MSG_SHUTDOWN\n");			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
	printf("Client - shut_down: Shutting Down\n");					/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

	msg_shutdown.m_type = MSG_SHUTDOWN;
	msg_dirreq.m_type = MSG_DIRREQ;


	printf("Client - shut_down: sending MSG_DIRREQ\n");				/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
	if(send(socket_fd,(char*)&msg_dirreq, sizeof(msg_dirreq),0) == -1 )
	{
		perror("Client - send() on shutdown");
		exit(EXIT_FAILURE);				}

	if(recv(socket_fd,(char*)&msg_dirhdr, sizeof(msg_dirhdr),0) == -1)
	{
		perror("Client - recv() on shutdown");
		exit(EXIT_FAILURE);			}

	printf("Client - shut_down: received MSG_DIRHDR with %d items\n",msg_dirhdr.m_count);		/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

	/*collect all peers*/
	for (i = 0; i < msg_dirhdr.m_count ; i++)
	{
		if (recv(socket_fd, (char *) &msg_dirent, sizeof(msg_dirent), 0) == -1)
		{
			perror("Client - recv() on shutdown");
			exit(EXIT_FAILURE);
		}
		printf("Client - shut_down: received MSG_DIRENT for '%s' @ 127.0.0.1:%d\n",msg_dirent.m_name, msg_dirent.m_port);

		if(!peerPortExists(peers,idx,msg_dirent))
		{
			peers[idx].sin_family = AF_INET;
			peers[idx].sin_addr.s_addr = msg_dirent.m_addr;
			peers[idx++].sin_port = msg_dirent.m_port;
		}
	}
	tmp_addr.sin_family = AF_INET;
	inet_aton("127.0.0.1",&tmp_addr.sin_addr);

	for (i = 0; i < idx; i++)
	{
		if (  (tmp = socket(AF_INET, SOCK_STREAM, 0)  ) == -1 )
		{
			perror("Client -cannot open socket() on shutdown");
			exit(EXIT_FAILURE);
		}

		tmp_addr.sin_port = htons(peers[i].sin_port);

		if (connect(tmp, (struct sockaddr *) &tmp_addr, sizeof(tmp_addr)) != 0)
		{
			perror("Client - connect() - in  shutDownClient()");
			close(tmp);
			close(socket_fd);
			exit(EXIT_FAILURE);
		}

		printf("Client - shut_down: sending MSG_SHUTDOWN to peer at 127.0.0.1: %d\n",peers[i].sin_port);
		if (send(tmp, (char *) &msg_shutdown, sizeof(msg_shutdown), 0) < 0)
		{
			perror("Client - failed to send() shutdown ");
			exit(EXIT_FAILURE);
		}
		close(tmp);
	}

	printf("Client - shut_down: sending MSG_SHUTDOWN to server at 127.0.0.1: %d\n",P_SRV_PORT);

	if(send(socket_fd,(char*)&msg_shutdown, sizeof(msg_shutdown),0)== -1 )
	{
		perror("Client - failed to send() shutdown to server");
		exit(EXIT_FAILURE);
	}
	close(socket_fd);
	printf("Server - Exit Successful\n");
	exit(EXIT_SUCCESS);
}
/******************************************************/
int getFile(int server_fd, msg_dirent_t* msg_dirent, in_port_t port)
{
	struct sockaddr_in  clientAddress;
	msg_filereq_t msg_filereq;
	msg_filesrv_t msg_filesrv;
	msg_done_t msg_done;
	int peerSock_fd, fd,i;
	int timesOfBuffer, size;
	char buff[P_BUFF_SIZE];

	/*	SET ADRESS*/
	clientAddress.sin_family = AF_INET;
	clientAddress.sin_port = htons(msg_dirent->m_port);
	clientAddress.sin_addr.s_addr = INADDR_ANY;
	inet_aton("127.0.0.1", &clientAddress.sin_addr);

	printf("Client - get_file_from_client: getting file '%s' from client on port %d:\n",msg_dirent->m_name,msg_dirent->m_port );/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

	if( (peerSock_fd = socket(AF_INET,SOCK_STREAM,0)) == -1 )
	{
		perror("Client - cannot open socket() in getFile");
		exit(EXIT_FAILURE);			}


	/*CONNECT TO ADRESS */
	if(  connect(peerSock_fd ,(struct sockaddr*) &clientAddress, sizeof(clientAddress)  ) == -1)
	{
		perror("Client - failed to connect() in getFile FUNCT.");
		close(peerSock_fd);
		exit(EXIT_FAILURE);			}

	printf("Client - get_file_from_client: established connection\n");	  			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

	/* CREATE AND SEND A MESSAGE REQUEST*/
	msg_filereq.m_type = MSG_FILEREQ;
	strcpy(msg_filereq.m_name,msg_dirent->m_name);

	if( send(peerSock_fd,(char*)&msg_filereq, sizeof(msg_filereq),0) == -1 )
	{
		perror("Client -cannot send() to other client  in getFile");
		exit(EXIT_FAILURE);			}

	printf("Client - get_file_from_client: sent MSG_FILEREQ\n");	  			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

	/* RECIEVING AN AWNSER	*/
	if(recv(peerSock_fd,(char*)&msg_filesrv, sizeof(msg_filesrv),0) == -1)
	{
		perror("Client - failed to recv()");
		exit(EXIT_FAILURE);			}

	printf("Client - get_file_from_client: received MSG_FILESRV: file length:%d\n",msg_filesrv.m_file_size);/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

	/*OPEN THE FILE*/
	if((fd = open(msg_dirent->m_name, O_WRONLY | O_CREAT, 0666)) == -1)
	{
		perror("Client - cannot open() a file ...");
		exit(EXIT_FAILURE);			}

	printf("Client - get_file_from_client: opened file <%s>\n", msg_dirent->m_name);		/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

	if(msg_filesrv.m_file_size % P_BUFF_SIZE)
		timesOfBuffer = (msg_filesrv.m_file_size / P_BUFF_SIZE) + 1;
	else
		timesOfBuffer = (msg_filesrv.m_file_size / P_BUFF_SIZE);

	for ( i = 0 ; i < timesOfBuffer ; ++i )
	{
		size=recv(peerSock_fd, &buff, sizeof(buff)-1,0);
		if(size < 0)
		{
			perror("Client - failed on recv() from peer in getFile");
			exit(EXIT_FAILURE);
		}
		buff[size]= '\0';

		if(write(fd,buff,(size_t)size) == -1)
		{
			perror("Client - failed to write() on file in getFile");
			exit(EXIT_FAILURE);
		}
	}
	printf("Client - get_file_from_client: obtained file '%s' from client on port:%d\n",msg_dirent->m_name, msg_dirent->m_port );/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

	close(fd);

	msg_done.m_type = MSG_DONE;

	if(send(peerSock_fd,(char*)&msg_done, sizeof(msg_done),0) == -1 )
	{
		perror("Client - failed to send() in getFile");
		exit(EXIT_FAILURE);			}

	close(peerSock_fd);

	return true;
}
/******************************************************/
int haveReqestedFile(char* file2find,int count, char* arr[])
{
	int i;

	for (i = 0; i < count; i++ )
		if( arr[i] && strcmp (file2find ,arr[i] ) == 0 )
			return true;

	return false;
}
/******************************************************/
in_port_t leechFunct(int server_fd, char* arr[], int count)
{
	int i;
	msg_dirhdr_t msg_dirhdr;
	msg_dirent_t msg_dirent;
	msg_dirreq_t msg_dirreq;
	in_port_t port = 0;

	msg_dirreq.m_type = MSG_DIRREQ;
	printf("Client - get_list: sending MSG_DIRREQ\n");	  			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
	if( send(server_fd,(char*)&msg_dirreq, sizeof(msg_dirreq),0)== -1 )
	{
		perror("Client - failed to send() in leechFunct");
		exit(EXIT_FAILURE);				}

	if(recv(server_fd,(char*)&msg_dirhdr, sizeof(msg_dirhdr),0) == -1)
	{
		perror("Client - failed to recv() in leechFunct");
		exit(EXIT_FAILURE);				}

	printf("Client - get_list: received MSG_DIRHDR with %d items\n",msg_dirhdr.m_count);	  			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

	for ( i = 0; i < msg_dirhdr.m_count ; i++ )
	{
		if( recv(server_fd,(char*)&msg_dirent, sizeof(msg_dirent),0) == -1 )
		{
			perror("Client - failed on recv() in leechFunct");
			exit(EXIT_FAILURE);			}

		printf("Client - get_list: received MSG_DIRENT for <%s> @ 127.0.0.1:%d\n",msg_dirent.m_name,msg_dirent.m_port);/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

		if( haveReqestedFile(msg_dirent.m_name ,count ,arr) )
			if( getFile(server_fd, &msg_dirent, port) )
				arr[i] = NULL;
	}
	printf("Client - finished leeching\n");	  			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
	return port;
}

/******************************************************/

in_port_t seedFunct(int server_fd, struct sockaddr_in* serverAddress, char* arr[], int count)
{
	msg_notify_t msg_notify;
	msg_ack_t msg_ack;
	int i ;

	msg_notify.m_addr = serverAddress->sin_addr.s_addr;
	msg_notify.m_port = 0;
	msg_notify.m_type = MSG_NOTIFY;

	for (i = 0; i < count ; ++i)
	{
		strcpy(msg_notify.m_name, arr[i]);
		printf("Client - share: sending MSG_NOTIFY for '%s'",msg_notify.m_name);	  			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
		printf(" @ 127.0.0.1:%d\n",msg_notify.m_port);	  										/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

		if( send(server_fd,(char*)&msg_notify, sizeof(msg_notify),0) == -1 )
		{
			perror("Client - failed to send() in seedFunct");
			exit(EXIT_FAILURE);			}

		printf("Client - share: receiving MSG_ACK\n");
		if( recv(server_fd,(char*)&msg_ack, sizeof(msg_ack),0) == -1 )
		{
			perror("Client - failed to recv() in seedFunct");
			exit(EXIT_FAILURE);			}

		if(msg_notify.m_port == 0)
		{
			msg_notify.m_port = msg_ack.m_port;
			printf("Client - share: set port to : %d\n", msg_notify.m_port);	  			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
		}
	}
	return msg_ack.m_port;
}
/******************************************************/

clientType getType(char* cmd)
{
	if(strcmp(cmd,"shutdown") == 0)
		return shutdownAll;

	if(strcmp(cmd,"seed") == 0)
		return seed;

	if(strcmp(cmd,"leech") == 0)
		return leech;


	printf("Error - no such command %s\n",cmd);
	exit(EXIT_FAILURE);

}
/******************************************************/
void becomeServer(	struct sockaddr_in serverAddress, in_port_t port)
{
	int  msg, clientAsServerSock, leech_fd, done = false;

	serverAddress.sin_port = htons(port);

	if((clientAsServerSock = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("Client - failed to open socket in becomeServer");
		exit(EXIT_FAILURE);			}


	if((bind(clientAsServerSock,(struct sockaddr*)&serverAddress,sizeof(serverAddress))) == -1)
	{
		perror("Server - failed to bind() in becomeServer");
		close(clientAsServerSock);
		exit(EXIT_FAILURE);			}

	if ((listen(clientAsServerSock, BACKLOG)) == -1)
	{
		printf("client as Server -failed to listen() in becomeServer");
		close(clientAsServerSock);
		exit(EXIT_FAILURE);			}


	printf("Client : FROM NOW ON IS A SERVER\n");
	while(!closeClient)
	{
		if ((leech_fd = accept(clientAsServerSock, NULL, NULL)) == -1)
		{
			perror("Client - failed on accept() in becomeServer");
			close(clientAsServerSock);
			exit(EXIT_FAILURE);		}

		done = false;
		while (!done)
		{
			if (recv(leech_fd, &msg, sizeof(int), MSG_PEEK) <= 0)
				continue;

			switch (msg)
			{
			case MSG_SHUTDOWN:
				closeClient = true;
				done = true;
				close(leech_fd);
				close(clientAsServerSock);
				break;

			case MSG_FILEREQ:
				sendFile(leech_fd);
				break;

			case MSG_DONE:
				done = true;
				break;

			default:
				break;
			}
		}
		printf("Peer - listening ...\n");
	}
	printf("Peer : EXIT SUCCESSFULL\n");
}
/******************************************************/
int main(int argc, char* argv[])
{
	struct sockaddr_in serverAddress;
	in_port_t port = 0;
	clientType type;
	msg_done_t finished;
	int serverSocket;
	closeClient = false;

	if(argc < 2)
	{
		printf("Not enough arguments\n");
		exit(EXIT_FAILURE);			}

	type = getType(argv[1]);

	/*>>>>>>>>>>  OPEN SOCKET*/
	if((serverSocket = socket(AF_INET,SOCK_STREAM,0)) == -1)
	{
		perror("Client - cannot open socket() in main");
		exit(EXIT_FAILURE);			}

	serverAddress.sin_family = AF_INET;
	serverAddress.sin_port = htons(P_SRV_PORT);
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	inet_aton("127.0.0.1",&serverAddress.sin_addr);

	if(  connect(serverSocket,(struct sockaddr*) &serverAddress, sizeof(serverAddress)  ) == -1 )
	{
		perror("Client - cannot connect() to server in main");
		close(serverSocket);
		exit(EXIT_FAILURE);			}

	switch(type)	{

	case seed:
		if(argc < 3)
		{
			perror("Not enough arguments");
			exit(EXIT_FAILURE);			}
		port = seedFunct(serverSocket, &serverAddress, argv + 2, argc - 2);
		break;

	case leech:
		if(argc < 3)
		{
			perror("Not enough arguments");
			exit(EXIT_FAILURE);			}
		port = leechFunct(serverSocket,argv + 2, argc - 2);
		break;

	case shutdownAll:
		shutDownClients(serverSocket);
		break;

	default:
		break;
	}
	printf("Client - sending the server MSG_DONE\n");	  			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
	finished.m_type = MSG_DONE;

	if(send(serverSocket,(char*)&finished, sizeof(finished),0) == -1 )
	{
		perror("Client - failed to do send() in main");
		exit(EXIT_FAILURE);			}

	close(serverSocket);

	if(type == 0)
		becomeServer(serverAddress, port);


	return 0;
}
