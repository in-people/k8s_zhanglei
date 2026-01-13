
# C程序 clone演示


```c
#define _GNU_SOURCE
#include <sched.h>
#include <sys/syscall.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

#define STACK_SIZE (1024 * 1024)

int child_func(void *arg) {
    // printf 默认使用行缓冲或全缓冲，子进程的输出可能还在缓冲区中，当 exit(0) 被调用时缓冲区没有刷新就直接关闭了。
    printf("In new PID namespace: getpid()=%d, getppid()=%d\n", getpid(), getppid());
    fflush(stdout);  // 强制刷新输出缓冲区
    // 注意：这里 getppid() 可能返回 0！因为在新 PID ns 中，父进程不可见
    sleep(2);
    return 0;
}

/*
编译命令：gcc -o clone_example clone_example.c
运行命令：sudo ./clone_example
 
*/

int main() {
    char *stack = malloc(STACK_SIZE);
    if (!stack) { perror("malloc"); exit(1); }
    char *stack_top = stack + STACK_SIZE;

    // 启用 PID namespace 隔离
    // Parent (host PID=1376423) spawned child with host PID=1376424
    // In new PID namespace: getpid()=1, getppid()=0 新进程空间下，新进程的PID为1
    // pid_t pid = clone(child_func, stack_top, CLONE_NEWPID | SIGCHLD, NULL);


    // Parent (host PID=1383186) spawned child with host PID=1383187
    // In new PID namespace: getpid()=1383187, getppid()=1383186
    pid_t pid = clone(child_func, stack_top, SIGCHLD, NULL);

    if (pid == -1) {
        perror("clone (need root or CAP_SYS_ADMIN)");
        free(stack);
        exit(1);
    }

    // clone() 返回后，只有父进程会继续执行这里的代码
    // 子进程已经跳转到 child_func 执行了
    printf("Parent (host PID=%d) spawned child with host PID=%d\n", getpid(), pid);
    waitpid(pid, NULL, 0);
    free(stack);
    return 0;
}
```


**Linux** **PID namespace（进程 ID 命名空间）** **的核心机制。**

### 说明：

> **`CLONE_NEWPID` **的作用是：让新创建的进程进入一个全新的 PID namespace，在这个 namespace 中，它的 PID 是 1，并且它只能看到自己 namespace 内的进程。****

### 详细解释

#### 情况一：**使用 `CLONE_NEWPID`**

```
pid_t pid =clone(child_func, stack_top, CLONE_NEWPID | SIGCHLD,NULL);
```

* **子进程会进入一个** **全新的 PID namespace** **。**
* **在这个新 namespace 中：**
  * **它是第一个进程 →** **PID = 1**
  * 它的父进程（原进程）**不在这个 namespace 中** **→** `getppid()` **返回** **0**
* **它** **看不到宿主机上的其他进程** **（比如** `ps aux` **只会显示自己和后代）**
* **这就是容器中“PID=1”的来源！**

**✅ 输出示例：**

```c
In new PID namespace: getpid()=1, getppid()=0
```

---

#### 情况二：**不使用 `CLONE_NEWPID`（只用 `SIGCHLD`）**

```c
pid_t pid =clone(child_func, stack_top, SIGCHLD,NULL);
```

* **子进程** **仍在原来的 PID namespace（即宿主机的全局 namespace）中** **。**
* **它的 PID 是内核分配的一个普通数字（如 1383187）**
* **它的父进程 PID 就是主进程的 PID（如 1383186）**
* **它能看见宿主机上所有进程（和普通进程无异）**

**输出示例：**

```c
In new PID namespace: getpid()=1383187, getppid()=1383186
```

> **注意：虽然你打印时写了 "In new PID namespace"，但实际上** **并没有新 namespace** **！这只是普通进程。**

---

### 类比理解

**表格**

| **场景**               | **是否有新 PID namespace？** | **子进程 PID**           | **能否看到宿主机进程？** | **类似于**                               |
| ---------------------------- | ---------------------------------- | ------------------------------ | ------------------------------ | ---------------------------------------------- |
| `clone(..., CLONE_NEWPID)` | **是**                       | **1**                    | **不能**                 | **Docker 容器内的 init 进程**            |
| `clone(..., SIGCHLD)`      | **否**                       | **普通数字（如 12345）** | **能**                   | **普通** `fork()` **出的子进程** |

---

### 关键点总结

**表格**

| **特性**                        | **说明**                                                       |
| ------------------------------------- | -------------------------------------------------------------------- |
| `CLONE_NEWPID`                      | **创建新的 PID namespace，实现****进程视图隔离**               |
| **新 namespace 中的第一个进程** | **PID 固定为** **1**                                     |
| `getppid()` **返回 0**        | **因为父进程在** **另一个 namespace** **，不可见** |
| **不加** `CLONE_NEWPID`       | **所有进程共享同一个全局 PID 空间，无隔离**                    |

---

### 为什么 Docker 需要 `CLONE_NEWPID`？

* **容器内的程序（如 nginx、bash）期望自己运行在一个“干净的系统”中。**
* **它们通常假设自己可以成为**  **PID 1** **（负责回收僵尸进程等）。**
* **如果没有 PID namespace，容器内** `ps` **会列出宿主机上千个进程，既不安全也不符合预期。**
