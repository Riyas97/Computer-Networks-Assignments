# Tested with Python 3.8.5 in WSL (Windows) and Python 3.8.5 in Ubuntu 20.04

import errno
import os
from urllib.parse import unquote
import urllib
import signal
import socket

SERVER_ADDRESS = (HOST, PORT) = '', 8789
REQUEST_QUEUE_SIZE = 4096

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


def request_handler(client_connection):
    encodedRequest = client_connection.recv(4096)
    request = encodedRequest.decode()
    print ("serving request\n")
    print(request)

    # Parse HTTP (basic parser)
    lines = request.splitlines()

    http_response = """
<!DOCTYPE html>
<html>
  <head>
    <meta charset="utf-8">
    <title>PHP form submission example</title>
    <style>
      form {
        width: 500px;
      }

      div {
        margin-bottom: 20px;
      }

      label {
        display: inline-block;
        width: 240px;
        text-align: right;
        padding-right: 10px;
      }

      button, input {
        float: right;
      }
    </style>
  </head>
  <body>
    <form method="post" >
      <div>
        <label for="say">Enter text here:</label>
        <input name="say" id="say" value="">
      </div>
      <div>
        <button>Submit</button>
      </div>
    </form>
  </body>
</html>
"""

    for line in lines:
        if ("say=" in line):
            http_response = "You typed: " + urllib.parse.unquote_plus(line)[4:]
            
    # constructing http header
    length = len(http_response)
    status = "200 OK"
    response = "HTTP/1.1 {status}\r\nContent-Length: {length}\r\n\r\n".format(
        status = status,
        length = length
    )
    response_encoded = response.encode() + http_response.encode()
    client_connection.send(response_encoded)
    print ("http response sent\n")


if __name__ == '__main__':
    # create a TCP/IP socket
    tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # to re-use the same address again 
    tcp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    # to assign a local protocol address to the socket
    # specify a port number and an IP address
    tcp_socket.bind(SERVER_ADDRESS)
    # listening socket
    tcp_socket.listen(REQUEST_QUEUE_SIZE)
    print('Serving HTTP on port {port} ...'.format(port=PORT))

    # set up SIGCHLD event handler
    signal.signal(signal.SIGCHLD, grim_reaper)

    while True:
        try:
            # accept client connection
            client_connection, client_address = tcp_socket.accept()
        except IOError as e:
            code, message = e.args
            if code == errno.EINTR:
                continue
            else:
                raise

        # fork for concurrency
        pid = os.fork()
        # child process
        if pid == 0:
            # close the child copy
            tcp_socket.close()
            # print("test4")
            # handle the request
            request_handler(client_connection)
            # print("test5")
            client_connection.close()
            os._exit(0)
        else: 
            # parent process
            # close the parent copy
            client_connection.close()