#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>


#define TIMEOUT_SEC 3		//Macro to define timeout
#define MAX_LENGTH 512   //Macro to define length of packets

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
  int sock,length,n,fromlen;
  char buffer[MAX_LENGTH];
  char buffer_1[MAX_LENGTH+1];
  struct sockaddr_in server;
  struct sockaddr_in from;
  char command[10] = "";
  char file_name[100] = "";
  char packet[MAX_LENGTH] = "";
  char recd_ack[10] = "";
  char send_ack[10] = "";
  char packet_number[10] = "";

  /*to check if two arguments are passed*/ 
  if(argc < 2)
  {
	  fprintf(stderr,"ERROR, no port is provided");
    exit(1);
  }

	/*Creating a socket 
	AF_INET = refers to addresses from the internet
	SOCK_DGRAM = UDP*/
  sock = socket(AF_INET, SOCK_DGRAM, 0);
   
  if(sock < 0)
  {
    error( "ERROR: opening socket");
  } 
   
  /*finding size of struct server*/
  length = sizeof(server);
   
  /*set all values in a buffer to zero*/
  bzero(&server,length);
   
  /*contains code for address family, equal to AF_INET*/
  server.sin_family = AF_INET;
   
  /*contains IP address of the host*/
  server.sin_addr.s_addr = INADDR_ANY;
   
  /*storing port number in network byte order*/
  server.sin_port = htons(atoi(argv[1]));
   
  /*binding socket o address of host and port number*/
  if(bind(sock,(struct sockaddr *)&server,length)<0)
  {
     error("ERROR: Binding");
  }
  
  /*size of struct sockaddr_in*/
  fromlen = sizeof(struct sockaddr_in);

  /*adding timeout for recvfrom*/
  struct timeval timeout = {TIMEOUT_SEC,0};
  setsockopt(sock,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));  

  int x = 0;
 
  while(1)
  {
    /*setting contents of buffer to zero*/
    bzero(buffer,sizeof(buffer));    
    
    /*while loop to wait for first command from client side*/	
    while(1)
    {
      /*waiting to receive command*/
      n = recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *)&from, &fromlen);
      if(n>=0)
      {
		    printf("\nrecd command\n");
		    break;
      }
      else
      {
		    printf("\nwaiting to receive command\n");
		    continue;
      }
    }
   
    /*copy the contents of buffer into command*/
    strcpy(command,buffer);
    printf("\ncommmand is %s\n",command);
    
    bzero(buffer,sizeof(buffer));

    /*if command is not exit, receive file name*/
    if(strcmp(command, "exit")!=0)
    {
      n = recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *)&from, &fromlen);
      if (n<0)
      {
        error("ERROR: receiver");
      }     
    
      strcpy(file_name,buffer);
      printf("\nfile name is : %s\n",file_name);
    }
   
    bzero(buffer,sizeof(buffer));

    /*check if command is put*/
    if(strcmp(command, "put") == 0)
    { 

		  /*receiving hash of the original file*/
		  char recd_sha[256] = "";
		  n = recvfrom(sock,recd_sha,256,0,(struct sockaddr*)&from, &fromlen);
	  	if(n<0)
		  {
		  	printf("ERROR: receiver\n");
		  	continue;
		  }	  
		
		  /*set all elements of packet to zero*/
		  bzero(packet,sizeof(packet));

		  /*receive size of file*/
		  int filesize_recd = 0;
		  n = recvfrom(sock,buffer,sizeof(buffer),0,(struct sockaddr *)&from, &fromlen);
	  	if (n<0)
	  	{
	  		error("ERROR: receiver");
	  	}
		
		  /*converting buffer to integer*/
		  filesize_recd = atoi(buffer);
		  bzero(buffer,sizeof(buffer));
		  bzero(buffer_1,sizeof(buffer_1));
		
	  /*creating a file pointer*/
		  FILE *f;
		  f = fopen(file_name,"w");  //open file to write
		  int no_of_loops = 0;
		  int size = 0;
		  int size_recd = filesize_recd;

		  /*calculating number of loops required for transmission*/
		  if(filesize_recd > MAX_LENGTH)
		  {
			  no_of_loops = filesize_recd/MAX_LENGTH + 1;
		  }
		  else
		  {
			  no_of_loops = 1;
		  }
		
		  /*creating a variable to store packet count i.e. 0 or 1*/
		  int packet_count = 1;
		  int recd_count = 0;
		  int file_size = 0;
		  int counter = 0;
		 
		  do
		  {
			  /*calculate size of packet to be sent depending on number of loops remaining*/
			  if(no_of_loops == 1)
			  {
				  size = (filesize_recd%MAX_LENGTH)+1;
			  }
			  else
			  {
				  size = MAX_LENGTH+1;
			  }
        
			 
			 char decrypt[size];
			 int temp = 0;
			 for(int i=0; i<size; i++)
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
				  /*receive packet*/
				  n = recvfrom(sock,buffer_1,size,0,(struct sockaddr *)&from, &fromlen);
				
				  /*check if pcaket number  is zero or one*/
				  recd_count = buffer_1[0] - '0';
	 
				  /*check if packet is received or not*/
				  if(n >= 0)
				  {
					  /*check if received packet number matches*/
					  if(recd_count == packet_count)
					  {
						  /*send ack*/
						  sprintf(send_ack,"%d",packet_count);
						  sendto(sock,send_ack,sizeof(send_ack),0,(struct sockaddr *)&from, fromlen);
						  bzero(send_ack,sizeof(send_ack));
						  for(int j = 0;j<size-1;j++)
						  {
							  buffer[j] = buffer_1[j+1];
							  buffer[j] ^= decrypt[j];
							  packet[j] = buffer[j];
						  }
						
						  /*write the contents of packet received to the file*/
						  fwrite(buffer,1,size-1,f);
						  bzero(buffer,sizeof(buffer));
						
						  /*calculate file size received uptil now*/
						  file_size = file_size + n-1;	
						  
              /*exit the while loop*/
						  break;
					  }
					  else
					  {
						  /*send a nack twice, if count is equal to then resend ack for previous recd packet*/
						  counter++;
						  printf("\nPacket not received : %d\n",counter);
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
							  sendto(sock,send_ack,sizeof(send_ack),0,(struct sockaddr *)&from, fromlen);
							  counter = 0;
						  }
						
              bzero(send_ack,sizeof(send_ack));
					  }
				  }
			  } 
			
        no_of_loops--;		//decrement loop size
			
			  /*calculate remaining file size*/
			  filesize_recd = filesize_recd - MAX_LENGTH;  
			  
        /*modify packet count value*/
			  if(packet_count == 1)
			  {
				  packet_count = 2;
			  }
			  else
			  {
				  packet_count = 1;
			  }
			
			  /*set elements to zero*/
			  bzero(buffer,sizeof(buffer));
			  bzero(packet,sizeof(packet));
			  bzero(buffer_1,sizeof(buffer_1));
			  bzero(recd_ack,sizeof(recd_ack));
		  
      }while(no_of_loops > 0);
      
		  printf("total file received is %d\n",file_size);
		  printf("total file size to be received is %d\n",size_recd);
		
      fclose(f);		//close the file
		
		  bzero(buffer,sizeof(buffer));

		  bzero(buffer,sizeof(buffer));
		  bzero(packet,sizeof(packet));
		  bzero(command,sizeof(command));

		  /*calculate the hashed value of packet received*/
		  char array[MAX_LENGTH] = "";
		  snprintf(array,sizeof(array),"sha256sum %s > sha256_op_put.txt",file_name);
		  system(array);   
      
	    /*check if hash matches*/
		  FILE *fp;
		  fp = fopen("sha256_op_put.txt","r");
		  fread(buffer,1,256,fp);
		  if(strcmp(buffer,recd_sha) == 0)
		  {
			  printf("\n HASH MATCHED SUCCESSSSSSSS!!!!!!! \n");
		  }
		  else
		  {
			printf("\nHASH DID NOT MATCH FAILUREEEEEE!!!!!\n");
		  }
		  bzero(buffer,sizeof(buffer));

    }
    else if(strcmp(command, "get")==0)	//check if command is get
    {
      bzero(buffer,sizeof(buffer));
		  
      /*check if file is present*/
      		  FILE *fw;
		  fw = fopen(file_name,"r");
		  if(fw == NULL)
		  {
        printf("\nERRROR: FILE NOT PRESENT\n");
			  char file_no[2] = "no";	
			  n = sendto(sock,file_no,2,0,(struct sockaddr *)&from,fromlen);
			  continue;
		  }
		  else
		  {
			  char file_yes[3] = "yes";	
			  n = sendto(sock,file_yes,3,0,(struct sockaddr *)&from,fromlen);
			  fclose(fw);
		  }
		
      /*calculate hash of file to  be sent*/
		  char array[MAX_LENGTH] = "";
		  snprintf(array,sizeof(array),"sha256sum %s > sha256_op_get.txt ",file_name);
		  system(array);   
	  
		  /*send the hashed value*/
		  FILE *fp;
		  fp = fopen("sha256_op_get.txt","r");
		  fread(buffer,1,256,fp);
		  n = sendto(sock,buffer,256,0,(struct sockaddr *)&from,fromlen);
		  if(n<0)
		  {
			  error("ERROR: Send to");
		  }
		  fclose(fp);     
      
		  bzero(buffer,sizeof(buffer));

      /*file pointer to open the encrypted file*/
		  FILE *f;
		  f = fopen(file_name,"r");
		  {
		  	printf("\n file not present\n");
		  }

		  /*calculate size of file*/
		  int size_of_file;
		  fseek(f,0,SEEK_END);            //traversing the entire file till last
		  size_of_file = ftell(f);        // get the size of f
		  fseek(f,0,SEEK_SET);            //get the cursor back to start of file

		  char msg[100]= "";
		  sprintf(msg,"%d",size_of_file);

		  bzero(buffer,MAX_LENGTH);
		  bzero(buffer_1,MAX_LENGTH+1);

  		/*Send the length of file*/
	  	n = sendto(sock,msg,strlen(msg),0,(struct sockaddr*)&from,fromlen);
  
	  	if(n<0)
		  {
			  error("ERROR: Send to");
		  }
        
		  int no_of_loops = 0;
		  int send_file_size = 0;
		  int size_of_file_1 = size_of_file;
		  int size_sent = 0;

		  /*calculate no of loops required for transmission*/
		  if(size_of_file < MAX_LENGTH )
		  {
			  no_of_loops = 1;
		  }
		  else
		  {
			  no_of_loops = 1+ size_of_file/MAX_LENGTH;
		  }
      
		  int packet_count = 1;       
		  int z = no_of_loops;     
		  do
		  {	
			  /*convert packet number to char*/
			  sprintf(packet_number,"%d",packet_count);
			  strcpy(buffer_1,packet_number); 
			
		  	/*calculate size of packet to send*/
	  		if(no_of_loops  == 1)
			  {
				  send_file_size = (size_of_file % MAX_LENGTH) + 1;
			  }
			  else 
			  {
				  send_file_size = MAX_LENGTH+1;
			  }
            			 
			 char encrypt[send_file_size];
			 int temp = 0;
			 for(int i=0; i<send_file_size; i++)
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
	
			  /*read the contents of file to packet to send*/
			  fread(buffer,1,send_file_size-1,f);
			  for(int j= 0; j <send_file_size-1;j++)
			  { 
				buffer[j] ^=  encrypt[j];
               			buffer_1[j+1] = buffer[j];
               			packet[j] = buffer[j];
			  }

			  while(1)
			  {
			    /*send the packet*/
				  x = sendto(sock,buffer_1,send_file_size,0,(struct sockaddr*)&from,fromlen);
				
			  	/*wait for response from receiver*/
				  n = recvfrom(sock,recd_ack,sizeof(recd_ack),0,(struct sockaddr *)&from, &fromlen);
				  if(n >= 0)
				  {	
				  	/*check if ack is received*/
					  if(atoi(recd_ack)== packet_count)
					  {
					  	bzero(recd_ack,sizeof(recd_ack));
						  break;
					  }
					  else
					  {
						  printf("\n nack is received\n");
						  bzero(recd_ack,sizeof(recd_ack));
						  continue;
					  }
				  }
				  else
				  {
					  printf("\n\n TIMEOUT TIMEOUTTIMEOUT TIMEOUT TIMEOUT TIMEOUT TIMEOUT\n\n");
					  continue;
				  }
			  }
        
			  /*decrease number of loops, size of file sent, and remaining file size*/
			  no_of_loops--;
			  size_of_file = size_of_file - MAX_LENGTH;
			  size_sent = size_sent + x - 1;
        printf("\nsize sent till now: %d\n",size_sent);
			
        /*modify packet count*/
			  if(packet_count == 2)
			  {
				  packet_count = 1;
			  }
			  else if (packet_count == 1)
			  {
				  packet_count = 2;
			  }
 
  			/*set elements to zero*/
	  		bzero(buffer,MAX_LENGTH);
		  	bzero(buffer_1,sizeof(buffer_1));
		  	bzero(packet, sizeof(packet));
			  bzero(recd_ack,sizeof(recd_ack));

		  }while(no_of_loops > 0); 
      
		  packet_count = 0;
		  bzero(packet_number,sizeof(packet_number));
		  fclose(f);	//close the file
		  memset(command,0,strlen(command));
		  printf("size of file to be sent is %d\n",size_of_file_1);
		  printf("size of file sent is %d\n",size_sent);
		  printf("total no of loops is %d\n",z);    
    }
    else if(strcmp(command, "ls") == 0)		//check if command is ls
    {
      /*system call ls and copy the output to a text file*/
	    bzero(buffer,MAX_LENGTH);
      snprintf(buffer,sizeof(buffer)," ls -a >%s",file_name);
      system(buffer);
 	
	    /*open the file*/
      FILE *f;
      f = fopen(file_name,"r");
		
	    /*calculate size of file*/
      int size_of_file;
      fseek(f,0,SEEK_END);            //traversing the entire file till last
      size_of_file = ftell(f);        // get the size of f
      fseek(f,0,SEEK_SET);            //get the cursor back to start of file

	    /*convert size of file to char*/
      char msg[100]= "";
      sprintf(msg,"%d",size_of_file);
      printf("\n size of ls text file is: %d\n",size_of_file);

      bzero(buffer,MAX_LENGTH);
      bzero(buffer_1,MAX_LENGTH);

      /*Send the length of file*/
      n = sendto(sock,msg,strlen(msg),0,(struct sockaddr*)&from,fromlen);

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
        no_of_loops = 1 + (size_of_file/MAX_LENGTH);
      }
	
      int z = no_of_loops;
      int packet_count = 1;
        
      do
      {
		    /*convert packet number to char*/
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
         
	      /*read contents of file and append it after the packet number*/
		    fread(buffer,1,send_file_size-1,f);
		    for(int j= 0; j <send_file_size-1;j++)
		    {
		    	buffer_1[j+1] = buffer[j];
			    packet[j] = buffer[j];
		    }
		    while(1)
		    {
			    /*send the packet*/
			    x = sendto(sock,buffer_1,send_file_size,0,(struct sockaddr*)&from,fromlen);

			    /*waiting for ack*/
			    n = recvfrom(sock,buffer,MAX_LENGTH,0,(struct sockaddr *)&from, &fromlen);
			    
          if(n >= 0)
				  {	
				  	/*check if ack is received*/
					  if(atoi(buffer)== packet_count)
					  {
					  	bzero(buffer,sizeof(buffer));
						  break;
					  }
					  else
					  {
						  printf("\n nack is received\n");
						  bzero(buffer,sizeof(buffer));
						  continue;
					  }
				  }
				  else
				  {
					  printf("\n\n TIMEOUT TIMEOUTTIMEOUT TIMEOUT TIMEOUT TIMEOUT TIMEOUT\n\n");
					  continue;
				  }
        }
			  /*calculate file size sent, no of loops remaining and size of file remaining*/
		
	    	no_of_loops--;
		    size_of_file = size_of_file - MAX_LENGTH;
    		size_sent = size_sent + x - 1;
		
		    /*modify packet count*/
		    if(packet_count == 2)          
		    {
			    packet_count = 1;
		    }
		    else if(packet_count == 1)
		    {
			    packet_count = 2;
		    }

		    /*set elements to zero*/
		    bzero(buffer,MAX_LENGTH);
		    bzero(packet,sizeof(packet));
		    bzero(buffer_1, sizeof(buffer_1));
		    bzero(recd_ack , sizeof(recd_ack));
	    }while(no_of_loops > 0);
      
	    bzero(packet_number,sizeof(packet_number));
	    packet_count = 0;
     
	    /*close the file*/
	    fclose(f); 
	    memset(command, 0 ,sizeof(command));
    
	    printf("size of file to be sent is %d\n",size_of_file_1);
	    printf("size of file sent is %d\n",size_sent);
	    printf("total number of loops is %d\n",z);
    }
    else if(strcmp(command, "delete") == 0)		//check if command is delete
    {
      /*check if file is present*/
      FILE *fw;
		  fw = fopen(file_name,"r");
		  if(fw == NULL)
		  {
        printf("\nERROR: FILE NOT PRESENT\n");
        char file_no[2] = "no";	
			  n = sendto(sock,file_no,2,0,(struct sockaddr *)&from,fromlen);
			  continue;
		  }
		  else
		  {
			  char file_yes[3] = "yes";	
			  n = sendto(sock,file_yes,3,0,(struct sockaddr *)&from,fromlen);
			  fclose(fw);
		  }

		  int status = remove(file_name);			//delete file
		  if(status == 0)
		  {
			  printf("\n%s file is successfully deleted\n",file_name);  
		  }
		  else
		  {
			  printf("\nerror in deleting the file\n");
		  }
    }
    else if(strcmp(command, "exit") == 0)		//check if command is exit
    {
		    printf("\nEXIT: server side\n");
        close(sock);
        exit(1);
    }
    else
    {
        printf("\nunknown command\n");
    }
  }
}

