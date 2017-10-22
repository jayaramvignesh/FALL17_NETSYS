TCP webserver application

source file: webserver.c
execution: ./webserver
No arguments passed from command line

HTTP Methods Implemented:

1. GET: This method is used to get the differnet files and display it on webserver. This method is tested by starting firefox & typing localhost:portno.After that different files can be obtained by writing /filename after port number.The apporopriate header is sent first before the file is sent to the client.

2. POST: This method is used to POST differnet files on webserver.POST is tested using telnet and command line. The appropriate header is sent first before the file is posted.

All the other methods are not implemented.

ERROR METHODS:

Different Error methods have been implmented:

ERROR 400: this error displays an invalid request when a wrong protocol is entered like HTTP/2,1 or when mehtod name requested is incorrect i.e POSY.

ERROR 404: This error displays a FILE/URL not found request when a file is requested which does not exist

ERROR 500: This error displays an invalid extension request when the extension of file requested does not match the stored extensions.

ERROR 501: This error displays a method not implemented error when a request for any other method other than GET/POST is made.
