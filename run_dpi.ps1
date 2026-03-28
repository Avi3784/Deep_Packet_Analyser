param(
    [string]$InputPcap = "test_dpi.pcap",
    [string]$OutputPcap = "output_terminal_visible.pcap",
    [int]$MaxPackets = 100,
    [string]$ModelPath = "models/traffic_dt_model.txt",
    [string]$RulesFile = ""
)

$ErrorActionPreference = "Stop"

# Force UTF-8 terminal rendering so box-drawing characters display correctly.
chcp 65001 > $null
[Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)

$exePath = Join-Path $PSScriptRoot "build\dpi_engine.exe"
if (-not (Test-Path $exePath)) {
    throw "Executable not found: $exePath"
}

if ([string]::IsNullOrWhiteSpace($RulesFile)) {
    & $exePath $InputPcap $OutputPcap $MaxPackets --ml-model $ModelPath
} else {
    & $exePath $InputPcap $OutputPcap $MaxPackets --ml-model $ModelPath --rules $RulesFile
}
exit $LASTEXITCODE
