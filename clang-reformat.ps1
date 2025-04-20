$fileTypes = '*.cpp', '*.h'
$sourceDirs = 'src'
$exclude =

$clangFormatFile = "file"
if ($args.Count -ge 1) {
    $clangFormatFile = "file:" + $args[0]
}

$scriptDir = Split-Path -Path $PSCommandPath
$cores = (Get-CimInstance Win32_ComputerSystem).NumberOfLogicalProcessors

foreach ($dir in $sourceDirs) {
    Get-ChildItem -Path (Join-Path $scriptDir $dir) -Recurse -Include $fileTypes -Exclude $exclude |
    Foreach-Object -ThrottleLimit $cores -Parallel {
        & clang-format $cmdArgs --style=$using:clangFormatFile --verbose -i $_.FullName
    }
}
