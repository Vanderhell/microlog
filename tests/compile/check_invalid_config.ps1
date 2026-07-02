$ErrorActionPreference = "Stop"

param(
    [string]$CC = "gcc",
    [string]$CFlags = "-std=c99 -Wall -Wextra -Wpedantic -Werror -I../include"
)

$cases = @(
    @{ File = "invalid_config_max_backends_zero.c"; Pattern = "MLOG_MAX_BACKENDS" },
    @{ File = "invalid_config_buf_size_zero.c"; Pattern = "MLOG_BUF_SIZE" },
    @{ File = "invalid_config_level_min_range.c"; Pattern = "MLOG_LEVEL_MIN" },
    @{ File = "invalid_config_color_toggle.c"; Pattern = "MLOG_ENABLE_COLOR" },
    @{ File = "invalid_config_timestamp_toggle.c"; Pattern = "MLOG_ENABLE_TIMESTAMP" }
)

$failed = $false

foreach ($case in $cases) {
    $source = Join-Path $PSScriptRoot $case.File
    $object = Join-Path $PSScriptRoot "$($case.File).o"
    $output = & $CC @($CFlags.Split(" ")) "-c" $source "-o" $object 2>&1
    if ($LASTEXITCODE -eq 0) {
        Write-Error "Expected compile failure for $($case.File), but compilation succeeded."
    }
    if (($output | Out-String) -notmatch $case.Pattern) {
        Write-Error "Expected diagnostic mentioning $($case.Pattern) for $($case.File)."
    }
}
