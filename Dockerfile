FROM ubuntu:impish AS builder
RUN apt-get update
RUN apt install -y --no-install-recommends make gcc libc6-dev
COPY ./ .
RUN make
RUN chmod 777 piwatcher

FROM ubuntu:impish
WORKDIR /
COPY --from=builder /piwatcher .
