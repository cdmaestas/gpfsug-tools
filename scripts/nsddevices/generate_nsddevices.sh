#!/bin/sh

# Author:  Jez Tucker
# Date:    12/04/2012
# Version: 1.0

# Generates the partition list in the correct format
# Tested on Linux
# Add the output to the correct section of the nsddevices file in /var/mmfs/etc/

# Exit on error / unset variable. (pipefail is not available in POSIX sh.)
set -eu

echo "-- Cut here --"

# `awk` replaces the cat|grep|awk pipeline; `grep -E` replaces the deprecated
# egrep; testing grep directly replaces the `$?`-after-command idiom.
# shellcheck disable=SC2013  # device names are single tokens; word-splitting is intended.
for PART in $(awk '/sd/ {print $4}' /proc/partitions | sort); do
  if echo "$PART" | grep -qE '[0-9]'; then
    echo "echo \"$PART gpt\""
  else
    echo "echo \"$PART generic\""
  fi
done

echo "-- Cut here --"

# EOF
