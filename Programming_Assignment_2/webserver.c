#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>
#include<stdbool.h>
#include<signal.h>
#include<fcntl.h>
#include<unistd.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/wait.h>
#include<netinet/in.h>
#include<arpa/inet.h>
#include<netdb.h>

#define CONN_MAX 1000
#define BYTES 1024
#define BUFFER_SIZE 512

FILE *file_ptr;

typedef struct index_values
{
  char *index_array[3];
}index_val;

struct node* head = NULL;

/*Enum to describe the function return values*/
typedef enum
{
	SUCCESS = 1,
	FAIL = 2
}states;


/*Double linked list Structure to store format name and format description*/
struct node
{
  char format_extension[10];
  char format_description[20];
  struct node *next; // Pointer to next node in DLL
  struct node *prev; // Pointer to previous node in DLL
};

/*char array for error 404*/
char error_not_found[] = 
"HTTP/%0.1f 404 Not Found\r\n"
"Content-Type: text/html; charset = UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<body><center><h1>ERROR 404: NOT FOUND!!! REASON: URL does not exist</h1><br>\r\n";

char error_not_found_send[250];

/*character array for error 500*/
char extension_error[] = 
"HTTP/%0.1f 500 Format Not Supported\r\n"
"Content-Type: text/html; charset = UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<body><center><h1>ERROR 500: REASON:given file format not supported </h1><br>\r\n";

char extension_error_send[250];

/*character array for error 501*/

char method_not_supported_1[] = 
"HTTP/1.1 501 NOT IMPLEMENTED\r\n"
"Content-Type: text/html; charset = UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<body><center><h1>ERROR 501: METHOD NOT IMPLEMENTED </h1><br>\r\n";

/*char array for error 400*/
char bad_request[] = 
"HTTP/1.1 400 BAD REQUEST\r\n"
"Content-Type: text/html; charset = UTF-8\r\n\r\n"
"<!DOCTYPE html>\r\n"
"<body><center><h1>ERROR 400: BAD REQUEST REASON: %s </h1><br>\r\n";

char bad_request_send[250];

/*global variable declaratoin*/
int listen_fd; 
int clients[CONN_MAX];
char extension[10];

/*declaring function prototypes*/
states position_insert(struct node** head_ref,uint32_t position_to_be_inserted,char* format_ext, char* format_desc);
uint32_t dll_length(struct node* node);
void print_list(struct node *node);
void error(char *);
void start_server(char *);
void respond(int a ,char *str);
int get_port_number(char *buffer);
void get_format(char *buffer, int iter);
char* get_root_directory(char *buffer);
int get_extension(char *format_name);
int check_extension(char *name);
index_val get_index_types(char *buffer);


/*MAIN*/
int main(int argc, char* argv[])
{

  char root_dir[100];
  char port[10];
  char buffer[BUFFER_SIZE];
  char *buffer1;
  int flag = 0;
  int port_number;
  index_val values;
  char *root_directory;
  struct sockaddr_in client_address;
  socklen_t address_length;
  int slot = 0;

  /*file pointer to open ws.comf file*/
  file_ptr = fopen("ws.conf","r");

  /*check if file exists*/
  if(file_ptr == NULL)
  {
    printf("ERROR: OOOPS!!! ws.conf file not found...  BYE");
    exit(1);
  }
  else
  {
	/*Till end of file is reached*/
    while(!feof(file_ptr))
    {
	  /*read the contents of file into a buffer*/	
      fgets(buffer, BUFFER_SIZE, file_ptr);
      
      /*tokenize at every new line*/
      strtok(buffer, "\n");
     
      /*depending on the flag value extract the information as required*/
      
      if(flag == 1)       /*get the port number*/
      {
        port_number = get_port_number(buffer);
        sprintf(port,"%d",port_number);
        printf("\nPort number is %d\n",port_number);
      }
      else if(flag == 2)		/*get the root directory*/
      {
        root_directory = get_root_directory(buffer);
        strcpy(root_dir,root_directory);
        printf("\nroot directory is %s\n\n",root_directory);
      }
      else if(flag == 3)		/*get the various types of index files*/
      {
        values = get_index_types(buffer);
      }
      else if(flag == 4)		/*get the different extensions and their description*/
      {
          for(int i = 0; i<9; i++)
          {
              get_format(buffer,i);
              fgets(buffer, BUFFER_SIZE, file_ptr);
          }
		  
		  /*check the linked list value and print it*/
          uint32_t length = dll_length(head);

      }

      /*check for different lines in the ws.conf file and accordingly set the flag*/
      if(strcmp(buffer, "#serviceport number") == 0)
      {
        flag = 1;
      }
      else if(strcmp(buffer, "#document root") == 0)
      {
        flag = 2;
      }
      else if(strcmp(buffer, "#default web page") == 0)
      {
        flag = 3;
      }
      else if(strcmp(buffer, "#Content-Type which the server handles") == 0)
      {
        flag = 4;
      }
      else
      {
        flag = 0;
      }
    }
  }

	  /*setting all elements to - 1 to indicate no client is connected*/
    int i;
    for(i =0;i<CONN_MAX;i++)
      clients[i] = -1;

    /*start server*/
    start_server(port);

    int child_pid;

	  /*run continuously*/
    while(1)
    {
		
        printf("\n----------------WAITING FOR REQUEST-----------------\n");
        /*accept the connection*/
        address_length = sizeof(client_address);
        clients[slot] = accept(listen_fd,(struct sockaddr*)&client_address , &address_length);
      
		    /*after connection is established,*/
        if(clients[slot] < 0)
        {
          error("ERROR: accept()");
        }
        else
        {
		      /*create a child process*/
          child_pid = fork();
          if(child_pid == 0)
          {
            respond(slot,root_dir);
            exit(1);
          }
        }
         
		    /*wait for child to execute*/
    //    waitpid(child_pid,NULL,0);
        
		    /*close the connection*/
    		clients[slot] = -1;
        while(clients[slot] != -1)
        {
          slot = (slot + 1) % CONN_MAX;
        }

    }
    
    return 0;
}

/*function to start server*/
void start_server(char *port)
{
   printf("\n-------------------START SERVER-------------------------\n");
   int port_no = atoi(port);
   struct sockaddr_in server;
   bzero((char *)&server, sizeof(server));
   server.sin_family = AF_INET;
   server.sin_addr.s_addr = INADDR_ANY;
   server.sin_port = htons(port_no);

    if((listen_fd = socket(AF_INET,SOCK_STREAM, 0)) < 0)
    {
        printf("ERROR: CREATING SOCKET\n");
        exit(1);
    }
  	
	/*check if bind is success*/
    if((bind(listen_fd,(struct sockaddr *)&server,sizeof(server))) < 0)
    {
      printf("ERROR : BINDING\n");
      exit(1);
    }
 
  /*listen for incoming connections*/
  if(listen(listen_fd,1000000) != 0)
  {
    perror("ERROR: listen()");
    exit(1);
  }
}

/*client connection response*/
void respond(int n, char *root)
{
    char message[99999],message1[99999], *req_line[3], data_to_send[BYTES] , path[99999];
    int bytes_rcvd, bytes_read;
    
	/*file descriptor*/
	FILE *fd;

	/*set all the values of buffer to zero*/
    memset((void*)message, (int)'\0', 99999);

	/*receive bytes from the client*/
    bytes_rcvd = recv(clients[n],message,99999,0);
    printf("\n------------------COMMAND RECEIVED---------------------\n");
    strncpy(message1,message,strlen(message));
    /*check if bytes have been received correctly*/
    if(bytes_rcvd < 0)
    {
      fprintf(stderr,("ERROR: recv()\n"));
    }
    else if(bytes_rcvd == 0)
    {
      printf("\n--------------NO COMMAND RECEIVED---------------------\n");
    }
    else
    {
      printf("%s\n",message);
      req_line[0] = strtok(message," \t\n");			/*tokenize to get the METHOD*/
	  
	  /*check if method is GET or POST*/
      if((strncmp(req_line[0], "GET\0" , 4) != 0) && (strncmp(req_line[0], "POST\0" , 5) != 0))
      {
		  /*if command is not GET or POST, check for other valid methods*/
	      if((strncmp(req_line[0], "PUT\0" , 4) == 0) || (strncmp(req_line[0], "DELETE\0" , 7) == 0) ||(strncmp(req_line[0], "HEAD\0" , 5) == 0)|| (strncmp(req_line[0], "TRACE\0" , 6) == 0) || (strncmp(req_line[0], "PATCH\0" , 6) == 0) ||  (strncmp(req_line[0], "OPTIONS\0" , 8) == 0) || (strncmp(req_line[0], "CONNECT\0" , 8) == 0) )
	      {
		       /*send the error message for method not implemented and shut the connections*/	  
	         printf("error:\n%s\n\n",method_not_supported_1);
           send(clients[n], method_not_supported_1, strlen(method_not_supported_1), 0);  
           
           printf("\n-------------------SHUT DOWN THE CLIENT----------------\n");
           shutdown(clients[n], SHUT_RDWR);
           close(clients[n]);
           clients[n] = -1;
           return;
	      }
	      else
	      {
		   /*if no valid method request, then send bad request error*/
	       sprintf(bad_request_send,bad_request,"Invalid Request"); 
           printf("error:\n%s\n\n",bad_request_send);
           send(clients[n], bad_request_send, strlen(bad_request_send), 0);  
        
           printf("\n-------------------SHUT DOWN THE CLIENT----------------\n");
           shutdown(clients[n], SHUT_RDWR);
           close(clients[n]);
           clients[n] = -1;
           return;
	      }
      }		
      
	  /*get file name requested and the protocol*/
      req_line[1] = strtok(NULL, " \t");
      req_line[2] = strtok(NULL, " \t\n");
      

	  /*if protocol is not 1.1 or 1.0, send invalid protocol error*/ 
      if(strncmp(req_line[2], "HTTP/1.0" , 8) != 0 && strncmp(req_line[2], "HTTP/1.1" , 8) != 0)
      {
         sprintf(bad_request_send,bad_request,"invalid protocol");
         printf("error:\n%s\n\n",bad_request_send);
         
         printf("\n-------------------SHUT DOWN THE CLIENT----------------\n");
         send(clients[n], bad_request_send, strlen(bad_request_send), 0);  
         shutdown(clients[n], SHUT_RDWR);
         close(clients[n]);
         clients[n] = -1;
         return;
      }
      else
      {
		  /*if filename is one of the index files then send index.html*/
         if((strncmp(req_line[1], "/index.html\0", 11) == 0) || (strncmp(req_line[1], "/index.htm\0", 10) == 0) || (strncmp(req_line[1], "/index.ws\0",9) == 0) || (strncmp(req_line[1], "/\0", 2) == 0))
         {
           req_line[1] = "/index.html";
         }
		 
		 /*copy filename in a char array*/
         char format_n[10];
         strcpy(format_n,req_line[1]);
		 
		 /*get the extension from filename*/
         int extension_get_check ;
         extension_get_check = get_extension(req_line[1]);
         if(extension_get_check!=0)
         {
           
		       /*check for valid extension*/
    		   int extension_check = check_extension(extension);
           
		      /*if not a valid extension, then send error 500*/
		      if(extension_check == 0)
           {
             if(strncmp(req_line[2], "HTTP/1.1" , 8) == 0)
             {
               sprintf(extension_error_send,extension_error,1.1);
               printf("error:\n%s\n\n",extension_error_send);
               send(clients[n], extension_error_send, strlen(extension_error_send), 0);  
             }
             else if(strncmp(req_line[2], "HTTP/1.0" , 8) == 0)
             {
               sprintf(extension_error_send,extension_error,1.0);
               printf("error:\n%s\n\n",extension_error_send);
               send(clients[n], extension_error_send, strlen(extension_error_send), 0);  
             }

             printf("\n-------------------SHUT DOWN THE CLIENT----------------\n");
             shutdown(clients[n], SHUT_RDWR);
             close(clients[n]);
             clients[n] = -1;
             return;


           }
         }
		 
		    /*get the root path and append filename to it*/
         strcpy(path, root);
         strcpy(&path[strlen(root)], req_line[1]);
         
		     /*create a char array for header*/
		     char header1[100];
         char *header = NULL;
		 
         /*check if file is found*/
         if((fd = fopen(path, "r")) != NULL)
         {
           int file_length = 0;
           fseek(fd,0,SEEK_END);
           file_length = ftell(fd);
           fseek(fd,0,SEEK_SET);
           
		      /*check if requested method is GET or POST*/
		      /*Appropriately set the header*/
	        if(strncmp(req_line[0], "GET\0" , 4) == 0)
	        {
	          if(strncmp(req_line[2], "HTTP/1.1" , 8) == 0)
              {
                sprintf(header1,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n",extension,file_length);
              }
              else if(strncmp(req_line[2], "HTTP/1.0" , 8) == 0)
              {
                sprintf(header1,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n",extension,file_length);
              }
          }
	        else if(strncmp(req_line[0], "POST\0" , 5) == 0)
	        {
              /*To get the string to be printed in POST header*/
              char *post_data_check; 
              post_data_check = strstr(message1, "\r\n\r\n");
              post_data_check = post_data_check + 4;

              if(strncmp(req_line[2], "HTTP/1.1" , 8) == 0)
              {
                sprintf(header1,"HTTP/1.1 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n<h1>%s</h1>\r\n",extension,file_length,post_data_check);
              }
              else if(strncmp(req_line[2], "HTTP/1.0" , 8) == 0)
              {
                sprintf(header1,"HTTP/1.0 200 OK\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n<h1>%s</h1>\r\n",extension,file_length,post_data_check);
              }
          }	
        
    		  /*send the header to client*/
          printf("\nheader1 is \n%s\n",header1);
          send(clients[n], header1, strlen(header1), 0);
          
		      /*send the actual file*/
          while((bytes_read = fread(data_to_send,1,BYTES,fd)) > 0)
          {
             write(clients[n],data_to_send,bytes_read);
          }
		  
		      /*close the file*/
          fclose(fd);
         }
         else
         {

			/*if file is not found, then send error 404*/
            if(strncmp(req_line[2], "HTTP/1.1" , 8) == 0)
            {
              sprintf(error_not_found_send,error_not_found,1.1);
              printf("error:\n%s\n\n",error_not_found_send);
              send(clients[n], error_not_found_send, strlen(error_not_found_send), 0);  
            }
            else if(strncmp(req_line[2], "HTTP/1.0" , 8) == 0)
            {
              sprintf(error_not_found_send,error_not_found,1.0);
              printf("error:\n%s\n\n",error_not_found_send);
              send(clients[n], error_not_found_send, strlen(error_not_found_send), 0);  
          
            }
            
            printf("\n-------------------SHUT DOWN THE CLIENT----------------\n");
			      /*shutdown the client*/
            shutdown(clients[n], SHUT_RDWR);
            close(clients[n]);
            clients[n] = -1;
            return;

          }
        }
      }
      
      printf("\n-------------------SHUT DOWN THE CLIENT----------------\n");
	    /*shutdown the client*/
      shutdown(clients[n], SHUT_RDWR);
      close(clients[n]);
      clients[n] = -1;
      return;
}

/*function to get the port number*/
int get_port_number(char *buffer)
{
  char *port_number;
  int port_no;
  
  /*tokenize at every space*/
  port_number = strtok(buffer," ");
  port_number = strtok(NULL," ");
  port_no = atoi(port_number);
  return port_no;
}

/*function to get root directory*/
char* get_root_directory(char *buffer)
{
  char *root;
  /*tokenize at every space*/
  root = strtok(buffer,"\"");
  root = strtok(NULL, "\"");
  return root;
}

/*function to get the different index file types*/
index_val get_index_types(char *buffer)
{
  index_val indextype;
  char *index_file;
  index_file = strtok(buffer, " ");
  int i = 0;
  while(index_file != NULL)
  {
    index_file = strtok(NULL, " ");
    i++;
    if(i < 4)
    {
      indextype.index_array[i-1] = index_file; 
    }
  }

  return indextype;
}

/*This function is used to find length of a double linked list*/
uint32_t dll_length(struct node* node)
{
  uint32_t count=0;
  struct node* node1 = node;
  if(node1 == NULL)
  {
    return 0;
  }
  else if (node1 != NULL)
  {
    while(node1!=NULL)				/*increment count till last node*/
    {
      count++;
      node1 = node1->next;
    }
    return count;
  }
  else
  {
    return 0;
  }
}



/*This function is used to print double linked list[forward and backward]*/
void print_list(struct node *node)
{
    struct node *last;

    while (node != NULL)
    {
        last = node;
        node = node->next;
    }
}

/*This function adds a new node at given position*/
states position_insert(struct node** head_ref,uint32_t position_to_be_inserted,char* format_ext, char* format_desc)
{
  struct node *temp_node=*head_ref;
  struct node* node_prev=NULL;

  struct node *new_node=(struct node*)malloc(sizeof(struct node));
  if(new_node == NULL)
  {	
	  return FAIL;
  }

  /*Assign the data to the new node*/
  strcpy(new_node->format_extension ,format_ext);  
  strcpy(new_node->format_description, format_desc);

  /*find the length of linked list*/
  int length_dll= dll_length(temp_node); 	
  int count=0;


  if(temp_node == NULL)
  {
        *head_ref = new_node;
        new_node->next = NULL;
        new_node->prev = NULL;
        return SUCCESS;
  }
 
  if(position_to_be_inserted > length_dll)
  {
	  return FAIL;
  }
  else if(position_to_be_inserted==0)	//If position is 0, then new node is head 
  {					//Modify next of new node as current head and previous as NULL
     new_node->next = *head_ref;															
     new_node->prev = NULL;
     temp_node->prev = new_node;
     *head_ref = new_node;
     return SUCCESS;
  }
  else if(position_to_be_inserted< length_dll)
  {

    while(count!=(position_to_be_inserted))//traverse till the position the new node has to be added
    {						//modify the next and previous
      node_prev=temp_node;
      temp_node=temp_node->next;
      count++;
    }
    new_node->next=temp_node;
    new_node->prev=temp_node->prev;
    temp_node->prev=new_node;
    node_prev->next=new_node;
    return SUCCESS;
  }

  if(position_to_be_inserted == length_dll)							//Check if new node has to be added at last
 {    									//if yes then modify next of new node to NULL and previous to the current last node
      while(temp_node->next != NULL)
     {

         temp_node = temp_node->next;
     }
     new_node->prev = temp_node;
     new_node->next = NULL;
     temp_node->next = new_node;
     return SUCCESS;
  }
}

/*get the different types of extension and thier description*/
void get_format(char *buffer, int iter)
{
  char *format_ext = strtok(buffer," ");
  char format_name[10];
  char format_des[20];
  int i = 0;
  while(format_ext != NULL)
  {
    if(i == 0)
    {
      strcpy(format_name,format_ext);
    }
    else
    {
      strcpy(format_des,format_ext);
    }

    format_ext = strtok(NULL, " ");
    i++;
  }
  position_insert(&head,1,format_name,format_des);

}

/*function to get the extension from the filename*/
int get_extension(char *format_name)
{
  int i =0;
  int length = strlen(format_name);
  
  /*start from backwards and find .*/
  while(format_name[length] != '.' && length != 0)
  {
    length--;
  }

  if(length == 0)
  {
    return 0;
  }

  /*get the extension*/
  while(format_name[length] != '\0')
  {
      extension[i] = format_name[length];
      length++;
      i++;
  }

  /*NULL terminate the extension*/
  extension[i+1] = '\0';
  return 1;
}

/*function to check if the extension is a valid one*/
int check_extension(char *ext)
{
  struct node *node1;
  node1 = head;
  int check_flag = 0;

  /*traverse the linked list and set a flag if extension exists*/
  while(node1 !=NULL)
  {
    if(strcmp(ext,node1->format_extension) == 0)
    {
      check_flag++;
    }

    node1 = node1->next;
  }

  return check_flag;
}
