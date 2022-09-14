#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>
#include <sys/un.h>
#include <stddef.h>
#include <dlfcn.h>
#include <sys/resource.h>

/**
 * This code is adapted from the samples available at:
 * https://opensource.com/article/19/4/interprocess-communication-linux-networking and
 * https://www.gnu.org/software/libc/manual/html_node/Local-Socket-Example.html
 *
 * Compile it using: gcc request_processing.c -lpthread -ldl
 * 
 */

bool create_worker_thread(int fd, int idx, int arr[]);

/*
* Struct passed as an argument to the dispatcher function.
*/
struct dispatcher_parameter{
    int sock_fd;
    int next_thread_idx;
    int *active_status;
};

void log_msg(const char *msg, bool terminate) {
    printf("%s\n", msg);
    if (terminate) exit(-1); /* failure */
}

/**
 * Create a named (AF_LOCAL) socket at a given file path.
 * @param socket_file
 * @param is_client whether to create a client socket or server socket
 * @return Socket descriptor
 */
int make_named_socket(const char *socket_file, bool is_client) {
    printf("Creating AF_LOCAL socket at path %s\n", socket_file);
    if (!is_client && access(socket_file, F_OK) != -1) {
        log_msg("An old socket file exists, removing it.", false);
        if (unlink(socket_file) != 0) {
            log_msg("Failed to remove the existing socket file.", true);
        }
    }
    struct sockaddr_un name;
    /* Create the socket. */
    int sock_fd = socket(AF_LOCAL, SOCK_STREAM, 0);
    if (sock_fd < 0) {
        log_msg("Failed to create socket.", true);
    }

    /* Bind a name to the socket. */
    name.sun_family = AF_LOCAL;
    strncpy (name.sun_path, socket_file, sizeof(name.sun_path));
    name.sun_path[sizeof(name.sun_path) - 1] = '\0';

    /* The size of the address is
       the offset of the start of the socket_file,
       plus its length (not including the terminating null byte).
       Alternatively you can just do:
       size = SUN_LEN (&name);
   */
    size_t size = (offsetof(struct sockaddr_un, sun_path) +
                   strlen(name.sun_path));
    if (is_client) {
        if (connect(sock_fd, (struct sockaddr *) &name, size) < 0) {
            log_msg("connect failed", 1);
        }
    } else {
        if (bind(sock_fd, (struct sockaddr *) &name, size) < 0) {
            log_msg("bind failed", 1);
        }
    }
    return sock_fd;
}

/**
 * Invokes the given function  from the DLL library, on the supplied parameters.
 * @param dll_name DL library path to be loaded
 * @param func_name Function in the DLL to be invoked
 * @param func_arg Parameter to the function of the DLL
 * @param para 
 */

double dll_function_invoker(char *dll_name, char* func_name, char *func_arg, struct dispatcher_parameter *para){
    void *handle;
    double (*function)(double);
    char *error;
    //To open the library and prepare it for use
    handle = dlopen (dll_name, RTLD_LAZY);
    if (!handle) {
        fputs (dlerror(), stderr);
        write(para->sock_fd, "Failed to process your request!", 31); 
        close(para->sock_fd);
        para->active_status[para->next_thread_idx] = 0;
        pthread_exit(NULL);
    }
    function = dlsym(handle, func_name);
    if ((error = dlerror()) != NULL)  {
        fputs(error, stderr);
        dlclose(handle);
        write(para->sock_fd, "Failed to process your request!", 31); 
        close(para->sock_fd);
        para->active_status[para->next_thread_idx] = 0;
        pthread_exit(NULL);
    }
    double parameter = atof(func_arg);
    double ans = (*function)(parameter);
    printf ("%f\n", ans);

    //To close a DL library
    dlclose(handle);
    write(para->sock_fd, "Your request has been processed!", 32);
    return ans;
}
/**
 * Starts a server socket that waits for incoming client connections.
 * @param socket_file
 * @param max_connects
 * @param thread_limit
 */
_Noreturn void start_server_socket(char *socket_file, int max_connects, char *thread_limit) {
    int sock_fd = make_named_socket(socket_file, false);
    
    /* listen for clients, up to MaxConnects */
    if (listen(sock_fd, max_connects) < 0) {
        log_msg("Listen call on the socket failed. Terminating.", true); /* terminate */
    }
    log_msg("Listening for client connections...\n", false);
    int max_threads = atoi(thread_limit);

    /* Array to store the activity status of the threads, in order to impose the thread_limit bound */
    int active_status[max_threads];
    memset(active_status, 0, sizeof(active_status));
    int next_thread_idx = 0, tmp;
    /* Listens indefinitely */
    while (1) {
        struct sockaddr_in caddr; /* client address */
        int len = sizeof(caddr);  /* address length could change */

        printf("Waiting for incoming connections...\n");
        int client_fd = accept(sock_fd, (struct sockaddr *) &caddr, &len);  /* accept blocks */

        if (client_fd < 0) {
            log_msg("accept() failed. Continuing to next.", 0); /* don't terminate, though there's a problem */
            continue;
        }
        //Check the active threads status
        tmp = next_thread_idx;
        while (tmp < max_threads && active_status[tmp] != 0)
        {
            tmp += 1;
        }
        if(tmp == max_threads){
            tmp = 0;
            while (tmp < next_thread_idx && active_status[tmp] != 0)
            {
                tmp += 1;
            }
            if(tmp == next_thread_idx){
                log_msg("Cannot accomodate this request.Continuing to next.", 0);
                continue;
            }
        }
        next_thread_idx = tmp;
        /* Start a worker thread to handle the received connection. */
        if (!create_worker_thread(client_fd, next_thread_idx, active_status)) {
            log_msg("Failed to create worker thread. Continuing to next.", 0);
            continue;
        }
    }  /* while(1) */
}


/**
 * This function is executed in a separate thread.
 * @param argument This argument is casted to a pointer of struct dispatcher_parameter type.
 */
void* dispatcher_function(void *argument) {
    struct dispatcher_parameter *para = (struct dispatcher_parameter *)(argument);
    log_msg("SERVER: thread_function: starting", false);
    char buffer[5000];
    char dll_name[50], func_name[50], func_arg[50];
    int idx, idx2;
    memset(buffer, '\0', sizeof(buffer));
    int count = read(para->sock_fd, buffer, sizeof(buffer));
    if (count > 0) {
        printf("SERVER: Received from client: %s\n", buffer);
        write(para->sock_fd, "Your request has been received!", 31); 
        idx = 0; idx2 = 0;
        while(buffer[idx] != ','){
            dll_name[idx2] = buffer[idx];
            idx++; idx2++;
        }
        dll_name[idx2] = '\0';
        idx += 1; idx2 = 0;
        while(buffer[idx] != ','){
            func_name[idx2] = buffer[idx];
            idx++; idx2++;
        }
        func_name[idx2] = '\0';
        idx += 1; idx2 = 0;
        while(buffer[idx] != '\0'){
            func_arg[idx2] = buffer[idx];
            idx++; idx2++;
        }
        func_arg[idx2] = '\0';
        dll_function_invoker(dll_name, func_name, func_arg, para);
    }
    close(para->sock_fd); /* break connection */
    log_msg("SERVER: thread_function: Done. Worker thread terminating.", false);
    para->active_status[para->next_thread_idx] = 0;
    pthread_exit(NULL);
    return NULL;
}

/**
 * This function launches a new worker thread.
 * @param sock_fd
 * @param next_thread_idx Index given to current thread.
 * @param active_status Array containing the active status of threads.
 * @return Return true if thread is successfully created, otherwise false.
 */
bool create_worker_thread(int sock_fd, int next_thread_idx, int active_status[]) {
    log_msg("SERVER: Creating a worker thread.", false);
    pthread_t thr_id;
    struct dispatcher_parameter *para = (struct dispatcher_parameter *)malloc(sizeof(struct dispatcher_parameter));
    para->sock_fd = sock_fd;
    para->next_thread_idx = next_thread_idx;
    para->active_status = active_status;
    int rc = pthread_create(&thr_id,
            /* Attributes of the new thread, if any. */
                            NULL,
            /* Pointer to the function which will be
             * executed in new thread. */
                            dispatcher_function,
            /* Argument to be passed to the above
             * thread function. */
                            (void *) para);
    if (rc) {
        log_msg("SERVER: Failed to create thread.", false);
        return false;
    }
    active_status[next_thread_idx] = 1;
    return true;
}

/**
 * Sends a request to the server socket.
 * @param socket_file Path of the server socket on localhost.
 * @param dll_name DL library path to be loaded
 * @param func_name Function in the DLL to be invoked
 * @param func_arg Parameter to the function of the DLL
 */
void send_message_to_socket(char *socket_file, char *dll_name, char *func_name, char *func_arg) {
    int sockfd = make_named_socket(socket_file, true);
    int dll_len = strlen(dll_name), func_name_len = strlen(func_name), func_arg_len = strlen(func_arg);
    char msg[dll_len + func_name_len + func_arg_len + 3];
    int idx = 0;
    for(int i = 0; i < dll_len; i++){
        msg[idx++] = dll_name[i];
    }
    msg[idx++] = ',';
    for(int i = 0; i < func_name_len; i++){
        msg[idx++] = func_name[i];
    }
    msg[idx++] = ',';
    for(int i = 0; i < func_arg_len; i++){
        msg[idx++] = func_arg[i];
    }
    msg[idx] = '\0';
    /* Write some stuff and read the echoes. */
    log_msg("CLIENT: Connect to server, about to write some stuff...", false);
    if (write(sockfd, msg, strlen(msg)) > 0) {
        /* get confirmation echoed from server and print */
        char buffer[5000];
        memset(buffer, '\0', sizeof(buffer));
        if (read(sockfd, buffer, sizeof(buffer)) > 0) {
            printf("CLIENT: Received from server:: %s\n", buffer);
        }
    }
    log_msg("CLIENT: Processing done, about to exit...", false);
    close(sockfd); /* close the connection */
}