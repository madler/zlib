FROM    ubuntu:24.10

ARG     RUNNER_PATCH="https://github.com/anup-kodlekere/gaplib/raw/1a5e012e8af1ff96f482f493ba1c27eb115930af/patches/runner-main-sdk8-s390x.patch"
ARG     RUNNER_REPO="https://github.com/actions/runner"
ARG     ACTIONS_RUNNER_SCRIPT="actions_runner.sh"
ARG     ACTIONS_RUNNER_CONFIG="actions_runner_conf.sh"

ENV     DEBIAN_FRONTEND=noninteractive

# install dependencies
RUN     apt-get -qq update -y && \
        apt-get -qq -y install wget git sudo curl dotnet-sdk-8.0 && \
        apt-get autoclean 

# load actions runner
RUN     cd /tmp && \
        git clone -q ${RUNNER_REPO} && \
        cd runner && \
        git checkout main -b build && \
        wget ${RUNNER_PATCH} -O runner.patch && \
        git apply runner.patch && \
        sed -i'' -e /version/s/8......\"$/$8.0.100\"/ src/global.json

# build actions runner
RUN     cd /tmp/runner/src && \
        ./dev.sh layout && \
        ./dev.sh package && \
        rm -rf /root/.dotnet /root/.nuget

# setup actions runner env
RUN     useradd -c "Actions Runner" -m actions_runner && \
        usermod -L actions_runner

RUN     tar -xf /tmp/runner/_package/*.tar.gz -C /home/actions_runner && \
        chown -R actions_runner:actions_runner /home/actions_runner

# cleanup
RUN     rm -rf /tmp/runner

# install build utils for the CI
RUN     apt-get -qq update -y && \
        apt-get -qq -y install build-essential cmake ninja-build clang g++ \
        python3 flex bison jq pkg-config rsync && \
        apt-get autoclean 

# setup container config
USER    actions_runner
EXPOSE  443
COPY    --chmod=0555 ${ACTIONS_RUNNER_SCRIPT} /usr/bin/actions_runner
COPY    --chmod=0444 ${ACTIONS_RUNNER_CONFIG} /etc/actions_runner_conf
WORKDIR /home/actions_runner
ENTRYPOINT     ["/usr/bin/actions_runner"]
