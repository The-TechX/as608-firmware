param(
    [string]$HostIp = "192.168.1.66",
    [int]$Port = 8765
)
$ErrorActionPreference = "Stop"
$root = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$certPath = Join-Path $root "tests/integration/certs/server.crt"
if (-not (Test-Path $certPath)) { throw "Certificate not found: $certPath. Run tests/integration/start.ps1 first." }
$cert = (Get-Content -Raw $certPath).Trim()
if ($cert -notmatch '-----BEGIN CERTIFICATE-----' -or $cert -notmatch '-----END CERTIFICATE-----') {
    throw "Invalid PEM certificate"
}
if ($HostIp -notmatch '^\d{1,3}(\.\d{1,3}){3}$') { throw "HostIp must be an IPv4 address" }
$output = @"
#pragma once
// Generated local development configuration. Do not commit.
static constexpr char kWssHost[] = "$HostIp";
static constexpr uint16_t kWssPort = $Port;
static const char kWssCaCert[] = R"PEM(
$cert
)PEM";
"@
$path = Join-Path $root "include/transport/wss_local_config.h"
[System.IO.File]::WriteAllText($path, $output, (New-Object System.Text.UTF8Encoding($false)))
Write-Host "Generated $path"
Write-Host "ESP32 target: wss://${HostIp}:${Port}"
