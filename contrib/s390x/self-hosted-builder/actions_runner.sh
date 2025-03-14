#!/bin/bash

#
# action runner startup script.
#
# Expects the following environment variables:
#
# - REPO=<owner>
# - TOKEN=<***>
# - LABEL=<LABELS for this runner>
#

set -e -u
source /etc/actions_runner_conf

# Validate required environment variables
if [ -z "${REPO:-}" ] || [ -z "${TOKEN:-}" ] || [ -z "${LABEL:-}" ]; then
    echo "Error: REPO and/or TOKEN and/or LABEL environment variables not found"
    exit 1
fi

./config.sh \
    --unattended \
    --url "https://github.com/$REPO" \
    --token "$TOKEN" \
    --disableupdate \
    --name "${LABEL}_actions_runner" \
    --replace \
    --labels "$LABEL"

# Run the actions-runner
./run.sh
