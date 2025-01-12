#include <stdio.h>
#include <zmq.h>
#include <string.h>

int main() {
    void *context = zmq_ctx_new();
    void *socket_recv = zmq_socket(context, ZMQ_PULL);
    zmq_connect(socket_recv, "ipc:///tmp/zmq_ipc");

    void *socket_send = zmq_socket(context, ZMQ_PUSH);
    zmq_bind(socket_send, "ipc:///tmp/zmq_ipc");

    char message[16];
    while (1) { // Endloses Empfangen und Rücksenden
        zmq_recv(socket_recv, message, sizeof(message), 0);
        zmq_send(socket_send, message, sizeof(message), 0);
    }

    zmq_close(socket_recv);
    zmq_close(socket_send);
    zmq_ctx_destroy(context);
    return 0;
}
