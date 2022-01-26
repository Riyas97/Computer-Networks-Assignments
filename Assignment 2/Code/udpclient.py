# Tested with Python 3.8.5 in WSL (Windows) and Python 3.8.5 in Ubuntu 20.04

import argparse
import errno
import os
import socket


SERVER_ADDRESS = 'localhost', 10000
REQUEST = b"""\
GET /hello HTTP/1.1
Host: localhost:10000
"""

def test(max_clients, max_conns):
    socks = []
    for client_num in range(max_clients):
        print ("new client")
        pid = os.fork()
        if pid == 0:
            for connection_num in range(max_conns):
                sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
                print ("new connection")
                sent = sock.sendto(REQUEST, SERVER_ADDRESS)
                print ("http request sent")
                print('connection number is: {con_num}'.format(con_num=connection_num))
                # receive response
                data, server = sock.recvfrom(1024)
                print('received response is {response}'.format(response=data))
                os._exit(0)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(
        description='Test client for multiple clients concurrency',
        formatter_class=argparse.ArgumentDefaultsHelpFormatter,
    )
    parser.add_argument(
        '--clients',
        type=int,
        default=1,
        help='Max number of clients.'
    )
    parser.add_argument(
        '--conns',
        type=int,
        default=1024,
        help='Max number of connections per client.'
    )
    args = parser.parse_args()
    print ('no of clients is: {client}'.format(client=args.clients))
    print ('no of connections per client is: {con}'.format(con=args.conns))
    test(args.clients, args.conns)