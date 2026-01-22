# Hello App Docker 部署说明

这个 Dockerfile：
- 构建一个体积小的 Python 环境镜像
- 拷贝你的代码进去
- 安装 Flask（通过公司代理 + 清华源）
- 启动 Flask 服务，监听 5000 端口

## 构建镜像

```bash
docker build -t hello-app:latest .
```

## 拉取镜像

```bash
docker pull ****/python:3.11-slim
```

从 **** 仓库拉取镜像。

## 运行容器

```bash
docker run -d -p 5000:5000 --name my-hello-app hello-app:latest
```

Flask 应用已启动，可以通过以下方式访问：

- 本地访问: http://localhost:5000
- 容器内: http://172.17.0.3:5000

## 常用命令

- 查看日志: `docker logs -f my-hello-app`
- 停止容器: `docker stop my-hello-app`
- 重启容器: `docker restart my-hello-app`
- 删除容器: `docker rm -f my-hello-app`

## IP 分配机制

172.17.0.3 是 Docker 容器内部 IP，由 Docker 自动分配。从上面的输出可以看到：

1. Docker 默认网络: bridge 网络，默认网段是 172.17.0.0/16
2. 自动分配: 容器启动时，Docker 从该网段自动分配可用 IP
3. 顺序分配:
   - 172.17.0.2 - goready 容器
   - 172.17.0.3 - my-hello-app 容器（第2个可用的 IP）

## 查看 Docker 网络

```bash
docker network ls                    # 列出所有网络
docker network inspect bridge        # 查看 bridge 网络详情
docker inspect my-hello-app          # 查看容器完整网络配置
```

## 重要区别

| 地址           | 类型          | 访问范围                      |
|----------------|---------------|-------------------------------|
| localhost:5000 | 宿主机端口映射 | 宿主机内部访问                |
| 0.0.0.0:5000   | 监听所有网卡  | 外部可访问（通过宿主机IP）    |
| 172.17.0.3:5000| 容器内部IP    | 仅 Docker 网络内可访问        |

通常应该使用 宿主机 IP:5000 或 localhost:5000 来访问应用，而不是容器内部 IP。

### 查看网络配置

```bash
docker network inspect bridge --format '{{json .IPAM.Config}}'
# 输出：[{"Subnet":"172.17.0.0/16","Gateway":"172.17.0.1"}]
```

## 访问应用

访问以下链接，来跟应用建立连接：

http://localhost:5000/
