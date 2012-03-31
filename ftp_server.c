/* Title	:	AlokFTP - A minimal FTP server						*/
/* Source	:	ftp_server.c 								*/	
/* Date		:	09/03/2012	   							*/
/* Author	:	Alok Upadhyay	   							*/
/* Input	:	The incoming connections from clients
			- Port 2121 for incoming control messages and file transfer(inband)	*/
/* Output	:	The server prompts, the error/warning/help messages.			*/
/* Method	:	A well planned execution of Linux function/system calls			*/
/* Possible Further Improvements :	1. User account creation/authentication -not done, since it is tftp type only
					2. A good/innovative prompt.		-done
					3. An impressive banner! :P		-done
					4. Serve users across multiple sessions -done
					5. Serve multiple users at the same time -not done	
					6. Storing logs of the commands executed -not done		
					7. How to transfer using sendfile()	-done 		*/

/* Included header files */
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/sendfile.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>


/* Pre-processor directives */
#define DELIM " "
#define die(e) do { fprintf(stderr, e); exit(EXIT_FAILURE); } while (0);
#define CONTROL_PORT_NO 2121;
#define FILE_PORT_NO 2120;


/* Functions and subroutines declaration */
void setupServerPrimaries();
void serveClients();
void closeSocket();
void executeCommand(char *);
void transferFile(char *);
void writeToFile(char *, char *);

/* Global data-structures */
int sock, connected, bytes_recieved , fd, true = 1, control_port, file_bytes_recieved;
char send_data[20] , recv_data[1024], response[4096], buffer[8192], file_recv_data[4096];
struct sockaddr_in server_addr, client_addr, server_addr_file, client_addr2;
int sin_size, sin_size2;


int main()
{
	/*Defining the prompt*/
	char prompt[] = "alokFTP:~>";
	strcpy(send_data, prompt);
	
	control_port = CONTROL_PORT_NO;
	//file_port = FILE_PORT_NO;

	/* Function Calls for actual working of the server*/	
	setupServerPrimaries();
	serveClients();
}

void setupServerPrimaries()
{

	/*  Setting up the socket sock on port CONTROL_PORT_NO for control message transfer */
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) { // TCP connection
            perror("Socket");
            exit(1);
        }

	if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&true,sizeof(int)) == -1) {
            perror("Setsockopt");
            exit(1);
        }
        
        server_addr.sin_family = AF_INET;    //IPv4 Protocol     
        server_addr.sin_port = htons( control_port );   // Port number  
        server_addr.sin_addr.s_addr = INADDR_ANY; 
        bzero(&(server_addr.sin_zero),8); 

        if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) // bind a name 
                                                                       == -1) {
            perror("Unable to bind");
            exit(1);
        }

        if (listen(sock, 5) == -1) { // listening to a port #
            perror("Listen");
            exit(1);
        }
		
	
		
	printf("FTP Server waiting for client at %d (Control)\n", control_port);
}

void serveClients()
{
	printf("\nAlok's FTP Server Waiting for client on port CONTROL_PORT_NO.\n"); 
        fflush(stdout);
	
	while(1)
        {  

            sin_size = sizeof(struct sockaddr_in);
          

            connected = accept(sock, (struct sockaddr *)&client_addr,&sin_size);
	  
 	
            printf("\n I got a control connection from (%s , %d)",
                   inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
		
	  
	    char banner[] = "\n\n\t\t\t Welcome to Alok's FTP Server \n\nSupported commands: ls, dir, get, put, delete <filename>, mkdir <dirname>, rmdir <dirname>, cd <dirname>, bye.\n\n";
            send(connected, banner,strlen(banner), 0);


            while (1)
            {
	      strcpy(buffer, "");
		int i=0;
		while(i<8192)
		{
			buffer[i] = '\0';
			i++;
		}
	      strcat(buffer, response);
	      strcat(buffer, send_data);
	      strcpy(send_data, buffer);
	      
	      send(connected, send_data,strlen(send_data), 0);  
	      strcpy(response, "");
	      strcpy(buffer, "");	
	      strcpy(send_data, "alokFTP:~>");

	
              bytes_recieved = recv(connected, recv_data, 1024, 0);

              recv_data[bytes_recieved] = '\0';

              if (strcmp(recv_data , "bye") == 0 || strcmp(recv_data , "Q") == 0)
              {
                close(connected);
                break;
              }

              else 
              printf("\n%s" , recv_data);
	      executeCommand(recv_data);				
              fflush(stdout);
            }
        }       
	
	//closeSocket();
}

void executeCommand(char *command)
{
		char BYE[] = "bye";
		char GET[] = "get";
		char PUT[] = "put";
		char DIR[] = "dir";
		char LS[] = "ls";
		char DELETE[] = "delete";
		char MKDIR[] = "mkdir";
		char RMDIR[] = "rmdir";
		char CD[] = "cd";
		char HELP[] = "help";
		
		char * first_arg ,  * second_arg ;
		int i;

		
		printf("\n\n=======New Command Entered======\n");

		if(command == NULL)
		{
			printf("The client did not enter any valid command or went down unexpectedly going to waiting mode\n");
			serveClients();
		}	
	
		printf("\n Raw command=%s \n\n", command);
		first_arg = strtok(command, DELIM);
		second_arg = strtok(NULL, DELIM);
		
		if(first_arg == NULL)
		{
			printf("The client did not enter any valid command or went down unexpectedly going to waiting mode\n");
			serveClients();
		}

		printf("first %s, second %s\n", first_arg, second_arg);


		if(!strcmp(first_arg, GET))
		{
			printf("The user entered GET command\n");	
			strcpy(response, "You just entered GET command\n");	

			if(second_arg != NULL)			
			{
				transferFile(second_arg);	
				
				for(i=0 ; i<4095 ; i++)		
				{
				    
				    response[i] = '\0';
				}
		
				send(connected, response,strlen(response), 0); 

				for(i=0 ; i<4095 ; i++)		
				{
				    
				    response[i] = '\0';
				}
						

			}
			else
				strcpy(response, "Bad command for get : Usage >> get <dir_name>\n");
		}			
		
		else if(!strcmp(first_arg, PUT))
		{   
		    if(second_arg != NULL)
			{  
			printf("The user entered PUT command\n");
			file_bytes_recieved = recv(connected, file_recv_data, 4096, 0);
		        file_recv_data[file_bytes_recieved] = '\0';
			writeToFile(second_arg, file_recv_data); //second_arg=filename; file_recv_data=actual recieved data
			printf("recieved %d bytes\n", file_bytes_recieved);
			strcpy(response, "Data bytes recieved successfully at the server.\n");		
			}
		
		    else
			strcpy(response, "Bad command for put : Usage >> put <dir_name>\n");	 	
		}
		
		else if(!strcmp(first_arg, LS) || !strcmp(first_arg, DIR))
		{
			printf("The user entered LS command\n");
			int link[2];
			pid_t pid;
  			char output_of_ls[4096];

  			if (pipe(link)==-1)
    				die("pipe");

			if ((pid = fork()) == -1)
    				die("fork");

 			if(pid == 0) {

			    dup2 (link[1], STDOUT_FILENO);
			    close(link[0]);
				if(second_arg != NULL)				
				    execl("/bin/ls", "/bin/ls", "-r", second_arg, "-l", (char *) 0);
				else
					execl("/bin/ls", "/bin/ls", "-r", "-t", "-l", (char *) 0);			
				    

			} 
			else {

			    close(link[1]);
				int i;
			    for(i=0 ; i<4095 ; i++)		
				{
				    output_of_ls[i] = '\0';
				    response[i] = '\0';
				}
			    printf("The value of output_of_ls after flush: %s \n", output_of_ls);	
			    read(link[0], output_of_ls, sizeof(output_of_ls));
			    printf("Output: (%s)\n", output_of_ls);
			    strcpy(response, "");
			    strcpy(response, output_of_ls);
			    
			    wait(NULL);

			}

		}
		
		else if(!strcmp(first_arg, DELETE))
		{
			printf("The user entered DELETE command\n");
			int link[2];
			pid_t pid;
  			char output_of_delete[4];

  			if (pipe(link)==-1)
    				die("pipe");

			if ((pid = fork()) == -1)
    				die("fork");

 			if(pid == 0) {

			    dup2 (link[1], STDOUT_FILENO);
			    close(link[0]);
				if(second_arg != NULL)				
				    execl("/bin/rm", "/bin/rm", "-rf", "", second_arg, (char *) 0);
				else
					strcpy(response, "Bad command for delete : Usage >> delete <filename>\n");

			} else {

			    close(link[1]);	
			    read(link[0], output_of_delete, sizeof(output_of_delete));
			    printf("Output: (%s)\n", output_of_delete);
				strcpy(response, "");
			    strcpy(response, output_of_delete);
			    wait(NULL);

			}

		}

		else if(!strcmp(first_arg, MKDIR))
		{
			printf("The user entered MKDIR command\n");
			int link[2];
			pid_t pid;
  			char output_of_mkdir[4096];

  			if (pipe(link)==-1)
    				die("pipe");

			if ((pid = fork()) == -1)
    				die("fork");

 			if(pid == 0) {

			    dup2 (link[1], STDOUT_FILENO);
			    close(link[0]);
			    
				if(second_arg != NULL)				
				    execl("/bin/mkdir", "/bin/mkdir", "", "", second_arg, (char *) 0);
				else
					strcpy(response, "Bad command for mkdir : Usage >> mkdir <dir_name>\n");

			} else {

			    close(link[1]);	
			    read(link[0], output_of_mkdir, sizeof(output_of_mkdir));
			    printf("Output: (%s)\n", output_of_mkdir);
			    strcpy(response, "");
			    wait(NULL);

			}

		}
	
		else if(!strcmp(first_arg, RMDIR))
		{
			printf("The user entered RMDIR command\n");
			int link[2];
			pid_t pid;
  			char output_of_rmdir[4];

  			if (pipe(link)==-1)
    				die("pipe");

			if ((pid = fork()) == -1)
    				die("fork");

 			if(pid == 0) {

			    dup2 (link[1], STDOUT_FILENO);
			    close(link[0]);
			    
				if(second_arg != NULL)				
				   {
					 execl("/bin/rm", "/bin/rm", "-rf", "", second_arg, (char *) 0);
					 
				    }
				else
					strcpy(response, "Bad command for mkdir : Usage >> mkdir <dir_name>\n");
				
			} else {

			    close(link[1]);	
			    read(link[0], output_of_rmdir, sizeof(output_of_rmdir));
			    printf("Output: (%s)\n", output_of_rmdir);
			    strcpy(response, "");
			    wait(NULL);

			}

		}
		
		else if(!strcmp(first_arg, CD))
		{
			printf("The user entered CD command\n");
			int link[2];
			pid_t pid;
  			char output_of_cd[4096];

  			if (pipe(link)==-1)
    				die("pipe");

			if ((pid = fork()) == -1)
    				die("fork");

 			if(pid == 0) {

			    dup2 (link[1], STDOUT_FILENO);
			    close(link[0]);
			    
				if(second_arg != NULL)				
				   {
					int result =chdir(second_arg);
					if(result != -1)					
					{strcat(response, "PWD changed to ");
					strcat(response, second_arg);
					strcat(response, "\n");
					}
					else
					{
						perror("Couldn't execute\n");
						strcat(response, "Bad directory name\n");
					}
					 
				    }
				else
					strcpy(response, "Bad command for mkdir : Usage >> mkdir <dir_name>\n");
				
			} else {

			    close(link[1]);	
			    read(link[0], output_of_cd, sizeof(output_of_cd));
			    printf("Output: (%s)\n", output_of_cd);
			    strcpy(response, "");
			    wait(NULL);

			}

		}
		
		else if(!strcmp(first_arg, BYE))
		{		
			printf("The user entered BYE command\n");
			//free(first_arg); 
			//free(second_arg);
			printf("reaching here\n");
			//closeSocket();
			//close(connected);
			serveClients();
			//exit(0);
		}
		else if(!strcmp(first_arg, HELP))
		{
			printf("The user entered BYE command\n");
			strcpy(response, "Supported commands: ls, dir, get, put, delete <filename>, mkdir <dirname>, rmdir <dirname>, cd <dirname>, bye.\n");
		}

		else
		{
			printf("Invalid command entered\n");
			strcpy(response, "Bad command : type 'help' for more info.\n");
		}
}

void transferFile(char *filename)
{
	struct stat stat_buf;	/* argument to fstat */
	off_t offset = 0;          /* file offset */
	int rc;


	/* null terminate and strip any \r and \n from filename */
		//filename[rc] = '\0';
	 	//if (filename[strlen(filename)-1] == '\n')
		//filename[strlen(filename)-1] = '\0';
		//if (filename[strlen(filename)-1] == '\r')
		//filename[strlen(filename)-1] = '\0';



	/* open the file to be sent */
	    fd = open(filename, O_RDONLY);
	    if (fd == -1) {
	      fprintf(stderr, "unable to open '%s': %s\n", filename, strerror(errno));
	      exit(1);
	    }

	/* get the size of the file to be sent */
	    fstat(fd, &stat_buf);

    	/* copy file using sendfile */
	    offset = 0;
	    rc = sendfile (connected, fd, &offset, stat_buf.st_size);
	    if (rc == -1) {
	      fprintf(stderr, "error from sendfile: %s\n", strerror(errno));
	      exit(1);
	    }
	    if (rc != stat_buf.st_size) {
	      fprintf(stderr, "incomplete transfer from sendfile: %d of %d bytes\n",
	              rc,
	              (int)stat_buf.st_size);
	      exit(1);
	    }
	
    	/* close descriptor for file that was sent */
	    close(fd);
	
}

void writeToFile(char *filename, char *data1)
{
	FILE *fp;
	
	puts(data1);	

	fp = fopen(filename, "w");
	if (fp!=NULL)
 	 {
  		 fputs ( data1,fp);
  	 	 fclose (fp);
  	 }

}
	
void closeSocket()
{
	close(sock);
}
