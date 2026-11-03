param(
    [ValidateSet('Early','Latest','AudioTest')][string]$Checkpoint='Latest',
    [string]$ArduinoCli='arduino-cli',
    [switch]$Upload,
    [string]$Port
)
$ErrorActionPreference='Stop'
if($Upload -and [string]::IsNullOrWhiteSpace($Port)) { throw 'Specify -Port with -Upload. Check the current board port first.' }
$root=Split-Path -Parent $PSScriptRoot
$sketches=@{Early='firmware/CyberPlayer';Latest='firmware/StrandPlayer20';AudioTest='diagnostics/PCM5102ToneTest'}
$sketch=Join-Path $root $sketches[$Checkpoint]
$build=Join-Path $root ('build/'+$Checkpoint)
New-Item -ItemType Directory -Force -Path $build | Out-Null
$fqbn='esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,CDCOnBoot=default,UploadSpeed=115200'
$cliArgs=@('compile','--fqbn',$fqbn,'--libraries',(Join-Path $root 'libraries'),'--build-path',$build,$sketch)
& $ArduinoCli @cliArgs
if($LASTEXITCODE -ne 0) { throw 'Compilation failed; upload was not attempted.' }
if($Upload) {
    & $ArduinoCli upload --fqbn $fqbn --port $Port --input-dir $build $sketch
    if($LASTEXITCODE -ne 0) { throw 'Upload failed.' }
} else { Write-Host 'Build complete. Upload requires -Upload -Port <current-port>.' }
