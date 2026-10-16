param(
    [string]$HostIp = "",
    [int]$Port = 8765
)
$ErrorActionPreference = "Stop"
Set-Location $PSScriptRoot

if (-not $HostIp) {
    $ips = @(Get-NetIPAddress -AddressFamily IPv4 |
        Where-Object { $_.IPAddress -notmatch '^(127\.|169\.254\.)' -and $_.InterfaceAlias -notmatch 'vEthernet|WSL|Docker|Loopback' } |
        Select-Object -ExpandProperty IPAddress)
    if ($ips.Count -ne 1) {
        Write-Host "Detected LAN addresses: $($ips -join ', ')"
        throw "Pass the reachable LAN IPv4 explicitly: .\start.ps1 -HostIp 192.168.1.50"
    }
    $HostIp = $ips[0]
}
$parsedIp = $null
if (-not [System.Net.IPAddress]::TryParse($HostIp, [ref]$parsedIp) -or
    $parsedIp.AddressFamily -ne [System.Net.Sockets.AddressFamily]::InterNetwork) {
    throw "HostIp must be a valid IPv4 address"
}
if (-not (Get-Command openssl -ErrorAction SilentlyContinue)) {
    throw "OpenSSL not found in PATH. Install OpenSSL or run this script in an environment with openssl."
}
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "Docker CLI not found in PATH. Install/start Docker Desktop."
}
New-Item -ItemType Directory -Path certs -Force | Out-Null
if (-not (Test-Path certs/server.crt) -or -not (Test-Path certs/server.key)) {
    & openssl req -x509 -newkey rsa:2048 -nodes -days 30 -keyout certs/server.key -out certs/server.crt -subj "/CN=mock-gateway" -addext "subjectAltName=IP:$HostIp,DNS:localhost"
    if ($LASTEXITCODE -ne 0) { throw "OpenSSL failed" }
} else {
    & openssl x509 -in certs/server.crt -noout -ext subjectAltName
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect existing certificate" }
    Write-Warning "Reusing existing certificate. Ensure its SAN includes IP:$HostIp. Delete certs/server.crt and certs/server.key to regenerate."
}
$env:WSS_PORT = "$Port"
& docker compose up --build -d
if ($LASTEXITCODE -ne 0) { throw "Docker Compose failed" }
Write-Host "Mock gateway: wss://$($HostIp):$Port"
Write-Host "Run tests: python -m unittest -v test_wss.py"
