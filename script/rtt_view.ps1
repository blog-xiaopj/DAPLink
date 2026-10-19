# Attach CMSIS-DAP, print RTT channel 0. No reset, no halt.
$ErrorActionPreference = "Stop"

$scriptDir = $PSScriptRoot
$outLog = Join-Path $env:TEMP "openocd_rtt.out"
$errLog = Join-Path $env:TEMP "openocd_rtt.err"
$rttPort = 9090

$openocd = Get-Command openocd -ErrorAction SilentlyContinue
if (-not $openocd) {
	Write-Host "openocd not found in PATH"
	exit 1
}

# $cfgAdapter = Join-Path $scriptDir "stlink-dap.cfg"	# DAPLink 仿真器配置
$cfgAdapter = Join-Path $scriptDir "cmsis-dap.cfg"	# CMSIS-DAP 仿真器配置
# $cfgAdapter = Join-Path $scriptDir "jlink.cfg"	# JLink 仿真器配置

$cfgTarget = Join-Path $scriptDir "stm32f1x.cfg"	# 目标芯片配置
$cfgRtt = Join-Path $scriptDir "rtt.cfg"	# RTT 配置

Write-Host "Starting OpenOCD RTT (no reset / no halt) ..."
$proc = Start-Process -FilePath $openocd.Source `
	-ArgumentList @("-f", $cfgAdapter, "-f", $cfgTarget, "-f", $cfgRtt) `
	-NoNewWindow -PassThru `
	-RedirectStandardOutput $outLog `
	-RedirectStandardError $errLog

$client = $null
try {
	for ($i = 0; $i -lt 50; $i++) {
		if ($proc.HasExited) {
			break
		}
		try {
			$client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", $rttPort)
			break
		} catch {
			Start-Sleep -Milliseconds 200
		}
	}

	if (-not $client) {
		Write-Host "Cannot connect RTT 127.0.0.1:$rttPort"
		if (Test-Path $errLog) {
			Write-Host "---- OpenOCD log ----"
			Get-Content $errLog
		}
		if (Test-Path $outLog) {
			Get-Content $outLog
		}
		exit 1
	}

	Write-Host "RTT connected 127.0.0.1:$rttPort  (Ctrl+C to stop)"
	Write-Host "--------------------------------"

	$stream = $client.GetStream()
	$client.ReceiveTimeout = 200
	$buffer = New-Object byte[] 1024
	$utf8 = [System.Text.Encoding]::UTF8

	while (-not $proc.HasExited) {
		try {
			$n = $stream.Read($buffer, 0, $buffer.Length)
			if ($n -le 0) {
				break
			}
			Write-Host -NoNewline ($utf8.GetString($buffer, 0, $n))
		} catch [System.IO.IOException] {
			# read timeout, keep waiting
		}
	}
} finally {
	if ($client) {
		$client.Close()
	}
	if ($proc -and -not $proc.HasExited) {
		Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
	}
}