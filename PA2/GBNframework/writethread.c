#include "gbn_socket.h"

void gbn_socket_write_thread_main( gbn_socket_t* sock ) {
    while ( true ) {
        if( sock->_m_sending_window._m_size == 0 ) {
            /* The sending window is empty */
        }
    }
}
