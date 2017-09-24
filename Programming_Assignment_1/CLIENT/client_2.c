#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>

#define TIMEOUT_SEC 3   //Macro to define receiver timeout
#define MAX_LENGTH 512 //Macro to define length of packets


/*function to print error message*/
void error(char *msg)
{
    perror(msg);
    exit(1);
}


/*argc = count of number of arguments*/
/*argv = actual arguments*/
int main(int argc, char *argv[]) 
{

  /*initializing variables*/
  int sock, length, n;
	struct sockaddr_in server, from;
	struct hostent *hp;
	char buffer[MAX_LENGTH];
	char buffer_1[MAX_LENGTH+1];
	char server_recd_msg[MAX_LENGTH];
	char packet[MAX_LENGTH]= "";
	char send_ack[10] = "";
	char recd_ack[10] = ""; 
	char packet_number[10] = "";

	/*to check if two arguments are passed*/ 
	if(argc != 3)
	{
		printf("Usage: server port\n");
		exit(1);
	}
  
	/*Creating a socket 
	AF_INET = refers to addresses from the internet
	SOCK_DGRAM = UDP*/
	sock = socket(AF_INET, SOCK_DGRAM, 0);

	if(sock < 0)
	{
		error("ERROR: socket");
	}
  
  /*contains code for address family, equal to AF_INET*/
	server.sin_family = AF_INET;
	
	/*get the host name*/
	hp = gethostbyname(argv[1]);
	if(hp == 0)
	{
		error("ERROR:Unknown host");
	}

	/*sets the fields in serv_addr. bcopy copies n bytes from string 1 to string 2*/
	bcopy((char*)hp->h_addr, (char *)&server.sin_addr, hp->h_length);
	server.sin_port = htons(atoi(argv[2]));
	length = sizeof(struct sockaddr_in);

	/*adding timeout for recvfrom*/
	struct timeval timeout_interval = {TIMEOUT_SEC,0};
  setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout_interval,sizeof(struct timeval));

	while(1)
	{
   
		/*initializing variables*/
		char array[] = " ";
		char command[100]= "";
		char filename[100] = "";
		char *str1;
		char *str2;

		/*print the options,take the command input and filename*/
		printf("\nList of commands\n");
    printf("1. get filename\n");
    printf("2. put filename\n");
    printf("3. delete filename\n");
    printf("4. ls filename\n");
    printf("5. exit\n");

		printf("\nenter command with file name: ");
    scanf( "%[^\n]s",array);
		getchar();
		
		/*check if command is exit*/
		if(strcmp(array,"exit")!= 0)
		{
			/*split the string to get command and filename*/
			str1 = strtok(array," ");
			strcpy(command,str1);
			str2 = strtok(NULL," ");    
			strcpy(filename,str2);
		}
		else
		{
			strcpy(command,array);
		}
  
    /*if command is put, check if file is present*/
    if(strcmp(command,"put") == 0)
    {
      /*check if file is present*/
      FILE *fw;
			fw = fopen(filename,"r");
      if(fw == NULL)
      {
        printf("\n\nERROR: NO SUCH FILE\n\n");
        continue;
      }
      else
      {
        printf("\nFILE PRESENT\n");
      }
    }

		printf("command is %s\n",command); 
    
		/*find command length*/
		int command_length = strlen(command);
		n = sendto(sock,command,command_length,0,(struct sockaddr*)&server,length);

		if(strcmp(command,"exit")!= 0)
		{
			/*Send the file name if command is not exit*/
			n = sendto(sock,filename,strlen(filename),0,(struct sockaddr*)&server,length);
			printf("file name is %s\n",filename);
    }
		else
		{
			//printf("\nexit command: no file\n");
		}
  
    int x = 0;

    /*check if command is put*/
		if(strcmp(command, "put") == 0)
		{
			/*set elements of buffer to zero*/
			bzero(buffer,sizeof(buffer));

			/*calculate hashed value for file to be sent*/
			char array[MAX_LENGTH] = "";
			snprintf(array,sizeof(array),"sha256sum %s > sha_256_put.txt ",filename);
			system(array);   
			
			/*send the hashed value*/
			FILE *fp;
			fp = fopen("sha_256_put.txt","r");
			fread(buffer,1,256,fp);
			n = sendto(sock,buffer,256,0,(struct sockaddr*)&server, length);
			if(n < 0)
			{
				error("ERROR: Send to");
			}
			fclose(fp);
    
			bzero(buffer,sizeof(buffer));		
     
		  /*open the file to send and calculate its size*/
			FILE *f;
			f = fopen(filename,"r");
			int size_of_file;
			fseek(f,0,SEEK_END);            //traversing the entire file till last
			size_of_file = ftell(f);        // get the size of f
			fseek(f,0,SEEK_SET);            //get the cursor back to start of file
     
			/*converting size of file to char*/
			char msg[100];
			sprintf(msg,"%d",size_of_file);
			printf("\n size of file is: %d\n",size_of_file);

			bzero(buffer,MAX_LENGTH);
			bzero(buffer_1,MAX_LENGTH+1);

			/*Send the length of file*/
			n = sendto(sock,msg,strlen(msg),0,(struct sockaddr*)&server,length);
  
			if(n<0)
			{
				error("ERROR: Send to");
			}
  
			int no_of_loops = 0;
			int send_file_size = 0;
			int size_of_file_1 = size_of_file;
			int size_sent = 0;
     
			/*calculate number of loops*/
			if(size_of_file < MAX_LENGTH )
			{
				no_of_loops = 1;
			}
			else
			{
				no_of_loops = 1+(size_of_file/MAX_LENGTH);
			}
     
			int z = no_of_loops;	  
			int packet_count = 1;
   
			do
			{ 
				/*converting packet number to char*/
				sprintf(packet_number,"%d",packet_count);
				strcpy(buffer_1,packet_number);
        
				/*calculate packet size to be sent*/
				if(no_of_loops  == 1)
				{
					send_file_size = (size_of_file % MAX_LENGTH)+1;
					printf("last file size %d\n",send_file_size);
				}
				else 
				{
					send_file_size = MAX_LENGTH+1;
				}
        
        /*creating an array of 1's and 0's to xor*/
        char encrypt[send_file_size];
        int temp = 0;
        for(int i = 0; i<send_file_size;i++)
        {
            if(temp == 0)
            {
              encrypt[i] = '1';
              temp = 1;
            }
            else if(temp == 1)
            {
              encrypt[i] = '0';
              temp = 0;
            }
        }



				/*copy contents from file and append it the buffer containing packet number*/
				fread(buffer,1,send_file_size-1,f);
				for(int k = 0;k <send_file_size-1; k++)
				{
          buffer[k]^ =  encrypt[k];
					buffer_1[1+k] = buffer[k];
					packet[k] = buffer[k];
				}
        
				while(1)
				{
					/*send the packet*/
					x = sendto(sock,buffer_1,send_file_size,0,(struct sockaddr*)&server,length);
					
					n =  recvfrom(sock,recd_ack,sizeof(recd_ack),0,(struct sockaddr *)&from, &length);
					/*wait for receiver response and check for ack*/
					if(n >=0)
					{
            if((atoi(recd_ack)) == packet_count )
						{             	
              bzero(recd_ack,sizeof(recd_ack));
              break;
						}
						else
						{
							printf("\nreceived nack\n");
							bzero(recd_ack,sizeof(recd_ack));
              continue;
						}
					}
					else
					{
						printf("\n\nTIMEOUT TIMEOUT TIMEOUT TIMEOUT TIMEOUT TIMEOUT \n\n");
					
            continue;
          }
				}
				
				/*calculate remaining number of loops and size of file left*/
				no_of_loops--;
				size_of_file = size_of_file - MAX_LENGTH;
	
  			/*calculate size sent till now*/
				size_sent = size_sent + x -1;
	      printf("\nSize sent till now: %d\n",size_sent);

        /*modifying packet value*/
			  if(packet_count == 2)
				{
				    packet_count = 1;
				}
				else if(packet_count == 1)
				{
				    packet_count = 2;
				}
     
        /*setting elements to zero*/
				bzero(buffer,MAX_LENGTH);
				bzero(buffer_1,MAX_LENGTH+1);
				bzero(packet,sizeof(packet));
				bzero(recd_ack,sizeof(recd_ack));
    
			}while(no_of_loops > 0);
     
			fclose(f);			//close the file
			
      bzero(packet_number,sizeof(packet_number));
			packet_count = 0;
			memset(command,0,command_length);
     
			printf("size of file to be sent is %d\n",size_of_file_1);
			printf("size of file sent is %d\n",size_sent);
			printf("total no of loops is %d\n",z);
		}
		else if(strcmp(command, "ls") == 0)		//check if command is ls
		{
			
      bzero(buffer,MAX_LENGTH);
			
			/*open file*/
			FILE *f;
			f = fopen(filename,"w");
			
			/*receive length of file*/
			n = recvfrom(sock,buffer,MAX_LENGTH,0,(struct sockaddr*)&from,&length);
			if(n<0)
			{
				error("ERROR: Receive from");
			}

			/*Receiving size of the ls output file*/
			int  size_of_file = atoi(buffer);
			int size_recd = size_of_file;
			bzero(buffer,sizeof(buffer));
			bzero(buffer_1,sizeof(buffer_1));

			int no_of_loops = 0;

			/*checking if loop is required*/
			if(size_of_file < MAX_LENGTH )
			{
				no_of_loops = 1;
			}
			else
			{
				no_of_loops = 1+ size_of_file/MAX_LENGTH;
			}
   
			int file_size = 0 ; 
			int recd_count = 0;
			int size = 0;
			int packet_count = 1;
      int counter = 0;
      do
			{ 
				/*calculate size of packet to be sent*/
				if(no_of_loops  == 1)
				{
					size = (size_of_file % MAX_LENGTH) + 1;
				}
				else 
				{
					size = MAX_LENGTH + 1;
				}
        
				while(1)
				{
					/*receive packet*/
					n = recvfrom(sock,buffer_1,size,0,(struct sockaddr*)&server,&length);
				
					/*convert received count to integer*/
					recd_count = buffer_1[0] - '0';
					
					if(n >= 0)
					{
						/*check if received count matches*/
						if(recd_count == packet_count)
						{
              bzero(send_ack,sizeof(send_ack));
							/*send ack*/
							sprintf(send_ack,"%d",packet_count);
              printf("%d %s\n\n\n\n",packet_count,send_ack);
							sendto(sock,send_ack,sizeof(send_ack),0,(struct sockaddr *)&server,length);
				      bzero(send_ack,sizeof(send_ack));			
							/*copy the contents of buffer and removing the packet number*/
							for(int j= 0; j < size-1 ;j++)
							{
								buffer[j] = buffer_1[j+1];
								packet[j] = buffer[j];
							}
							
							/*write the contents of packet to file*/
							fwrite(buffer,1,size-1,f);
							bzero(buffer,sizeof(buffer));
							bzero(buffer_1,sizeof(buffer));
							
							/*calculate file size received*/
							file_size = file_size + n - 1;
              bzero(send_ack,sizeof(send_ack));
							break;
						}
						else
						{
             /*send nack and if counter is 2, resend ack for previous received packet*/
							counter++;
							printf("\nPacket not received: %d\n",counter);
              if(counter == 2)
              {
                if(packet_count == 1)
                {
                  sprintf(send_ack,"%d",2);
                }
                else if(packet_count == 2)
                {
                  sprintf(send_ack,"%d",1);
                }
                sendto(sock,send_ack,sizeof(send_ack),0,(struct sockaddr *)&server, length);
                counter = 0;
              }
              bzero(send_ack,sizeof(send_ack));
            }
					}
          else
          {
            printf("\nTIMEOUT TIMEOUT TIMEOUT\n");
          }
				}

				/*calculate number of loops remaining and size of file remaining*/
				no_of_loops--;
				size_of_file = size_of_file - MAX_LENGTH;
				
				/*modify the packet count*/
				if(packet_count == 2)
				{
					packet_count = 1;
				}
				else if(packet_count == 1)
				{
					packet_count = 2;
				}
        
				/*set the elements to zero*/
				bzero(buffer, MAX_LENGTH);
				bzero(buffer_1, MAX_LENGTH+1);

			}while(no_of_loops > 0);

			/*close the file*/
			fclose(f);
      
			printf("total file received is %d\n",file_size);
			printf("total file size to be received is %d\n",size_recd);
      
			/*set the elements to zero*/
			bzero(packet,sizeof(packet));
			bzero(packet_number,sizeof(packet_number));
			bzero(buffer,sizeof(buffer));
			bzero(buffer_1, MAX_LENGTH+1);
      
			memset(command,0,command_length);
		}
		else if(strcmp(command,"get") == 0)  //check if command is get
		{
		
      /*check if file is present on server side*/
      char file_check[10] = "";
			n = recvfrom(sock,file_check,10,0,(struct sockaddr*)&server,&length);
			if(n<0)
			{
				printf("ERROR: receiver");
			}
      if(strcmp(file_check,"yes") == 0)
      {
        printf("\nFILE PRESENT IN SERVER\n");
      }
      else
      {
        printf("\nERRROR: FILE NOT PRESENT IN SERVER\n");
        continue;
      }

			/*receive the hash value of the file to be received*/
			char recd_sha[256] = "";
			n = recvfrom(sock,recd_sha,256,0,(struct sockaddr*)&server,&length);
			if(n<0)
			{
		  	printf("ERROR: receiver");
			}
			
			/*receive the file size*/
			int filesize_recd = 0;
			n = recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *)&server, &length);
			if (n<0)
			{
				error("ERROR: receiver");
			}	
			filesize_recd = atoi(buffer);
      
			bzero(buffer,sizeof(buffer));
			bzero(buffer_1,sizeof(buffer_1));
      
			/*open the file to store contents*/
      FILE *f;
			f = fopen(filename,"w");
			int no_of_loops = 0;
			int size = 0;
			int size_of_file = filesize_recd;      

			/*calculate the number of loops*/
			if(filesize_recd > MAX_LENGTH)
			{
				no_of_loops = filesize_recd/MAX_LENGTH + 1;
			}
			else
			{
				no_of_loops = 1;
			}
      
			int packet_count = 1;
			int recd_count = 0;
			int file_size = 0;  
      int counter = 0;
			
      do
			{
				/*calculate the packet size to be sent*/
				if(no_of_loops == 1)
				{
					size = (filesize_recd%MAX_LENGTH) + 1;  
				}
				else
				{
					size = MAX_LENGTH + 1;
				}
  
        /*create an array of 1's and 0's for encryption*/
        char decrypt[size];
        int temp = 0;
        for(int i = 0; i<size;i++)
        {
            if(temp == 0)
            {
              decrypt[i] = '1';
              temp = 1;
            }
            else if(temp == 1)
            {
              decrypt[i] = '0';
              temp = 0;
            }
        }

				while(1)
				{
					/*receive pcaket*/
					n = recvfrom(sock,buffer_1,size,0,(struct sockaddr*)&server,&length);

					/*convert received count to integer*/
					recd_count = buffer_1[0] - '0';
            
		   		if(n >= 0)
					{
						/*check if received packet number matches*/
						if(recd_count == packet_count)
						{ 
							/*send ack*/
							sprintf(send_ack,"%d",packet_count);
							sendto(sock,send_ack,sizeof(send_ack),0,(struct sockaddr *)&server,length);
							bzero(send_ack,sizeof(send_ack));
				
              /*copy the contents of packet into file removing the packet number*/
							for(int j= 0; j <size-1;j++)
							{
								buffer[j] = buffer_1[j+1];
				        buffer[j] ^= decrypt[j];
                packet[j] = buffer[j];
							}
							fwrite(buffer,1,size-1,f);
							bzero(buffer,sizeof(buffer));
							
							/*calculate rfile received*/
							file_size = file_size + n -1;	
							break;
						}
						else
						{
  /*send nack and if two times previous packet is received, resend ack for previous received packet*/
							counter++;
							printf("\nPacket not received: %d\n",counter);
              if(counter == 2)
              {
                if(packet_count == 1)
                {
                  sprintf(send_ack,"%d",2);
                }
                else if(packet_count == 2)
                {
                  sprintf(send_ack,"%d",1);
                }
                sendto(sock,send_ack,sizeof(send_ack),0,(struct sockaddr *)&server, length);
                counter = 0;
              }
              bzero(send_ack,sizeof(send_ack));
            }
					}
          else
          {
            printf("\n\n\nTIMEOUT TIMEOUT TIMEOUT TIMEOUT TIMEOUT TIMEOTU TIMEOUT\n\n\n");
            continue;

          }
				}
				
				/*calculate remaining number of loops and file size remaining*/
				no_of_loops--;
				size_of_file = size_of_file - MAX_LENGTH;
        
				/*modifying packet number*/
				if(packet_count == 2)
				{
					packet_count = 1;
				}
				else if(packet_count == 1)
				{
					packet_count = 2;
				}
				
				/*set elements to zero*/
				bzero(buffer, MAX_LENGTH);
				bzero(buffer_1, MAX_LENGTH+1);
				bzero(packet,sizeof(packet));
			}while(no_of_loops > 0);
      
			printf("total file received is %d\n",file_size);
			printf("total file size to be received is %d\n",filesize_recd);
      
			fclose(f);		//close the file
   	
			/*set elements to zero*/
			bzero(packet_number,sizeof(packet_number));
			bzero(buffer,sizeof(buffer));
			bzero(buffer_1, sizeof(buffer_1));
			bzero(packet,sizeof(packet));
			memset(command,0,command_length);
			
			/*calculate hashed value of received file*/
			char array[MAX_LENGTH] = "";
			snprintf(array,sizeof(array),"sha256sum %s >sha256_op_get.txt",filename);
			system(array);
		
			FILE *fp;
			fp = fopen("sha256_op_get.txt","r");
			fread(buffer,1,256,fp);
      
			/*compare if hashed values match*/
			if(strcmp(buffer,recd_sha) == 0)
			{
				printf("\nHASH MATCHED SUCCCESSS!!!\n");
			}
      else
      {
        printf("\nHASH DID NOT MATCH> FAILUREEEe.\n");
      }
			
      bzero(buffer,strlen(buffer));
		}
		else if(strcmp(command,"delete") == 0) 		//check if command is delete
		{	
      /*check if file is present on server side*/
      char file_check[10] = "";
			n = recvfrom(sock,file_check,10,0,(struct sockaddr*)&server,&length);
			if(n<0)
			{
				printf("ERROR: receiver");
			}
      if(strcmp(file_check,"yes") == 0)
      {
        printf("\nFILE PRESENT IN SERVER\n");
      }
      else
      {
        printf("\nERRROR: FILE NOT PRESENT IN SERVER\n");
        continue;
      }
      memset(command,0,command_length);
		}
		else if(strcmp(command,"exit") == 0)		//check if command is exit
		{
			memset(command,0,command_length);
			printf("\nEXIT: client side\n");
			close(sock);							//close the socket
			exit(1);								//exit
		}
		else
		{
			printf("\nwrong command\n");			//wrong command
		}
	}
}




