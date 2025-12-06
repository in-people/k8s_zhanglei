

## secret_beg.yaml
```yaml
apiVersion: v1
kind: Secret
metadata:
  name: mysecret
type: Opaque
data:
  user: YWRtaW4=
  pass: MWYyZDFlMmU2N2Rm
```

## pod-secret-as-files.yaml
文件挂载的方式，挂载到容器中
```yaml
# pod-secret-as-files.yaml
apiVersion: v1
kind: Pod
metadata:
  name: test-secret-file-pod
spec:
  containers:
  - name: test-container
    image: busybox:1.36
    command: ["sleep", "3600"]
    volumeMounts:
    - name: secret-volume
      mountPath: /etc/mysecret   # 容器内挂载路径
      readOnly: true             # 推荐只读
  volumes:
  - name: secret-volume
    secret:
      secretName: mysecret       # 引用你已创建的 Secret
  restartPolicy: Never

```
test-container 容器的/etc/mysecret目录下会有两个文件 user paas


## test-secret-env-pod.yaml
创建1个pod,将mysecret挂载为环境变量

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: test-secret-env-pod
spec:
  containers:
  - name: test-container-env
    image: busybox:1.36          # ← 不存在？K3s 会自动 pull
    command: ["sleep", "3600"]
    env:
    - name: USERNAME
      valueFrom:
        secretKeyRef:
          name: mysecret
          key: user
    - name: PASSWORD
      valueFrom:
        secretKeyRef:
          name: mysecret
          key: pass
  restartPolicy: Never

```

进入容器，会看到如下两个环境变量
```shell
USERNAME=admin
PASSWORD=1f2d1e2e67df

kubectl exec -it test-secret-pod-env -- sh
/ # 
/ # env
KUBERNETES_SERVICE_PORT=443
KUBERNETES_PORT=tcp://10.43.0.1:443
HOSTNAME=test-secret-pod-env
SHLVL=1
HOME=/root
USERNAME=admin
TERM=xterm
KUBERNETES_PORT_443_TCP_ADDR=10.43.0.1
PATH=/usr/local/sbin:/usr/local/bin:/usr/sbin:/usr/bin:/sbin:/bin
KUBERNETES_PORT_443_TCP_PORT=443
KUBERNETES_PORT_443_TCP_PROTO=tcp
KUBERNETES_SERVICE_PORT_HTTPS=443
KUBERNETES_PORT_443_TCP=tcp://10.43.0.1:443
KUBERNETES_SERVICE_HOST=10.43.0.1
PWD=/
PASSWORD=1f2d1e2e67df
```
