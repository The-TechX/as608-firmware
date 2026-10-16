param(
    [string]$HostIp = "",
    [int]$Port = 8765,
    [string]$OpenSslPath = ""
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
# Resolve OpenSSL from an explicit path, PATH, or common Git for Windows locations.
if ($OpenSslPath) {
    if (-not (Test-Path -LiteralPath $OpenSslPath -PathType Leaf)) {
        throw "OpenSSL executable not found at: $OpenSslPath"
    }
    $OpenSslExe = (Resolve-Path -LiteralPath $OpenSslPath).Path
} else {
    $command = Get-Command openssl.exe -ErrorAction SilentlyContinue
    if ($command) {
        $OpenSslExe = $command.Source
    } else {
        $candidates = @(
            (Join-Path $env:ProgramFiles "Git\\usr\\bin\\openssl.exe"),
            (Join-Path $env:ProgramFiles "Git\\mingw64\\bin\\openssl.exe"),
            (Join-Path ${env:ProgramFiles(x86)} "Git\\usr\\bin\\openssl.exe"),
            (Join-Path $env:LOCALAPPDATA "Programs\\Git\\usr\\bin\\openssl.exe"),
            (Join-Path $HOME "scoop\\apps\\git\\current\\usr\\bin\\openssl.exe"),
            (Join-Path $HOME "scoop\\apps\\git\\current\\mingw64\\bin\\openssl.exe")
        )
        $OpenSslExe = $candidates | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf } | Select-Object -First 1
    }
}
if (-not $OpenSslExe) {
    throw "OpenSSL not found. Pass -OpenSslPath 'C:\\path\\to\\openssl.exe' or install OpenSSL."
}
Write-Host "Using OpenSSL: $OpenSslExe"
if (-not (Get-Command docker -ErrorAction SilentlyContinue)) {
    throw "Docker CLI not found in PATH. Install/start Docker Desktop."
}
New-Item -ItemType Directory -Path certs -Force | Out-Null
if (-not (Test-Path certs/server.crt) -or -not (Test-Path certs/server.key)) {
    & $OpenSslExe req -x509 -newkey rsa:2048 -nodes -days 30 -keyout certs/server.key -out certs/server.crt -subj "/CN=mock-gateway" -addext "subjectAltName=IP:$HostIp,DNS:localhost" -addext "basicConstraints=critical,CA:TRUE" -addext "keyUsage=critical,keyCertSign,cRLSign,digitalSignature,keyEncipherment" -addext "extendedKeyUsage=serverAuth"
    if ($LASTEXITCODE -ne 0) { throw "OpenSSL failed" }
} else {
    & $OpenSslExe x509 -in certs/server.crt -noout -ext subjectAltName
    if ($LASTEXITCODE -ne 0) { throw "Cannot inspect existing certificate" }
    Write-Warning "Reusing existing certificate. Ensure its SAN includes IP:$HostIp. Delete certs/server.crt and certs/server.key to regenerate."
}
$env:WSS_PORT = "$Port"
& docker compose up --build -d
if ($LASTEXITCODE -ne 0) { throw "Docker Compose failed" }
Write-Host "Mock gateway: wss://$($HostIp):$Port"
Write-Host "Run tests: docker compose run --build --rm integration-tests"
