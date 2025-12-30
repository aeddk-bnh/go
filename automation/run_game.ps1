$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = Join-Path $PWD 'go_console.exe'
$psi.UseShellExecute = $false
$psi.RedirectStandardInput = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$proc = New-Object System.Diagnostics.Process
$proc.StartInfo = $psi
$started = $proc.Start()
$in = [System.IO.File]::ReadAllText('D:\go\automation\game1.txt')
$proc.StandardInput.Write($in)
$proc.StandardInput.Close()
$out = $proc.StandardOutput.ReadToEnd()
$err = $proc.StandardError.ReadToEnd()
$proc.WaitForExit()
Write-Output "=== STDOUT ==="
Write-Output $out
Write-Output "=== STDERR ==="
Write-Output $err
Write-Output "=== EXIT CODE: $($proc.ExitCode) ===" 
