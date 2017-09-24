PROGRAMMING ASSIGNMENT: UDP FILESERVER IN C

The assignment when compiled runs a UDP fileserver. There are two .c files.

UDP fileserver executes 5 commands:

-> ls <filename>
In this command, the client requests the server to list all the files present in the server directory and send it to the client over the socket. Server runs "ls" using system command redirects the output to the filename. Then server reads the contents of the file ands sends them over to the client. Client then stores the contents under the filename.

-> put <filename> 
In this command, the client wants to send over the contents of a file to server and store it on the server side. First the client checks if file is present. Then using command line system call "sha256sum" the hashed value of the file is calculated and sent to the server. Then using XOR  the file is encrypted and stored in a new file. The client then sends the encrypted file over the socket in packets of fixed length. After the file has been received, the server then decrypts the file. After the file is decrypted, server calculates the  hash value of file received. Then both the hash values are matched for file integrity check.

-> get <filename> 
In this command, the client requests server to send over the contents of a file present on the server side. First the server checks if file is present and sends a corresponding yes/no to the client. Then using command line system call "sha256sum", the hashed value of the file is calculated and sent to the client. Then using XOR the file is encrypted and stored in a new file. The server then sends the enccrypted file over the socket in packets of fixed length. After the file has been received, the client then decrypts the file. After the file is decrypted, client calculates the hashed value of file received. Then both the hash values are matched for file integrity check.

-> delete <filename>
In this command, the client requests the server to delete a particular file in the server directory. The server checks if file is present and sends corresponding yes/no to the client. If file is present, then it is deleted usiing remove command.

-> exit
Using this command, both client and server exit the connection using exit(1).

RELIABILITY:

Stop and wait protocol has been used for reliability. There is always only one packet in the transit.The sender sends the packet and waits for ack/nack from the receiver. Every alternate packet is appended using 1 or 2 to check if the correct packet is received on the receiver side. If the correct packet is received then the receiver sends an ack(i.e received packet number is sent) else nack. Timeout has been implemented to take care of packet loss or ack loss. in case of ack loss, if the receiver receives the old packet twice, then it resends the ack for the previously received packet.

file integrity is checked using "sha256sum" system command.
