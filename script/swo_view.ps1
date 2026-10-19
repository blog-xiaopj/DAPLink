# Attach CMSIS-DAP, print SWO ITM port 0. No reset, no halt. SWD only.
$ErrorActionPreference = "Stop"

$scriptDir = $PSScriptRoot
$outLog = Join-Path $env:TEMP "openocd_swo.out"
$errLog = Join-Path $env:TEMP "openocd_swo.err"
$swoPort = 9091

$openocd = Get-Command openocd -ErrorAction SilentlyContinue
if (-not $openocd) {
	Write-Host "openocd not found in PATH"
	exit 1
}

# $cfgAdapter = Join-Path $scriptDir "stlink-dap.cfg"	# DAPLink 仿真器配置
$cfgAdapter = Join-Path $scriptDir "cmsis-dap.cfg"	# CMSIS-DAP 仿真器配置
# $cfgAdapter = Join-Path $scriptDir "jlink.cfg"	# JLink 仿真器配置

$cfgTarget = Join-Path $scriptDir "stm32f1x.cfg"	# 目标芯片配置
$cfgSwo = Join-Path $scriptDir "swo.cfg"	# SWO 配置

Write-Host "Starting OpenOCD SWO (no reset / no halt, SWD) ..."
$proc = Start-Process -FilePath $openocd.Source `
	-ArgumentList @("-f", $cfgAdapter, "-f", $cfgTarget, "-f", $cfgSwo) `
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
			$client = New-Object System.Net.Sockets.TcpClient("127.0.0.1", $swoPort)
			break
		} catch {
			Start-Sleep -Milliseconds 200
		}
	}

	if (-not $client) {
		Write-Host "Cannot connect SWO 127.0.0.1:$swoPort"
		if (Test-Path $errLog) {
			Write-Host "---- OpenOCD log ----"
			Get-Content $errLog
		}
		if (Test-Path $outLog) {
			Get-Content $outLog
		}
		exit 1
	}

	Write-Host "SWO connected 127.0.0.1:$swoPort  (Ctrl+C to stop)"
	Write-Host "--------------------------------"

	$stream = $client.GetStream()
	$client.ReceiveTimeout = 200
	$buffer = New-Object byte[] 1024
	# ITM: 0=等头, 1=等 1 字节载荷; skip=还需丢掉的载荷字节
	$itmWait = 0
	$itmSkip = 0

	while (-not $proc.HasExited) {
		try {
			$n = $stream.Read($buffer, 0, $buffer.Length)
			if ($n -le 0) {
				break
			}
			for ($k = 0; $k -lt $n; $k++) {
				$b = $buffer[$k]
				if ($itmSkip -gt 0) {
					$itmSkip--
					continue
				}
				if ($itmWait -eq 1) {
					Write-Host -NoNewline ([char]$b)
					$itmWait = 0
					continue
				}
				if ($b -eq 0) {
					continue
				}
				$sizeBits = $b -band 3
				$port = $b -shr 3
				if ($sizeBits -eq 0) {
					continue
				}
				if (($port -eq 0) -and ($sizeBits -eq 1)) {
					$itmWait = 1
				} else {
					if ($sizeBits -eq 3) {
						$itmSkip = 4
					} else {
						$itmSkip = $sizeBits
					}
				}
			}
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
