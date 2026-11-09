#include <netinet/in.h> /* sockaddr_in, INADDR_ANY, htons() */
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h> /* socket(), bind(), listen(), accpet() */                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                 #include<sys/socket.h> /* socket(), bind(), listen(), accpet() */
#include <unistd.h>

//HTTP ==> Hypertext Transfer Protocol defines the rule for web communication
/*In http request router it have two jobs, Splitting and identifying is the
first half what browsers sends us The router's main job is the second half:
deciding which code runs for what is found Browser Sends: GET /about HTTP/1.1
1. Parse: method = "GET", path = "/about", version = "HTTP/1.1"
2. Route: look up (GET, /about) -> find the matching handler
3. Handler: runs, builds the response (status, headers, body)
4. Send: write the response back to the client  */

const char* CRLF = "\r\n";
const char* SP = " ";

typedef struct {
  char* method;
  char* request;
  char* version;
} http_req_line;

typedef enum { HTTP_RES_OK, HTTP_RES_INTERNAL_SERVER_ERR } http_result;

typedef struct {
  const char* start;
  size_t len;
} string_view;

typedef struct {
  string_view* splits;
  size_t capacity;
  size_t count;
} string_splits;

static string_splits split_string(const char* str, size_t len, char split_by) {
  string_splits result;
  size_t result_i = 0;
  const char* start = str;

  result.splits = calloc(sizeof(string_view), 8);
  result.count = 0;
  result.capacity = 8;

  for (size_t i = 0; i < len; ++i) {
    if (str[i] == split_by) {
      result.splits[result_i].start = start;
      result.splits[result_i].len = &str[i] - start;
      result.count += 1;
      result_i += 1;
      start = &str[i + 1];

      if (result.count == result.capacity) {
        result.capacity *= 2;

        /*realloc(a, b) basically means increase size of a to size b
        what it does internally:
        1> Room to grow in place: If there is free space right after the block,
        it extends the block and returns the same address 2> NO room: It
        allocates a new, bigger block elsewhere, copies the old contents over,
        frees the old block, and returns the new address 3> Shrinking: It
        usually trims the block in place and returns the same pointer. We don't
        know in advance what will happen so always use the returned pointer, and
        never keep using the old one */

        string_view* temp =
            realloc(result.splits, sizeof(string_view) * result.capacity);

        if (temp)
          result.splits = temp;
        else {
          perror("realloc()");
          abort();
        }
      }
    }
  }
  result.splits[result.count].start = start;
  result.splits[result.count].len = str + len - start;
  result.count += 1;
  return result;
}

static void free_splits(string_splits* splits) {
  if (splits) {
    free(splits->splits);
  }
}

http_req_line http_req_line_init(void) {
  http_req_line line;
  line.method = NULL;
  line.request = NULL;
  line.version = NULL;
  return line;
}

http_result parse_req_line(const char* buf, size_t len,
                           http_req_line* req_line) {
  if (!buf || !req_line) {
    return HTTP_RES_INTERNAL_SERVER_ERR;
  }
  req_line->method = "GET";
  req_line->version = "HTTP/1.0";
  return HTTP_RES_OK;
}

int handle_client(int client_socket) {
  ssize_t n = 0;
  char buffer[1024];

  /* a browser or curl speaks HTTP, a text protocol layered on TCP. it expects a
  reply in a specific shape HTTP/1.0 200 OK <- status line: version, status
  code, reason
                  <- header would go here
                  <- blank line = end of headers
  Hello, world!   <- the body
  HTTP/1.0 is the protocal version
  200 OK is the status code (200 = success; 404 = not found)
  \r\n is a line ending in HTTP
  Everything after that is the body the browser displays*/

  const char* hello = "HTTP/1.0 200 OK\r\n\r\n<h1>Hello, Anita Rawat</h1>";

  printf("\n----\n");
  for (;;) {
    memset(buffer, 0, sizeof(buffer));

    /*It copies bytes from a file descriptor into our program's memory. In my
    server it recieves the browser's request from the client socket it takes
    three arguments: 1> fd: the file descriptor to read from 2> buf: where to
    store the bytes 3> count: the maximum number of bytes to read, which should
    never be largert than the space in buf Internally : Data from the browser
    arrives in the kernel's receive buffer for that socket. read copies bytes
    out of it into my memory. Return value: positive value how many bytes were
    read. It can be less than count, and it can be less than the full request.
    TCP is a byte stream with no message boundaries, so one read may return half
    a request, or parts of two. 0 the other side closed the connection. -1 an
    error, with the reason in errno. EINTR means a signal interrupted the call
    and you can retry  */

    n = read(client_socket, buffer, sizeof(buffer) - 1);

    if (n < 0) {
      perror("read(client)");
      return -1;
    }
    if (n == 0) {
      printf("connection closed gracefully\n");
      return 0;
    }
    printf("REQUEST:\n%s", buffer);

    /*write send bytes from your program memory to a file descriptor, the bytes
    travel from over the network to the browser the three arguments: 1> fd: the
    file descriptor to write to (client_socket in my code) 2> buf: a pointer to
    the data to send 3> count: how many bytes to send from buffer
    write(client_socket, hello, strlen(hello));
    This sends the first strlen(hello) bytes of the string hello to the browser.
    We pass the length ourself because write works on raw bytes and doesn't look
    for \0 terminator, strlen doesn't count the terminator Write doesn't send
    anything directly to the browser. It copies bytes into the kernel's send
    buffer for that socket and returns. The kernel then handles TCP: splitting
    the data into packets, sending them, and retransmitting any that get lost.
    So when write returns, the data is queued, no necessarily delivered It
    returns a positive number how many bytes were accepted, it can be less than
    the count. It's called a short write and it happens with large data or a
    full buffer. -1 means an error, with the reason in errno. For example EPIPE
    means the client already disconnected, and EBADF means a bad descriptor */

    (void)write(client_socket, hello, strlen(hello));

    /*This ends the conversation with that client and release the file
    descriptor. Two reason it matters here:
    1. It tells the client the response if finished
    2. It frees the descriptor */

    close(client_socket);

    break;
  }
  printf("\n----\n");
  return 0;
}

int main(void) {
  const char* subject = "hello ";
  string_splits splits = split_string(subject, strlen(subject), ' ');

  for (size_t i = 0; i < splits.count; ++i) {
    printf("split %zu: '%.*s'\n", i, (int)(splits.splits[i].len),
           splits.splits[i].start);
  }
  return 0;

  int ret = 0;
  int enabled = true;

  /*returns a file discriptor that the os gives our program to refer to
  something it has opened, like a ticket number for the open resource */

  int tcp_server = socket(AF_INET, SOCK_STREAM, 0);
  if (tcp_server == -1) {
    perror("socket()");
    return 1;
  }
  printf("Socket Creation Successfull\n");

  /*It changes a setting on the socket before bind, SO_REUSEADDR lets you bind
  to a prot that was recently used The Arguements:
  1. SOL_SOCKET: the option applies at the socket level
  2. SO_REUSEADDR: which option (allow reusing the address
  3. &enabled, sizeof(enabled): the value and its size.*/

  setsockopt(tcp_server, SOL_SOCKET, SO_REUSEADDR, &enabled, sizeof(enabled));

  struct sockaddr_in addr;
  memset(&addr, 0, sizeof(addr));

  /* act as a label tell what type of network we are using here we are using IP4
   */

  addr.sin_family = AF_INET;

  /* can connect to my laptop either by wifi, ethernet or localhost(127.0.0.0)
   */

  addr.sin_addr.s_addr = INADDR_ANY;

  /* sets the port number your server will listen on, a port find the right
   program on that machine. One laptop can run many programs at once, the port
   tells the OS which one an incoming connection is for. Port go grom 0 to 65535
   (16 bit which is why sin_port uses uint16_t) 0 to 1023 are reserved (80 =
   web, 22 = SSH) Binding to them needs root. 1024 and above are free for your
   own program, 8060 is fine, and so is 8080 (htons means host to network short:
   it converts a 16-bit number from your computer's byte order to network byte
   order. There are two ways to order them <1. little-endian: least significant
   byte first. 2. Big-endian: most significant bit first. */

  addr.sin_port = htons(8080);

  /*it attaches socket to a specific ip address and port, so the
  operating system knows that connections arriving at that address should go to
  this socket. Before, bind the socket exists but has no address. return value 0
  on success and - 1 on failure */

  int checkbind = bind(tcp_server, (struct sockaddr*)&addr, sizeof(addr));
  if (checkbind < 0) {
    perror("bind()");
    ret = 1;
    goto exit;
  }
  printf("Bind successfully\n");

  /* listen switches your socket into listening mode, so it can accept incoming
   conncections Before this call, the socket is bound to an address but isn't
   waiting for anyone. After it, the OS starts accepting conncection requests on
   your port . it takes two argument 1> tcp_server: the socket you created and
   bound. 2> 5: the backlog, which is the size of the waiting queue for
   connections. return 0 on success and -1 on failure */

  int checklisten = listen(tcp_server, 5);
  if (checklisten < 0) {
    perror("listen()");
    ret = 1;
    goto exit;
  }
  printf("Listen successfully\n");

  /*accpet takes the next waiting client fromt the queue and gives you a new
  socket for talking to that client.. tcp_server: the listening socket, First
  NULL: where to store the client address, NULL means "I dont care who it is"
  Second NULL: a pointer to the size of that adddress buffer. */

  for (;;) {
    printf("Waiting for connection...\n");
    int client_socket = accept(tcp_server, NULL, NULL);

    printf("got a connection!\n");
    handle_client(client_socket);
  }

exit:
// 
  close(tcp_server);
  return ret;
}
