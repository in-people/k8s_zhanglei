apiVersion: v1
kind: Pod
metadata:
  name: nginx
spec:
  shareProcessNamespace: true
  containers:
  - name: nginx
    image: swr-plat:2524/admin/image/nginx:v19
  - name: shell
    image: swr-plat:2524/admin/image/busybox:v1
    stdin: true
    tty: true

---

apiVersion: v1
kind: Pod
metadata:
  name: nginx
spec:
  hostNetwork: true
  hostIPC: true
  hostPID: true
  containers:
  - name: nginx
    image: swr-plat:2524/admin/image/nginx:v19
  - name: shell
    image: swr-plat:2524/admin/image/busybox:v1
    stdin: true
    tty: true

---

apiVersion: v1
kind: Pod
metadata:
  name: lifecycle-demo
spec:
  containers:
  - name: lifecycle-demo-container
    image: swr-plat:2524/admin/image/nginx:v19
    lifecycle:
      postStart:
        exec:
          command: ["/bin/sh", "-c", "echo Hello from the postStart handler > /usr/share/message"]
      preStop:
        exec:
          command: ["/usr/sbin/nginx","-s","quit"]

---

TCF-3952842 bug-fix riscv安装nerdctl失败；应先安装containerd，再安装nerdctl

apiVersion: v1
kind: Pod
metadata:
  name: test-projected-volume
spec:
  containers:
  - name: test-secret-volume
    image: swr-plat:2524/admin/image/busybox:v1
    args:
    - sleep
    - "86400"
    volumeMounts:
    - name: mysql-cred
      mountPath: "/projected-volume"
      readOnly: true
  volumes:
  - name: mysql-cred
    projected:
      sources:
      - secret:
          name: user
      - secret:
          name: pass

---

apiVersion: v1
kind: Pod
metadata:
  name: test-downwardapi-volume
  labels:
    zone: us-est-coast
    cluster: test-cluster1
    rack: rack-22
spec:
  containers:
  - name: client-container
    image: swr-plat:2524/admin/image/busybox:v1
    command: ["sh", "-c"]
    args:
    - |
      while true; do
        if [[ -e /etc/podinfo/labels ]]; then
          echo -en '\n\n'; cat /etc/podinfo/labels; fi;
        sleep 5;
      done;
    volumeMounts:
    - name: podinfo
      mountPath: /etc/podinfo
      readOnly: false
  volumes:
  - name: podinfo
    projected:
      sources:
      - downwardAPI:
          items:
          - path: "labels"
            fieldRef:
              fieldPath: metadata.labels

---

Mounts:
  /etc/podinfo from podinfo (rw)
  /var/run/secrets/kubernetes.io/serviceaccount from kube-api-access-24rfb (ro)
Conditions:
  Type                        Status
  PodReadyToStartContainers   True
  Initialized                 True
  Ready                       True
  ContainersReady             True
  PodScheduled                True
Volumes:
  podinfo:
    Type:         Projected (a volume that contains injected data from multiple sources)
    DownwardAPI:  true
  kube-api-access-24rfb:
    Type:                    Projected (a volume that contains injected data from multiple sources)
    TokenExpirationSeconds:  3607
    ConfigMapName:           kube-root-ca.crt
    Optional:                false
    DownwardAPI:             true

---

/var/run/secrets/kubernetes.io/serviceaccount from kube-api-access-24rfb (ro)
是 Kubernetes 自动为每个 Pod 挂载的服务账户（ServiceAccount）凭证卷（volume），用于让 Pod 内的容器能够安全地访问 Kubernetes API Server。

## 详细解释

### 1. 这是什么？

这是一个 Projected Volume（投影卷），由 Kubernetes 自动注入。它包含以下关键文件（在容器内路径 /var/run/secrets/kubernetes.io/serviceaccount/ 下）：

- **token**：一个 Bearer Token，用于向 API Server 身份认证。
- **ca.crt**：API Server 的 CA 证书，用于验证 TLS 连接。
- **namespace**：当前 Pod 所在的命名空间名称。

**示例（在容器内执行）：**

```sh
ls /var/run/secrets/kubernetes.io/serviceaccount/
# 输出: ca.crt  namespace  token
```

### 2. kube-api-access-24rfb 是什么？

这是 Kubernetes 自动生成的 Volume 名称（后缀如 24rfb 是随机字符串）。在较新版本（Kubernetes ≥ v1.22）中，它取代了旧的 default-token-xxxxx，使用 TokenRequest API + BoundServiceAccountTokenVolume 特性，提供：

- 短期有效的 token（默认 1 小时，TokenExpirationSeconds: 3607 ≈ 1 小时）
- 更高的安全性（避免长期 token 泄露风险）

### 3. (ro) 是什么意思？

表示该卷以 **只读（read-only）** 方式挂载到容器中。这是安全最佳实践 —— 应用只需读取凭证，无需修改。

### 4. 为什么需要这个？

当 Pod 中的程序（比如 dns-operator）需要调用 Kubernetes API（例如：查询 Service、创建 ConfigMap、监听事件等），就必须通过这个目录下的 token 和 ca.crt 来完成认证和加密通信。例如，Go client-go 库默认会自动从该路径加载凭证。

## 安全机制说明（v1.22+）

| 特性 | 说明 |
|------|------|
| 自动挂载 | 只要 Pod 使用了 ServiceAccount（默认使用 default），就会自动挂载 |
| 短期 Token | Token 有效期约 1 小时，过期后 kubelet 自动轮换 |
| 绑定到 Pod | Token 与特定 Pod 绑定，Pod 删除后 token 立即失效 |
| 最小权限 | 实际权限由 RBAC 控制（RoleBinding / ClusterRoleBinding） |

## 常见问题

**Q：可以禁用这个挂载吗？**

A：可以，但不推荐，除非应用完全不需要访问 API Server。在 Pod spec 中设置：

```yaml
spec:
  automountServiceAccountToken: false
```

**Q：为什么我的 Pod 报错 "cannot list services"？**

A：通常是因为：
- 没有挂载（但你这里挂载了 ✅）
- RBAC 权限不足（最常见！）→ 需要给 ServiceAccount 绑定合适的 Role

## 回到你的场景

你的 op-containers-dns-operator Pod 已经正确挂载了 serviceaccount 凭证，说明它有能力访问 Kubernetes API。但它仍然报错：

```
services "kube-dns" not found
```

这进一步证明：
- 不是权限问题（可能有权限但查错地方）
- 而是代码在错误的命名空间（如 default）查找 kube-dns

因为：
- 凭证已挂载 ✅
- 如果真没权限，错误会是 **forbidden**，而不是 **not found**

## 总结

| 项目 | 说明 |
|------|------|
| 路径 | /var/run/secrets/kubernetes.io/serviceaccount |
| 内容 | token, ca.crt, namespace |
| 用途 | 让 Pod 内程序安全访问 Kubernetes API |
| 挂载方式 | 自动、只读、短期有效 token（v1.22+） |
| 你的 Pod | 已正确配置，具备 API 访问能力 |

---

## 容器健康检查和恢复机制

> -c：告诉 shell：下一个参数是要执行的命令字符串

apiVersion: v1
kind: Pod
metadata:
  labels:
    test: liveness
  name: test-liveness-exec
spec:
  containers:
  - name: liveness
    image: swr-plat:2524/admin/image/busybox:v1
    args:
    - /bin/sh
    - -c
    - touch /tmp/healthy; sleep 30; rm -rf /tmp/healthy; sleep 600
    livenessProbe:
      exec:
        command:
        - cat
        - /tmp/healthy
      initialDelaySeconds: 5
      periodSeconds: 5
