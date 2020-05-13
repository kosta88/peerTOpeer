/*		NAME: sklonnie konstantin
 * 		ID: 310496781
 * 		linux h.w 4
 * 		Tel-Hai cs 2019
 */

#include "p2p.h"

file_ent_t All_files[P_BUFF_SIZE];
int curFilesCnt;
int clientPort;

/******************************************************/
void dirReq(int client_fd)
{
	msg_dirhdr_t msg_dirhdr;
	msg_dirent_t msg_dirent;
	msg_dirreq_t msg_dirreq;
	int i;

	printf("Server - dirreq: receiving MSG_DIRREQ\n");

	if(recv(client_fd,(char*)&msg_dirreq,sizeof(msg_dirreq),0) == -1)
	{
		perror("Server -cannot do recv() in MSG_NOTIFY");
		close(client_fd);
		exit(EXIT_FAILURE);
	}
	msg_dirhdr.m_type = MSG_DIRHDR;
	msg_dirhdr.m_count = curFilesCnt;

	printf("Server - notify: sending MSG_NOTIFY with count = %d\n",curFilesCnt);

	if(send(client_fd,(char*)&msg_dirhdr, sizeof(msg_dirhdr),0) == -1 )
	{
		perror("Server - send()");
		close(client_fd);
		exit(EXIT_FAILURE);
	}


	msg_dirent.m_type = MSG_DIRENT;

	for(i = 0 ; i < curFilesCnt ; ++i)
	{
		msg_dirent.m_port = All_files[i].fe_port;
		msg_dirent.m_addr = All_files[i].fe_addr;
		strcpy(msg_dirent.m_name,All_files[i].fe_name);

		if(send(client_fd,(char*)&msg_dirent, sizeof(msg_dirent),0) == -1 )
		{
			perror("Server -cannot perform send()");
			close(client_fd);
			exit(EXIT_FAILURE);
		}
	}
}
/******************************************************/
void notify(int client_fd)
{
	msg_notify_t    msg_notify;
	msg_ack_t       msg_ack;

	if(recv(client_fd, &msg_notify, sizeof(msg_notify), 0) == -1 )
	{
		printf("Server - error receiving MSG_NOTIFY\n");
		close(client_fd);
		return;
	}

	msg_ack.m_type = MSG_ACK;

	if(msg_notify.m_port == 0)
	{
		printf("Server - notify: assigned port %d\n", clientPort);	  			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
		msg_ack.m_port = (in_port_t)clientPort++;
	}
	else
		msg_ack.m_port = msg_notify.m_port;


	strcpy( All_files[curFilesCnt].fe_name, msg_notify.m_name);
	All_files[curFilesCnt].fe_addr = msg_notify.m_addr;
	All_files[curFilesCnt].fe_port = msg_ack.m_port;
	curFilesCnt++;


	if(send(client_fd,(char*)&msg_ack, sizeof(msg_ack),0) == -1 )
	{
		perror("Server -failed doing send()");
		close(client_fd);
		exit(EXIT_FAILURE);
	}
	else
		printf("Server - notify: sending MSG_ACK\n");    				/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

}
/******************************************************/
int main(int argc, char* argv[])
{
	struct sockaddr_in server;
	int socket_fd, clientSocket_fd, msg;
	int shutDown = false;
	int clientDone = false;

	clientPort = P_SRV_PORT+1;

	curFilesCnt=0;

	/*>>>>>>>>>> FILL UP SERVERS ADRESS */
	server.sin_family = AF_INET;
	server.sin_port = htons(P_SRV_PORT);
	server.sin_addr.s_addr = INADDR_ANY;

	/*>>>>>>>>>>  OPEN SOCKET*/
	if ((socket_fd = socket(AF_INET, SOCK_STREAM, 0))== -1)
	{
		perror( "Socket failure, cannot open one in client main");
		exit(EXIT_FAILURE);
	}
	inet_aton("127.0.0.1", &server.sin_addr);
	printf("Server - server: opening socket on 127.0.0.1:%d\n",P_SRV_PORT);    			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/

	/*>>>>>>>>>>  PUBLISH THE SOCKET */
	if((bind(socket_fd,(struct sockaddr*)&server,sizeof(server))) == -1)
	{
		perror("Server - cannot bind()");
		close(socket_fd);
		exit(EXIT_FAILURE);			}
	else
		printf("Socket successfully binded to address\n");    			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/


	/*>>>>>>>>>>   LISTEN TO THE SOCKET*/
	if ((listen(socket_fd, BACKLOG))== -1)
	{
		perror("Listening failed...\n");
		exit(EXIT_FAILURE);			}
	else
		printf("Server listening..\n");    			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/


	while(!shutDown)
	{
		if ((clientSocket_fd = accept(socket_fd, NULL, NULL)) == -1)
		{
			perror("Server - accept()");
			close(socket_fd);
			exit(EXIT_FAILURE);		}

		clientDone= false;
		while (!clientDone)
		{
			/*>>>>>>>>>> PEEK TO SEE WHAT TYPE OF MSG IS COMING */
			if (recv(clientSocket_fd, &msg, sizeof(int), MSG_PEEK) <= 0)
				continue;

			switch (msg)
			{
			case MSG_SHUTDOWN:
				printf("Server - notify: receiving MSG_SHUTDOWN\n");    			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
				shutDown= true;
				clientDone=true;
				close(clientSocket_fd);
				close(socket_fd);
				printf("Server - notify: Shutting Down\n");    						/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
				break;

			case MSG_NOTIFY:
				printf("Server - notify: receiving MSG_NOTIFY\n");    			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
				notify(clientSocket_fd);
				break;

				case MSG_DIRREQ:
					dirReq(clientSocket_fd);
					break;

				case MSG_DONE:
					printf("Server - receiving MSG_DONE from client.... \n");    			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
					clientDone = true;
					break;

				default:
					break;
			}
		}
		printf("Server : client finished, waiting on the next client..\n");    			/*	<<<<<<<<<<<<<	PRINT	<<<<<<<<	*/
	}

	close(socket_fd);

	return 0;
}
