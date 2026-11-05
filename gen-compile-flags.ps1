# Writes <assembly>/compile_flags.txt (one flag per line) for clangd.
# Usage: gen-compile-flags.ps1 -Assembly <assembly> -Flags "<include flags and defines>"
# Relative include paths are relative to the repo root in the build, but clangd resolves
# them relative to compile_flags.txt, so they are rewritten to start with "../".
param(
    [Parameter(Mandatory = $true)][string]$Assembly,
    [Parameter(Mandatory = $true)][string]$Flags
)

$lines = $Flags -split '\s+' |
    Where-Object { $_ -ne '' } |
    ForEach-Object {
        $flag = $_ -replace '\\', '/'
        if ($flag -match '^-I(.+)$') {
            $dir = $Matches[1]
            if ($dir -match '^(\.\./|/|[A-Za-z]:)') {
                # Already absolute or already relative to the assembly folder.
            } elseif ($dir -match '^\./(.*)$') {
                $dir = '../' + $Matches[1]
            } else {
                $dir = '../' + $dir
            }
            $flag = "-I$dir"
        }
        $flag
    }

$path = Join-Path $Assembly 'compile_flags.txt'
[System.IO.File]::WriteAllLines($path, [string[]]$lines, (New-Object System.Text.UTF8Encoding($false)))
