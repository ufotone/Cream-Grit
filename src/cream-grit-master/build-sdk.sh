#!/usr/bin/env bash
set -eo pipefail
source /app/env
env drumlogue
cd /workspace/drumlogue/cream-grit-master
make -j2
make install
