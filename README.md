# cs_project  

### Important note
For encryption we use the OpenSSL library which is required to run our code, see https://www.openssl.org/.

### Compile server.c  

In `Server/`:  
`$ gcc cJSON.c server.c -o server -lssl -lcrypto`  

### Compile client.c  

In `Client/`:  
`$ gcc client.c -o client`  

### Execute

In the corresponding folder, run first the server using \\
`$ ./server`  \\
and then the client using \\
`$ ./client`  
