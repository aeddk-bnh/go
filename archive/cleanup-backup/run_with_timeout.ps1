param(
  [Parameter(Mandatory=$true)] [int]$TimeoutSeconds,
  [Parameter(Mandatory=$true, Position=0)] [string]$Command
)

# Runs a command with timeout on PowerShell
# Usage: .\run_with_timeout.ps1 -TimeoutSeconds 300 -Command 'C:\msys64\usr\bin\bash.exe -lc "..."'

# Split command into executable and args (first space)
$firstSpace = $Command.IndexOf(' ')
if ($firstSpace -ge 0) {
  $exe = $Command.Substring(0, $firstSpace)
  $args = $Command.Substring($firstSpace + 1)
} else {
  $exe = $Command
  $args = ''
}

# If exe path exists, start it directly to ensure Wait-Process tracks the correct process
if (Test-Path $exe) {
  Write-Host "Starting executable: $exe $args"
  $proc = Start-Process -FilePath $exe -ArgumentList $args -PassThru -NoNewWindow
} else {
  # Fallback to cmd.exe /c when exe not found
  Write-Host "Executable not found, running via cmd.exe: $Command"
  $proc = Start-Process -FilePath 'cmd.exe' -ArgumentList '/c', $Command -PassThru -NoNewWindow
}

# Wait-Process returns $true if process exited, $false if timeout
if (Wait-Process -Id $proc.Id -Timeout $TimeoutSeconds) {
  try { $exit = $proc.ExitCode } catch { $exit = 0 }
  exit $exit
} else {
  Write-Error "Command timed out after ${TimeoutSeconds}s; killing process tree for PID $($proc.Id)"
  # Attempt to kill process tree using taskkill (best-effort)
  try { & taskkill /PID $proc.Id /T /F } catch { Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue }
  exit 124
}
