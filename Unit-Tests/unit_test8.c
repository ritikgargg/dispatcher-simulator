/*
* In this unit test, an attempt is made to restrict the memory availability, in order 
* to force failure of thread creation call. In this example, a limit of 3 MB is imposed.
* This call is expected to fail. However, this behaviour may differ.
*/
#include "../all_functions.h"
#include <fcntl.h>
#include <errno.h>

int main(){
    int fd = open("foo.txt", O_RDWR | O_CREAT);
    int idx = 0;
    int arr[10];
    memset(arr, 0, sizeof(arr));
    struct rlimit mem_lim;
    mem_lim.rlim_cur = 3 * 1024 * 1024;
    mem_lim.rlim_max = 3 * 1024 * 1024;
    if(setrlimit(RLIMIT_AS, &mem_lim) == -1){
        fprintf(stderr, "%s\n", strerror(errno));
        exit(-1);
    }
    if(create_worker_thread(fd, idx, arr) == 0){
        printf("Test #8 passed\n");
    }else{
        printf("Test #8 failed\n");
    }
}