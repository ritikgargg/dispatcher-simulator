#include "../all_functions.h"
#include <fcntl.h>
#include <math.h>

int main(){
    char *dll_name = "/lib/x86_64-linux-gnu/libm.so.6";
    char* func_name = "floor";
    char *func_arg = "2.5";
    int fd = open("foo.txt", O_RDWR | O_CREAT);
    int *arr = (int *)malloc(sizeof(int) * 10);
    memset(arr, 0, sizeof(arr));
    struct dispatcher_parameter *para = (struct dispatcher_parameter *)malloc(sizeof(struct dispatcher_parameter));
    para->sock_fd = fd;
    para->next_thread_idx = 0;
    para->active_status = arr;
    if(dll_function_invoker(dll_name, func_name, func_arg, para) == floor(2.5)){
        printf("Test #7 passed\n");
    }else{
        printf("Test #7 failed\n");
    }
}
