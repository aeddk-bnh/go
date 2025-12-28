param(
  [Parameter(Mandatory=$true)] [int]$TimeoutSeconds,
  [Parameter(Mandatory=$true, Position=0)] [string]$Command
)

# Runs a command with timeout on PowerShell
# Usage: .\run_with_timeout.ps1 -TimeoutSeconds 300 -Command 'cmake -S . -B build'

$proc = Start-Process -FilePath 'cmd.exe' -ArgumentList '/c', $Command -PassThru -WindowStyle Hidden
# Wait-Process returns $true if process exited, $false if timeout
if (Wait-Process -Id $proc.Id -Timeout $TimeoutSeconds) {
  # process exited; get exit code
  try { $exit = $proc.ExitCode } catch { $exit = 0 }
  exit $exit
} else {
  Write-Error "Command timed out after ${TimeoutSeconds}s; killing process $($proc.Id)"
  Stop-Process -Id $proc.Id -Force -ErrorAction SilentlyContinue
  exit 124
}
