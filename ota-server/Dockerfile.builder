# ESP-IDF Builder Container
FROM espressif/idf:v5.5.2

WORKDIR /project

# Install additional tools
RUN apt-get update && apt-get install -y \
    git \
    && rm -rf /var/lib/apt/lists/*

# Entry point for building
COPY build.sh /build.sh
RUN chmod +x /build.sh

CMD ["/bin/bash"]
