# Docker 网络操作手册

## 一、容器操作命令

### 1. 运行容器

```bash
docker run -d --name nginx-1 swr:2512/admin/nginx:v17
```

- `-d`: 后台运行（detached mode）
- `--name`: 指定容器名称
- `swr:2512/admin/nginx:v17`: 镜像地址

### 2. 运行容器并指定网络

```bash
docker run -d --network dockerBridge --name nginx-1 swr:2512/admin/nginx:v17
```

- `--network`: 指定容器连接的网络（本例中使用自定义网络 dockerBridge）

### 3. 进入容器

```bash
docker exec -it nginx-1 /bin/sh
```

- `exec`: 在运行中的容器内执行命令
- `-it`: 交互式终端
- `/bin/sh`: 使用 sh shell（Alpine 镜像默认）

### 4. 停止和删除容器

```bash
docker stop nginx-1
docker rm nginx-1
```

## 二、网络诊断命令

### 1. 查看容器网络配置

```bash
# 查看详细网络配置
docker inspect nginx-1 | grep -A 30 "NetworkSettings"

# 查看是否禁用网络（关键字段）
docker inspect nginx-1 | grep "NetworkDisabled"

# 格式化输出查看IP
docker inspect nginx-1 --format='{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}'

# 查看网络模式
docker inspect nginx-1 --format='{{.HostConfig.NetworkMode}}'
```

### 2. 查看 Docker 网络

```bash
# 列出所有 Docker 网络
docker network ls

# 查看网络详情
docker network inspect dockerBridge

# 查看网络中所有容器及其IP
docker network inspect dockerBridge --format='{{range .Containers}}{{.Name}}: {{.IPv4Address}}{{println}}{{end}}'
```

### 3. 系统网络诊断

```bash
# 查看 Linux 网桥
ip link show type bridge

# 查看具体网桥配置
ip addr show dockerBridge

# 查看所有网络接口
ip link show

# 查看虚拟以太网接口
ip link show | grep veth

# 查看网桥连接（需要安装 bridge-utils）
brctl show dockerBridge
```

## 三、容器内部网络诊断

### 1. 基础网络命令

```bash
# 查看IP地址（Alpine镜像）
ip addr

# 查看路由表
route

# 测试网络连通性
ping 172.17.0.1
```

### 2. 安装网络工具

```bash
# Alpine Linux 安装网络工具
apk update
apk add net-tools  # 安装 ifconfig
apk add iproute2   # 安装 ip 命令（通常已安装）
```

### 3. 查看内核网络信息

```bash
# 查看网络接口
ls /sys/class/net/

# 查看接口链接信息
cat /sys/class/net/eth0/iflink  # 显示对应的宿主机接口索引

# 查看接口状态
cat /sys/class/net/eth0/operstate
```

## 四、系统诊断命令

### 1. 查看 Docker 服务状态

```bash
systemctl status docker
journalctl -u docker -n 50  # 查看 Docker 日志
systemctl restart docker    # 重启 Docker 服务
```

### 2. 查看防火墙规则

```bash
iptables -L -n | grep -i docker
```

### 3. 查看 Docker 配置

```bash
cat /etc/docker/daemon.json 2>/dev/null || echo "No daemon.json found"
```

## 五、问题排查流程总结

**问题现象**：容器内没有虚拟网卡（只有 lo 回环接口）

**排查步骤**：

1. 检查容器进入方式：

```bash
# Alpine 镜像使用 /bin/sh 而不是 /bin/bash
docker exec -it container_name /bin/sh
```

2. 检查网络模式：

```bash
docker inspect container_name --format='{{.HostConfig.NetworkMode}}'
```

3. 检查是否禁用网络：

```bash
docker inspect container_name | grep "NetworkDisabled"
```

4. 检查 Docker 网络列表：

```bash
docker network ls
```

5. 验证网络配置：

```bash
docker network inspect network_name
```

**关键发现**：

- 镜像默认禁用网络：`"NetworkDisabled": true`
- 缺少默认 bridge 网络：只有 dockerBridge 没有 bridge

**解决方案**：

```bash
# 明确指定网络
docker run -d --name nginx-1 --network dockerBridge swr:2512/admin/nginx:v17
```

## 六、查找容器与宿主机 veth 设备对应关系

### 1. 问题描述

在网络故障排查时，经常需要找到 Docker 容器在宿主机上对应的 veth 设备，以便进行网络流量分析和问题定位。

### 2. 使用 iflink 查找对应关系

**原理**：通过查看容器内 eth0 网卡的 iflink 值，与宿主机上 veth 设备的接口索引进行匹配。

**操作步骤**：

1. **在宿主机上查看所有网络接口**：

```bash
ip link
```

示例输出：

```
......
9: veth0e9cd8d@if8: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc noqueue master docker0 state UP mode DEFAULT group default
    link/ether 6a:fb:59:e5:7e:da brd ff:ff:ff:ff:ff:ff link-netnsid 1
```

2. **进入容器查看 iflink 值**：

```bash
# 进入容器（根据镜像类型选择 shell）
docker exec -it 容器名 /bin/sh    # Alpine 镜像
docker exec -it 容器名 /bin/bash   # 其他镜像
```

3. **在容器内查看 eth0 的 iflink**：

```bash
cat /sys/class/net/eth0/iflink
```

示例输出：9

4. **匹配对应关系**：
   - 容器内 `eth0` 的 `iflink` 值为 `9`
   - 宿主机上接口索引为 `9` 的是 `veth0e9cd8d`
   - 因此该容器对应的 veth 设备是 `veth0e9cd8d`

### 3. 自动化脚本方案

由于手动查找需要进入容器执行命令，对于生产环境可能不够便利。可以使用 GitHub 上的专业工具：

**项目地址**：https://github.com/micahculpepper/dockerveth

这个脚本可以自动化地查找容器与宿主机 veth 设备的对应关系，无需手动进入容器。

### 4. 批量查找脚本示例

```bash
#!/bin/bash
# 批量查找所有容器对应的 veth 设备

echo "容器名 -> 宿主机 veth 设备对应关系"
echo "=================================="

# 获取所有运行中的容器
containers=$(docker ps --format "table {{.Names}}\t{{.ID}}" | tail -n +2)

for line in $containers; do
    container_name=$(echo $line | awk '{print $1}')
    container_id=$(echo $line | awk '{print $2}')

    # 获取容器内 eth0 的 iflink
    iflink=$(docker exec $container_name cat /sys/class/net/eth0/iflink 2>/dev/null)

    if [ ! -z "$iflink" ]; then
        # 在宿主机上查找对应的 veth 设备
        veth_device=$(ip link | grep "^$iflink:" | awk -F': ' '{print $2}' | awk '{print $1}')
        echo "$container_name -> $veth_device (iflink: $iflink)"
    else
        echo "$container_name -> 无法获取 iflink"
    fi
done
```

### 5. 注意事项

- **权限要求**：需要 root 权限或有足够的网络管理权限
- **容器类型**：仅适用于使用 veth pair 网络模式的容器（默认的 bridge 网络）
- **网络模式**：host 模式和 none 模式的容器没有 veth 设备
- **容器状态**：容器必须处于运行状态才能获取 iflink 值

## 八、重要概念

- **veth pair**：虚拟以太网设备对，连接容器和宿主机网桥
- **iflink**：容器内网卡对应的宿主机接口索引
- **dockerBridge**：自定义网桥，替代默认的 bridge
- **NetworkDisabled**：容器配置项，控制是否启用网络

## 九、常用组合命令

```bash
# 一键获取容器IP
docker inspect -f '{{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}' 容器名

# 查看容器网络状态摘要
docker inspect 容器名 --format='容器: {{.Name}}
网络模式: {{.HostConfig.NetworkMode}}
IP地址: {{range .NetworkSettings.Networks}}{{.IPAddress}}{{end}}
网关: {{range .NetworkSettings.Networks}}{{.Gateway}}{{end}}'

# 验证容器网络
docker exec 容器名 sh -c "ip addr && echo '---' && ping -c 2 172.17.0.1"

# 快速查找容器对应的 veth 设备
find_veth() {
    container_name=$1
    iflink=$(docker exec $container_name cat /sys/class/net/eth0/iflink 2>/dev/null)
    if [ ! -z "$iflink" ]; then
        ip link | grep "^$iflink:" | awk -F': ' '{print $2}' | awk '{print $1}'
    fi
}

# 一行命令查找 veth 设备
docker exec 容器名 cat /sys/class/net/eth0/iflink | xargs -I {} ip link | grep "^{}:" | awk -F': ' '{print $2}' | awk '{print $1}'
```

这些命令涵盖了从基础容器操作到网络深度诊断的完整流程，是 Docker 网络故障排查的必备工具集。新添加的 veth 设备查找功能为网络故障排查提供了更精确的诊断手段。
