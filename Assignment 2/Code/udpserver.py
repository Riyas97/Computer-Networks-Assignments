# Tested with Python 3.8.5 in WSL (Windows) and Python 3.8.5 in Ubuntu 20.04

import errno
import os
import signal
import socket
from urllib.parse import unquote
import urllib

SERVER_ADDRESS = (HOST, PORT) = 'localhost', 10000

# setting up SIGCHLD event handler with waitpid system instead of wait 
# and with a WNOHANG option 
# ensures that all terminated child processes are taken care of
# if not, signals are not queued and the server can miss several signals
def grim_reaper(signum, frame):
    while True:
        try:
            pid, status = os.waitpid(
                -1,          # wait for any child process
                 os.WNOHANG  # do not block and return EWOULDBLOCK error
            )
        except OSError:
            return

        if pid == 0: 
            return


def request_handler(client_connection, addr):

    http_response = "EE-4210: Continuous assessment"
         
    # constructing http header
    length = len(http_response)
    status = "200 OK"
    ctype = "text/plain"
    response = "HTTP/1.1 {status}\r\nContent-Length: {length}\r\n\r\n".format(
        status = status,
        length = length
    )
    response_encoded = response.encode() + http_response.encode()
    #print (response_encoded)
    #print ("check1")
    client_connection.sendto(response_encoded, addr)
    print ("http response sent\n")
    #print ("check2")


if __name__ == '__main__':
    # create a UDP socket
    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    # to re-use the same address again 
    udp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # to assign a local protocol address to the socket
    # specify a port number and an IP address
    udp_socket.bind(SERVER_ADDRESS)
    print('Serving HTTP on port {port} ...'.format(port=PORT))

    # set up SIGCHLD event handler
    signal.signal(signal.SIGCHLD, grim_reaper)

    while True:
        
        data, addr = udp_socket.recvfrom(1024)
        # fork for concurrency
        pid = os.fork()
        # child process
        if pid == 0:
            # handle the request
            print ("http request received\n")
            print (data)
            request_handler(udp_socket, addr)
            os._exit(0)
        