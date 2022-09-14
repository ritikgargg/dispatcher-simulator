/*
* In this unit test, we expect a successful thread creation as all favourable conditions
* are available. However, this behaviour may differ due to some other factors.
*/
#include "../all_functions.h"
#include <fcntl.h>
#include <errno.h>

int main(){
    int fd = open("foo.txt", O_RDWR | O_CREAT);
    int idx = 0;
    int arr[10];
    memset(arr, 0, sizeof(arr));
    if(create_worker_thread(fd, idx, arr) == 1){
        printf("Test #1 passed\n");
    }else{
        printf("Test #1 failed\n");
    }
}