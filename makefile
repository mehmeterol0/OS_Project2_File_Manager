all: file_manager file_client

file_manager:	file_manager.c
	        gcc -pthread file_manager.c -o file_manager
	        
file_client:	file_client.c
		gcc -pthread file_client.c -o file_client
