

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
