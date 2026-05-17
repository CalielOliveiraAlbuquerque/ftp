FROM ubuntu:24.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    g++ \
    pkg-config \
    libavcodec-dev \
    libavutil-dev \
    libswscale-dev \
    libx11-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY server.cpp .
RUN g++ server.cpp -o app $(pkg-config --cflags --libs libavcodec libavutil libswscale x11)

CMD ["./app"]
