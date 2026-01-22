FROM ****/python:3.11-slim 

ENV http_proxy=****
ENV https_proxy=*****

WORKDIR /app
COPY . /app

RUN pip install --no-cache-dir --progress-bar off flask -i http://pypi.tuna.tsinghua.edu.cn/simple --trusted-host pypi.tuna.tsinghua.edu.cn

EXPOSE 5000
CMD ["python", "app.py"]
