#!/bin/bash

set -e

SCRIPT_DIR=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )

# goto parent dir
cd "$(dirname "$(dirname "$SCRIPT_DIR")")"

docker run -it --rm \
  -v "$(pwd)":/host \
  -v "$SCRIPT_DIR"/setup_compile_commands.sh:"$SCRIPT_DIR"/setup_compile_commands.sh \
  -v "$SCRIPT_DIR"/impl_setup_compile_commands.sh:"$SCRIPT_DIR"/impl_setup_compile_commands.sh \
  flatimage-boot ./compile_commands/impl_setup_compile_commands.sh
