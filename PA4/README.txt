Programming Assignment 4

	Josh Rahm
	Zach Anders

Summary:
	We have completed the assignment to specification:
		- Clients can connect via TCP
		- Clients interact with peers for file transfers
		- Clients support commands 'ls', 'get', and 'exit'/Ctrl-C
		- Master list is updated by new clients and can be queried by clients

Build Instructions:
	$ make

Run Instructions:
	# Start server
	./server <port>

	# Start clients. (Each in separate directories)

	cd <client1dir>
	../client <clientname> <host> <port>

	cd <client2dir>
	../client <clientname> <host> <port>

	cd <client3dir>
	../client <clientname> <host> <port>
