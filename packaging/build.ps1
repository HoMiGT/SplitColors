param(
    [string]$VcpkgRoot = 'D:\vcpkg',
    [string]$InnoCompiler = 'C:\Users\hm_la\AppData\Local\Programs\Antigravity IDE\_\resources\app\node_modules\innosetup\bin\ISCC.exe'
)
$ErrorActionPreference = 'Stop'
$project = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$stage = Join-Path $PSScriptRoot 'stage'
$bin = Join-Path $VcpkgRoot 'installed\x64-windows\bin'
$include = Join-Path $VcpkgRoot 'installed\x64-windows\include\opencv4'
$library = Join-Path $VcpkgRoot 'installed\x64-windows\lib\opencv_world4.lib'
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue) -or -not (Get-Command dumpbin.exe -ErrorAction SilentlyContinue) -or -not $env:VCToolsRedistDir) {
    throw 'Run this script from an x64 Visual Studio Developer PowerShell or Command Prompt.'
}
$crt = Join-Path $env:VCToolsRedistDir 'x64\Microsoft.VC143.CRT'
if (-not (Test-Path -LiteralPath $InnoCompiler)) { throw "Inno Setup compiler not found: $InnoCompiler" }
New-Item -ItemType Directory -Path $stage -Force | Out-Null
# Remove generated binaries only; retain the installer configuration and licenses.
Get-ChildItem -LiteralPath $stage -File | Where-Object { $_.Extension -in '.dll','.exe','.obj' } | Remove-Item -Force
Push-Location $project
try {
    & cl.exe /nologo /EHsc /std:c++20 /O2 /MD "/I$include" main.cpp config.cpp image_processing.cpp "/Fe:$stage\SplitColors.exe" "/Fo:$stage\" /link $library
    if ($LASTEXITCODE -ne 0) { throw 'Release build failed.' }
    $lookup = @{}
    foreach ($directory in @($bin,$crt)) {
        Get-ChildItem -LiteralPath $directory -Filter '*.dll' -File | ForEach-Object { $lookup[$_.Name.ToLowerInvariant()] = $_.FullName }
    }
    $queue = [System.Collections.Generic.Queue[string]]::new()
    $seen = [System.Collections.Generic.HashSet[string]]::new([System.StringComparer]::OrdinalIgnoreCase)
    $queue.Enqueue((Join-Path $stage 'SplitColors.exe'))
    while ($queue.Count -gt 0) {
        $binary = $queue.Dequeue()
        $dependencies = & dumpbin.exe /dependents $binary
        if ($LASTEXITCODE -ne 0) { throw "Cannot inspect $binary" }
        foreach ($line in $dependencies) {
            if ($line -match '^\s+([\w.-]+\.dll)\s*$') {
                $name = $Matches[1].ToLowerInvariant()
                if ($lookup.ContainsKey($name) -and $seen.Add($name)) {
                    Copy-Item -LiteralPath $lookup[$name] -Destination $stage -Force
                    $queue.Enqueue($lookup[$name])
                }
            }
        }
    }
    foreach ($package in @('opencv4','protobuf','abseil','libjpeg-turbo','libpng','zlib','tiff','libwebp','liblzma')) {
        $license = Join-Path $VcpkgRoot "installed\x64-windows\share\$package\copyright"
        if (Test-Path -LiteralPath $license) {
            $destination = Join-Path $stage "licenses\$package"
            New-Item -ItemType Directory -Path $destination -Force | Out-Null
            Copy-Item -LiteralPath $license -Destination (Join-Path $destination 'copyright.txt') -Force
        }
    }
    & $InnoCompiler (Join-Path $PSScriptRoot 'installer.iss')
    if ($LASTEXITCODE -ne 0) { throw 'Installer build failed.' }
} finally { Pop-Location }
