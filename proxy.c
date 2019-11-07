#include <stdio.h>
#include <limits.h>
#include "csapp.h"
#include "cache.h"

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr =
"User-Agent: curl/7.54.0\r\n";

static const char *USAGE = "usage: proxy <port number>";

typedef struct {
    char *uri;
    char *hostname;
    char *port;
    char *path;
    char *method;
} proxy_request_t;


void free_proxy_request(proxy_request_t *pr);
int parse_uri(proxy_request_t *u);
char *build_request(proxy_request_t *u, char *headers[20]);
int forward_request(int sendfd, proxy_request_t *uri, char *request);

int main(int argc, char *argv[])
{
    // test_cache();
    // return 0;
    
    if (argc != 2) {
        printf("exactly one argument must be given\n");
        printf("%s\n", USAGE);
        return 1;
    }

    init_cache();
    while (1) {
        socklen_t len = sizeof(struct sockaddr_storage);
        struct sockaddr_storage addr;

        int listenfd = open_listenfd(argv[1]);
        int connfd;
        if ((connfd = accept(listenfd, (SA *)&addr, &len)) < 0) {
            printf("faliure listending on port %s, retrying\n", argv[1]);
            continue;
        }

        // char client_hostname[MAXLINE], client_port[MAXLINE];
        // getnameinfo((SA *) &addr, len, client_hostname, MAXLINE,
        //             client_port, MAXLINE, 0);
        // printf("Connected to (%s, %s)\n", client_hostname, client_port);
        // "GET http://www.google.com:"
        // ""
        rio_t rio;
        rio_readinitb(&rio, connfd);

        char req_line[MAXLINE];
        char *request[100];
        for (int i = 0; i < 20; i++) // initialize
            request[i] = (char *)malloc(1000);
        // store request in array
        size_t n;
        printf("recieving proxy request:\n");
        for (int i = 0; (n = rio_readlineb(&rio, req_line, MAXLINE)) != 0; i++) {
            if (!strcmp(req_line, "\r\n"))
                break;
            printf("%s", req_line);
            strcpy(request[i], req_line);
        }
        printf("\n");

        char method[100];
        char uri[100];
        char version[100];

        sscanf(request[0], "%s %s %s", method, uri, version);
        proxy_request_t u = {NULL, NULL, NULL, NULL};
        u.uri = uri;
        u.method = method;
        if (parse_uri(&u) != 0) { // error in proxy request
            close(listenfd);
            close(connfd);
            continue;
        }

        char *req = build_request(&u, request);

        int fr = forward_request(connfd, &u, req);

        free_proxy_request(&u);
        free(req);
        close(listenfd);
        close(connfd);
        // break;
    }

    free_cache();
    return 0;
}

void free_proxy_request(proxy_request_t *pr) {
    free(pr->hostname);
    free(pr->port);
    free(pr->path);
}

int forward_request(int sendfd, proxy_request_t *uri, char *request) {
    rio_t send_rio;
    rio_readinitb(&send_rio, sendfd);

    int cache_index = -1;
    printf("looking up '%s' in cache\n", uri->uri);
    cache_index = cache_lookup(uri->uri);
    printf("result: %d\n", cache_index);
    if (cache_index != -1) {
        printf("sending from cache...\n");
        rio_writen(sendfd, cache[cache_index].value, strlen(cache[cache_index].value));
        return 0;
    }


    int clientfd = open_clientfd(uri->hostname, uri->port);

    rio_t rio;
    rio_readinitb(&rio, clientfd);

    printf("sending request to %s on port %s:\n%s", uri->hostname, uri->port, request);
    rio_writen(clientfd, request, strlen(request));


    char res_line[MAXLINE];
    char *output = (char *)malloc(MAX_OBJECT_SIZE);
    size_t n;
    // while ((n = rio_readlineb(&rio, res_line, MAXLINE)) != 0)
    //     rio_writen(sendfd, res_line, strlen(res_line));
        // printf("%s", res_line);

    strcpy(output, "\0");
    while ((n = rio_readlineb(&rio, res_line, MAXLINE)) != 0)
        strcat(output, res_line);

    rio_writen(sendfd, output, strlen(output));


    printf("inserting key: %s\n", uri->uri);
    cache_insert(uri->uri, output);

    free(output);
    close(clientfd);

    return 0;
}

int parse_uri(proxy_request_t *u) {
    // char *hostname = (char *)malloc(100);
    // char *port = (char *)malloc(100);
    // char *path = (char *)malloc(100);
    char hostname[100];
    char port[100];
    char path[100];

    int requires_port = 0;

    char protocall_test[8];
    for (int i = 0; i < 8; i++)
        protocall_test[i] = u->uri[i];
    protocall_test[7] = '\0';

    // we do not support non-http protocalls
    if (strcmp(protocall_test, "http://")) {
        printf("error in proxy request:\n");
        printf("non-http protocall used :: uri must begin with http://\n");
        return 1;
    }

    if (strcmp(u->method, "GET")) {
        printf("error in proxy request:\n");
        printf("method: (%s) not supported\n", u->method);
        return 1;
    }

    // parse hostname
    int index = 7;
    for (int i = 0; i < strlen(u->uri); i++) {
        if (u->uri[index] == ':') { // there is a port asccosiated
            requires_port = 1;
            hostname[i] = '\0';
            break;
        } else if (u->uri[index] == '/') { // there is no port, next is file name
            hostname[i] = '\0';
            break;
        }

        hostname[i] = u->uri[index];
        index++;
    }

    // parse port
    if (requires_port) {
        index++; // seek to after :
        for (int i = 0; i < (strlen(u->uri) - strlen(hostname)); i++) {
            if (u->uri[index] == '/') { // end of port
                port[i] = '\0';
                break;
            }
            port[i] = u->uri[index];
            index++;
        }
    } else {
        // port = "80";
        strcpy(port, "80");
    }

    // parse path
    int i = 1;
    index++; //move past path
    path[0] = '/';
    while (u->uri[index] != '\0') {
        path[i] = u->uri[index];
        index++;
        i++;
    }
    path[index] = '\0';

    u->hostname = (char *)malloc(100);
    u->port = (char *)malloc(100);
    u->path = (char *)malloc(100);

    strcpy(u->hostname, hostname);
    strcpy(u->port, port);
    strcpy(u->path, path);

    // u->hostname = hostname;
    // u->port = port;
    // u->path = path;

    return 0;
}

char *build_request(proxy_request_t *u, char *headers[20]) {
    const char *version = "HTTP/1.0";
    char *request = (char *)malloc(1000);
    // initial line
    sprintf(request, "%s %s %s\r\n", u->method, u->path, version);

    // headers
    // when these are included.... cs.coloradocollege.edu does not work
    // .. works for all others tested
    strcat(request, "Connection: Close\r\n");
    strcat(request, "Proxy-Connection: Close\r\n");
    // strcat(request, user_agent_hdr);

    char key[1000];
    for (int i = 1; i < 20; i++) { // ignore first line
        if (headers[i][0] == 0)
            break;
        sscanf(headers[i], "%s", key);
        if (!strcmp(key, "Proxy-Connection:"))
            continue;
        if (!strcmp(key, "User-Agent:"))
            continue;
        if (!strcmp(key, "Connection:"))
            continue;
        else {
            // format original headers
            size_t len = strlen(headers[i]);
            headers[i][len - 1] = '\r';
            headers[i][len] = '\n';
            headers[i][len + 1] = '\0';
            strcat(request, headers[i]);
        }
    }

    strcat(request, "\r\n");
    return request;
}
