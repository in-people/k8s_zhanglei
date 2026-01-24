#define _GNU_SOURCE
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <sched.h>
#include <signal.h>
#include <unistd.h>
#define STACK_SIZE (1024 * 1024)
static char container_stack[STACK_SIZE];
char* const container_args[] = {
  "/bin/bash",
  NULL
};

// sudo gcc -o ns ns.c 编译
// sudo ./ns  运行

// int container_main(void* arg)
// {  
//   printf("Container - inside the container!\n");
//   execv(container_args[0], container_args);
//   printf("Something's wrong!\n");
//   return 1;
// }

int container_main(void* arg)
{
  printf("Container - inside the container!\n");
  // 先将根目录设置为 private propagation，防止挂载传播到父命名空间
  // 加上 MS_PRIVATE 容器内挂载不影响宿主机（隔离成功）
  mount("", "/", NULL, MS_PRIVATE | MS_REC, "");

  // 挂载 tmpfs 到 /tmp
  mount("none", "/tmp", "tmpfs", 0, "");

  // 创建 /wdl 目录后再挂载 tmpfs
  mkdir("/wdl", 0755);
  mount("none", "/wdl", "tmpfs", 0, "");

  execv(container_args[0], container_args);
  printf("Something's wrong!\n");
  return 1;
}

  // - CLONE_NEWUTS - 隔离主机名                                                                                                                                  
  // - CLONE_NEWPID - 隔离进程 ID                                                                                                                                 
  // - CLONE_NEWNET - 隔离网络栈   

int main()
{
  printf("Parent - start a container!\n");
  // CLONE_NEWNS 挂载点隔离
  int container_pid = clone(container_main, container_stack+STACK_SIZE, CLONE_NEWNS | SIGCHLD , NULL);
  waitpid(container_pid, NULL, 0);
  printf("Parent - container stopped!\n");
  return 0;
}
