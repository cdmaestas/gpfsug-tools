#!/bin/bash

################################################################################
# Name:            backupsnapassoc.sh
# Author:          Achim Christ - achim(dot)christ(at)gmail(dot)com
# Version:         1.0
################################################################################

# Commandline arguments:
#   --delete       - Also delete snapshot associations

# Return codes:
#   0              - Success
#   1              - Error deleting snapshot associations
#   2              - Warning, nothing to do

# Fail loudly: exit on error, error on unset variables, and surface failures
# from any stage of a pipeline (not just the last command).
set -euo pipefail

# Echo to stderr
echoerr() { echo "$@" 1>&2; }

# Constants
readonly GUI_PATH="/usr/lpp/mmfs/gui/cli"

# Use sudo if not running as root
PREFIX=""
if [ "$(whoami)" != "root" ]; then
  PREFIX="sudo "
fi

# Read snapshot associations. `grep -v` returns 1 when every line is a HEADER
# (i.e. no associations); tolerate that here so the emptiness check below can
# emit the intended "nothing to do" exit 2 rather than tripping `set -e`.
# shellcheck disable=SC2086  # $PREFIX is "sudo " (or "") and is meant to word-split.
OUT=$(${PREFIX}${GUI_PATH}/lssnapassoc -Y | grep -v "HEADER" || true)

# Check if any snapshot associations exist
if [ -z "$OUT" ]; then
  echoerr "No snapshot schedule associations found!"
  exit 2
fi

# Write script header to stdout
echo "#!/bin/bash"

# Parse snapshot associations
echo "$OUT" | while read -r line || [ -n "$line" ]; do

  # Extract details
  DEVICE=$(echo "$line" | cut -d ':' -f 8)
  FILESET=$(echo "$line" | cut -d ':' -f 9)
  RULE=$(echo "$line" | cut -d ':' -f 10)

  # Compile backup command
  BKP_CMD="${PREFIX}${GUI_PATH}/mksnapassoc ${DEVICE} ${RULE}"

  # Compile delete command
  DEL_CMD="${PREFIX}${GUI_PATH}/rmsnapassoc ${DEVICE} ${RULE}"

  # Optionally append fileset to commands
  if [ -n "$FILESET" ]; then
    BKP_CMD+=" -j ${FILESET}"
    DEL_CMD+=" -j ${FILESET}"
  fi

  # Append further parameters to delete command
  DEL_CMD+=" -k -f &> /dev/null"

  # Write backup command to stdout
  echo "$BKP_CMD"

  # Optionally run delete command
  if [ "${1:-}" == "--delete" ]; then
    if ! eval "$DEL_CMD"; then
      echoerr "Error deleting snapshot association - try running following command manually:"
      echoerr "$DEL_CMD"
      exit 1
    fi
  fi

done

exit 0
