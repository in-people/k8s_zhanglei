FROM public-docker-virtual.artnj.zte.com.cn/python:3.11-slim 

ENV http_proxy=http://proxyhk.zte.com.cn:80
ENV https_proxy=http://proxyhk.zte.com.cn:80

WORKDIR /app
COPY . /app

RUN pip install --no-cache-dir --progress-bar off flask -i http://pypi.tuna.tsinghua.edu.cn/simple --trusted-host pypi.tuna.tsinghua.edu.cn

EXPOSE 5000
CMD ["python", "app.py"]
