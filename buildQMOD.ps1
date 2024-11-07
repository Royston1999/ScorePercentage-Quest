Param(
    [Parameter(Mandatory=$false)]
    [String] $qmodName="ScorePercentage",

    [Parameter(Mandatory=$false)]
    [Switch] $clean,

    [Parameter(Mandatory=$false)]
    [Switch] $help
)

if ($help -eq $true) {
    echo "`"BuildQmod <qmodName>`" - Copiles your mod into a `".so`" or a `".a`" library"
    echo "`n-- Parameters --`n"
    echo "qmodName `t The file name of your qmod"

    echo "`n-- Arguments --`n"

    echo "-Clean `t`t Performs a clean build on both your library and the qmod"

    exit
}

$mod = "./mod.json"

qpm qmod manifest

$modJson = Get-Content $mod -Raw | ConvertFrom-Json

if ($qmodName -eq "")
{
    echo "Give a proper qmod name and try again"
    exit
}

$filelist = @($mod)

$cover = "./" + $modJson.coverImage
if ((-not ($cover -eq "./")) -and (Test-Path $cover))
{ 
    $filelist += ,$cover
}

foreach ($mod in $modJson.lateModFiles)
{
    $path = "./build/" + $mod
    if (-not (Test-Path $path))
    {
        $path = "./extern/libs/" + $mod
    }
    $filelist += $path
}

foreach ($lib in $modJson.libraryFiles)
{
    $path = "./extern/libs/" + $lib
    if (-not (Test-Path $path))
    {
        $path = "./build/" + $lib
    }
    $filelist += $path
}

$zip = $qmodName + ".zip"
$qmod = $qmodName + ".qmod"

if ((-not ($clean.IsPresent)) -and (Test-Path $qmod))
{
    echo "Making Clean Qmod"
    Move-Item $qmod $zip -Force
}

Compress-Archive -Path $filelist -DestinationPath $zip -Update
Move-Item $zip $qmod -Force