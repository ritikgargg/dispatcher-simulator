# Dispatcher Simulator

### 1. What does this program do?

This program is expected to mimic the process management functionality of a dispatcher in the operating system. For this purpose, a multithreaded socket server is established to handle client requests. The server listens for incoming socket connections from clients, creating a new thread to handle the request pertaining to the maximum permissible thread count limit. Each of these threads invokes the dispatcher function which is responsible for extracting the request and invoking the corresponding DLL function on the supplied arguments, pertaining to the overall memory usage limit and number of open files limit.

### 2. How this program works?

The program uses a named (`AF_LOCAL`) socket at a given file path for establishing communication between server and client.

#### Client
The client sends a request to the server for invoking a DLL function. The request comprises of multiple parameters - DLL name, function name, function arguments to be sent over to the server. The server supports invocation of DLL functions with the function signature as double function(double). The parameters to be sent are serialized into a single string separated by commas(,).

#### Server
We first impose a limit on the total memory usage and number of open files using `setrlimit()`. These bounds are accepted through CLI.Then, we invoke the server function.

The server creates a named socket and starts to listen for incoming client requests, which is upper-bounded by the parameter `max_connects`(which is taken to be 8, but like the other limits, it can also be taken as input from the user, as it is not explicitly specified in the assignment).

In order to impose the max thread limit, we maintain an array that contains the active status of the previous threads in the form of 1(active)/0(completed). For a particular thread, this status is made active(or 1) at the start of the execution of the thread. Similarly, its status is marked completed(or 0) at the end of the thread function indicating thread termination. For assigning a thread to a new client request, we iterate over the array `active_status[]` to find the index of an already completed thread(active status 0). If we find the index successfully, we invoke the `create_worker_thread()` function to handle the request in a new thread, otherwise the client request is dropped and the server continues to next.

The `create_worker_thread()` function launches a new thread, invoking the `dispatcher_function()`. It returns true if the thread is created successfully, else false. Additionally, it also updates the `active_status` of the newly created thread to 1.

The `dispatcher_function()` is responsible for extracting the client request and deserializing it on the basis of ','. Then the extracted parameters of `dll_name`, `func_name` and `func_arg` are passed on to the `dll_function_invoker()`. After the execution of `dll_function_invoker()` is completed successfully, the `dispatcher_function()` breaks the connection and updates the `active_status` of the thread to 0.

`dll_function_invoker()` is responsible for invoking the required function on given parameters from the supplied DL library. It supports functions with signature double function(double). 
Some examples of the functions of Math library supported by the `dll_function_invoker()`:
1. cos(double)
2. log(double)
3. sin(double)
4. tan(double)
5. sqrt(double)
6. log(double)
7. acos(double)
8. asin(double)
9. ceil(double)
10. floor(double), etc.

The server keeps on running infinitely to listen to incoming client requests.


### 3. How to compile and run this program?

&nbsp;&nbsp;&nbsp;&nbsp;To compile the program:

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`gcc request_processing.c -lpthread -ldl`

&nbsp;&nbsp;&nbsp;&nbsp;To execute the server:

```
   ./a.out  server  local_socket_file_path  thread_cnt_limit  memory_limit  num_of_files_limit
```

&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;where,
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`local_socket_file_path` =    Path of the local file socket to be used for communication.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`thread_cnt_limit`       =    Maximum number of threads allowed to be used.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`memory_limit`           =    Maximum amount of total memory in MB permitted to be used.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`num_of_files_limit`    =    Maximum number of files permitted to be open at any point of time.

&nbsp;&nbsp;&nbsp;&nbsp;To execute the client:

```
  ./a.out  client  local_socket_file_path  dll_name  func_name  func_arg
```
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;where,
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`local_socket_file_path` =    Path of the local file socket to be used for communication.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`dll_name`               =    DLL library path to be loaded.
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`func_name`              =    Function of the DLL library to be invoked. Permitted function signature: double func(double).
&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;`func_arg `              =    Argument to be supplied to the DLL function.

**Commands for a sample run**

```
gcc request_processing.c -lpthread -ldl
For server:
        ./a.out server ./cs303_sock 100 100 100
    
For client:
    ./a.out client ./cs303_sock /lib/x86_64-linux-gnu/libm.so.6 log 2
```