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
#include "all_functions.h" 

/**
 * This is the driver function you can use to run server/client.
 * @param argc
 * @param argv
 * @return
 */
int main(int argc, char *argv[]) {
    if (argc <= 5) {
        printf("Usage: %s [server  local_socket_file_path  thread_cnt_limit  memory_limit  num_of_files_limit| client  local_socket_file_path  dll_name  func_name  func_arg]\n",
               argv[0]);
        exit(-1);
    }
    if (0 == strcmp("server", argv[1])) {
        int num_of_files_limit = atoi(argv[5]);
        struct rlimit file_lim;
        file_lim.rlim_cur = num_of_files_limit;
        file_lim.rlim_max = num_of_files_limit;
        if(setrlimit(RLIMIT_NOFILE, &file_lim) == -1){
            fprintf(stderr, "%s\n", strerror(errno));
            exit(-1);
        }

        int memory_limit = atoi(argv[4]) * 1024 * 1024; //Since argv[4] is in MB
        struct rlimit mem_lim;
        mem_lim.rlim_cur = memory_limit;
        mem_lim.rlim_max = memory_limit;
        if(setrlimit(RLIMIT_AS, &mem_lim) == -1){
            fprintf(stderr, "%s\n", strerror(errno));
            exit(-1);
        }
        start_server_socket(argv[2], 8, argv[3]);
    } else {
        send_message_to_socket(argv[2], argv[3], argv[4], argv[5]);
    }
}