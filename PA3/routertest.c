#include "router.h"
#include <stdint.h>
#include <assert.h>
#include "debugprint.h"


void packet_update_test() {
	ls_packet_t t1, t2;
	t1.origin = 'A';
	t1.seq_num = 2;

	
	t2.origin = 'A';
	t2.seq_num = 2;

	assert(!packet_has_update(&t1, &t2));

	t2.seq_num = 3;
	t2.num_entries = 1;
	t2.dest_id[0] = 'B';
	t2.cost[0] = 5;
	assert(packet_has_update(&t1, &t2));
	debug3("Completed packet update tests successfully!\n");
}

int main(int argc, char* argv[]) {
	packet_update_test();
}
