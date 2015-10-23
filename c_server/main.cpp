/*
    C webserver with DDOS protection.
*/

#include<netinet/in.h>
#include<stdio.h>
#include<stdlib.h>
#include<sys/socket.h>
#include<sys/stat.h>
#include<sys/types.h>
#include<unistd.h>
#include<pthread.h>
#include <map>
#include <signal.h>
#include<iostream>
#include <unistd.h>
#include <termios.h>

#define MULTITHREADED

int thread_count = 0;

// Structure hold in the connection database for each client
typedef struct {
    int    count;
    time_t first_connect;
    pthread_mutex_t mutex;
} con_struct;

// count running threads for gracefull exit
int running_threads = 0;
pthread_mutex_t    running_threads_mutex = PTHREAD_MUTEX_INITIALIZER;

// Flag to stop execution from keyboard
int stop = 0;

// Mutex for global operations on con_map.
pthread_rwlock_t   con_map_rw           = PTHREAD_RWLOCK_INITIALIZER;

// Mutex to create new entries in map
pthread_mutex_t    entry_creation_mutex = PTHREAD_MUTEX_INITIALIZER;
std::map<int,con_struct*> con_map;

int connect_socket = -1;

// Multiplatform get character
char getch() {
    char buf = 0;
    struct termios old = {0};
    if (tcgetattr(0, &old) < 0)
        perror("tcsetattr()");
    old.c_lflag &= ~ICANON;
    old.c_lflag &= ~ECHO;
    old.c_cc[VMIN] = 1;
    old.c_cc[VTIME] = 0;
    if (tcsetattr(0, TCSANOW, &old) < 0)
        perror("tcsetattr ICANON");
    if (read(0, &buf, 1) < 0)
        perror ("read()");
    old.c_lflag |= ICANON;
    old.c_lflag |= ECHO;
    if (tcsetattr(0, TCSADRAIN, &old) < 0)
        perror ("tcsetattr ~ICANON");
    return (buf);
}

void *keyboard_handler(void *nothing) {
    printf("Keyboard listening\n");
    getch();
    printf("Keyboard stop\n");
    if (connect_socket != -1) {
        // This will cause abort on the "connect_socket" accept.
        close(connect_socket);
        connect_socket = -1;
        printf("Socket stop\n");
    }
    stop = 1;
    return NULL;
}

void clean_up() {
    printf("calling periodic clean up\n");
    // Grab write mutex on the MAP
    pthread_rwlock_wrlock(&con_map_rw);
    std::map<int,con_struct*>::iterator it=con_map.begin();
    while (it != con_map.end()) {
        if (time(NULL) - it->second->first_connect > 5) {
            std::map<int,con_struct*>::iterator it2=it;
            it++;
            delete it2->second;
            con_map.erase(it2);
        } else {
            it++;
        }
    }
    pthread_rwlock_unlock(&con_map_rw);
}

int ddos_check(int client_id) {
    // Grab READ mutex on the MAP on every access
    pthread_rwlock_rdlock(&con_map_rw);
    std::map<int,con_struct*>::iterator it;
    it = con_map.find(client_id);
    if (it != con_map.end()) {
        pthread_mutex_lock(&it->second->mutex);
        // Yes, this is not very acurate to use seconds, but for excersise ok )
        if (time(NULL) - it->second->first_connect > 5) {
            it->second->count = 1;
            it->second->first_connect = time(NULL);
        } else {
            con_map[client_id]->count++;
            if (it->second->count > 5) {
                pthread_rwlock_unlock(&con_map_rw);
                pthread_mutex_unlock(&it->second->mutex);
                return 0;
            }
        }
        pthread_mutex_unlock(&it->second->mutex);
    } else { // Create new map entry
        // Grab entry_creation_mutex and test again that not created in mean-time
        pthread_mutex_lock(&entry_creation_mutex);
        it = con_map.find(client_id);
        if (it != con_map.end()) {
            // One level recursion to save typing in VERY rare cases
            pthread_mutex_unlock(&entry_creation_mutex);
            pthread_rwlock_unlock(&con_map_rw);
            return ddos_check(client_id);
        }
        con_struct* cs = new con_struct;
        cs->count = 1;
        cs->first_connect = time(NULL);
        pthread_mutex_init(&cs->mutex, NULL);
        con_map[client_id] = cs;
        pthread_mutex_unlock(&entry_creation_mutex);
    }
    pthread_rwlock_unlock(&con_map_rw);
    return 1;
}



void *connection_reply(int sock) {
    //Get the socket descriptor
    int read_size;
    int bufsize = 4000;
    char *buffer = (char *)malloc(bufsize);
    char *request_tail = (char *)malloc(bufsize);
    int client_id = -1;
    recv(sock, buffer, bufsize, 0);
    sscanf(buffer + 16, "%d%s",&client_id,request_tail);

    if (ddos_check(client_id)) {
        //first send the header
        write(sock, "HTTP/1.1 200 OK\n", 16);
        write(sock, "Content-length: 46\n", 19);
        write(sock, "Content-Type: text/html\n\n", 25);
        write(sock, "<html><body><H1>Hello world</H1></body></html>",46);
    } else {
        write(sock, "HTTP/1.1 503 Service Unavailable\n",
                        strlen("HTTP/1.1 503 Service Unavailable\n"));
        write(sock, "Content-length: 46\n", 19);
        write(sock, "Content-Type: text/html\n\n", 25);
        write(sock, "<html><body><H1>Unavailable</H1></body></html>",46);
    }
    fsync(sock);
    free(buffer);
    free(request_tail);
    close(sock);
    return 0;
}

/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc) {
    int sock = (int)(long)socket_desc;
    connection_reply(sock);
    pthread_mutex_lock(&running_threads_mutex);
    running_threads--;
    pthread_mutex_unlock(&running_threads_mutex);
    return NULL;
}

int main(int argc, char *argv[]) {
    int new_socket;
    socklen_t addrlen;
    struct sockaddr_in address;
    //signal(SIGINT, stop_all);
    int port = 0;

    if(argc<=1) {
        printf("Usage: server <port>\n");
        exit(1);
    }

    port = atoi(argv[1]);

    if ((connect_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0){
        perror("The socket was created\n");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(connect_socket, (struct sockaddr *) &address, sizeof(address)) != 0){
        perror("Binding Socket\n");
        return 1;
    }

    pthread_t thread_id_keyboard;
    if( pthread_create( &thread_id_keyboard ,
                        NULL ,
                        keyboard_handler ,
                        NULL) < 0) {
        perror("could not create keyboard thread");
        return 1;
    }

    time_t last_clean = 0;
    while (!stop) {
        if (time(NULL) - last_clean > 5) {
            clean_up();
            last_clean = time(NULL);
        }


        if (listen(connect_socket, 50) < 0) {
            perror("server: listen");
            exit(1);
        }

        if ((new_socket = accept(connect_socket,
                                 (struct sockaddr *) &address,
                                 &addrlen)) < 0) {
            perror("server: accept");
            break;
        }

        if (new_socket > 0) {
#ifndef MULTITHREADED
            running_threads++;
            connection_handler((void*)(long)new_socket);
#else
            pthread_t thread_id;
            pthread_mutex_lock(&running_threads_mutex);
            running_threads++;
            pthread_mutex_unlock(&running_threads_mutex);
            if( pthread_create( &thread_id ,
                                NULL ,
                                connection_handler ,
                                (void*) (long) new_socket) < 0) {
                perror("could not create thread");
                return 1;
            }
#endif

        }
    }
    // Just for safety
    if (connect_socket != -1) {
        close(connect_socket);
    }
    // Quick and dirty, no pthread_join but will work.
    pthread_mutex_lock(&running_threads_mutex);
    while (running_threads)
    {
        pthread_mutex_unlock(&running_threads_mutex);
        usleep(10000);
        printf("%d Pending Clients\n", running_threads);
        pthread_mutex_lock(&running_threads_mutex);
    }
    pthread_mutex_unlock(&running_threads_mutex);
    printf("Bye\n");
    return 0;
}