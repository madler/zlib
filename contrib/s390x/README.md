# General Notes

> [!NOTE]  
> This is a modified and simplified copy of the s390x self-hosted action runner scripts from here:
> - https://github.com/zlib-ng/zlib-ng/tree/a0fa24710c8faa1a746a20cfd5c7c24571e15ca4/arch/s390/self-hosted-builder
> - https://github.com/anup-kodlekere/gaplib
>
> **This code does NOT setup ephemeral workers!!**. It means that the worker context will be reused between CI runs and never be destroyed. 
>
> The implications are:
> - one needs to check a PR that does not leak the /etc/actions_runner_conf
> - one needs to check a PR that it does not alter the system in a persistent way
> - one needs to check a PR that it does not access any file outside of the build folder
>
> To create ephemeral workers we need to setup a token request process similar like zlib-ng has done. But such a process implies that the worker maintainer needs, not just commit, but administration rights to the repository.

## Self Hosted Runner

Given complexity of Vector and DFLTCC machine instruction, it is not clear whether QEMU TCG will ever support it. At the time of writing, one has to have access to an IBM z15+ VM or LPAR in order to test the support. Since DFLTCC and Vector instructions are non-privileged, neither special VM/LPAR configuration nor root are required.

The CI uses a self-hosted builder, provided by marist university. There is no official IBM Z GitHub Actions runner, so we build one inspired by anup-kodlekere/gaplib. Future updates to actions-runner might need an updated patch. The .net version number patch has been separated into a separate file to avoid a need for constantly changing the patch.

### Setup

1. request an account at https://linuxone.cloud.marist.edu/#/login and create a VM
2. install podman:

```
sudo apt update
sudo apt upgrade
sudo apt install podman
```

3. request an action runner authentication token via the github project website: 
```
Settings->Actions->Runners->New self-hosted runner
```
4. checkout zlib and setup the token in the actions_runner_conf.sh file:
```
git clone https://github.com/fneddy/zlib.git -b s390x_native_ci
```
```
cd zlib/contrib/s390x/self-hosted-builder/
nano actions_runner_conf.sh

# add the created token to the conf:
# REPO="madler/zlib"
# TOKEN="AAGXXXXXXXXXXXXXXXXXXXXXXXXXX"
# LABEL="S390X"
```
5. build the action runner container:
```
podman build --squash -f actions_runner_ubuntu.Dockerfile --tag zlib/actions_runner .
```
6. start the actions runner via either:

- directly running it from cli
```
podman run --rm -it zlib/actions_runner
```
- let systemd quadlet hooks start it automatically
```
mkdir -p $HOME/.config/containers/systemd/
cp actions_runner.container $HOME/.config/containers/systemd/
systemctl --user daemon-reload
systemctl --user start zlib_actions_runner
sudo -E loginctl enable-linger $USER
```
