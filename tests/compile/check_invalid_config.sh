#!/bin/sh
set -eu

CC_CMD=${CC:-gcc}
CFLAGS_STR=${CFLAGS:-"-std=c99 -Wall -Wextra -Wpedantic -Werror -I../include"}

run_case() {
    file="$1"
    pattern="$2"
    source_file="$script_dir/$file"
    object_file="$script_dir/${file}.o"

    set +e
    output=$($CC_CMD $CFLAGS_STR -c "$source_file" -o "$object_file" 2>&1)
    status=$?
    set -e

    if [ "$status" -eq 0 ]; then
        echo "Expected compile failure for $file, but compilation succeeded." >&2
        exit 1
    fi

    echo "$output" | grep -F "$pattern" >/dev/null 2>&1 || {
        echo "Expected diagnostic mentioning $pattern for $file." >&2
        echo "$output" >&2
        exit 1
    }
}

script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)

run_case "invalid_config_max_backends_zero.c" "MLOG_MAX_BACKENDS"
run_case "invalid_config_buf_size_zero.c" "MLOG_BUF_SIZE"
run_case "invalid_config_level_min_range.c" "MLOG_LEVEL_MIN"
run_case "invalid_config_color_toggle.c" "MLOG_ENABLE_COLOR"
run_case "invalid_config_timestamp_toggle.c" "MLOG_ENABLE_TIMESTAMP"
