Programming Assignment 3

Authors:
	Joshua Rahm
	Zach Anders

Compilation:
	$ make all

Execution:
	./routed_LS <RouterID> <LogFileName> <Initialization file>

Notes:
	We have successfully implemented all of the Link state requirements
	in the PDF. Our routers can successfully communicate and converge to a proper forwarding
	table. It initializes sockets to immediate neighbors as specified in the readme, and
	when all its neighbors come up it begins broadcasting and accepting link state packets.
	Using these packets, it constructs a forwarding table.
	Key values:
		- LSP timeout: LSP packets are sent every 5 seconds.
