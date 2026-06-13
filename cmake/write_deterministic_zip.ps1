param(
  [Parameter(Mandatory = $true)]
  [string]$SourceDir,

  [Parameter(Mandatory = $true)]
  [string]$ZipPath
)

$ErrorActionPreference = "Stop"

$entries = @(
  "NESticle.exe",
  "SDL3.dll",
  "gui.vol",
  "NESticle.pal",
  "README.md",
  "LICENSE",
  "SDL3-LICENSE.txt",
  "SDL3-PROVENANCE.md",
  "THIRD_PARTY_NOTICES.txt"
)

Add-Type -AssemblyName System.IO.Compression
Add-Type -AssemblyName System.IO.Compression.FileSystem

$fullSource = [System.IO.Path]::GetFullPath($SourceDir)
$fullZip = [System.IO.Path]::GetFullPath($ZipPath)
$zipParent = [System.IO.Path]::GetDirectoryName($fullZip)
[System.IO.Directory]::CreateDirectory($zipParent) | Out-Null

if ([System.IO.File]::Exists($fullZip)) {
  [System.IO.File]::Delete($fullZip)
}

$fixedTime = [System.DateTimeOffset]::Parse("1997-04-03T00:00:00Z")
$archive = [System.IO.Compression.ZipFile]::Open($fullZip, [System.IO.Compression.ZipArchiveMode]::Create)
try {
  foreach ($entryName in $entries) {
    $sourcePath = [System.IO.Path]::Combine($fullSource, $entryName)
    if (-not [System.IO.File]::Exists($sourcePath)) {
      throw "Missing package entry: $sourcePath"
    }

    $entry = $archive.CreateEntry($entryName, [System.IO.Compression.CompressionLevel]::Optimal)
    $entry.LastWriteTime = $fixedTime

    $inputStream = [System.IO.File]::OpenRead($sourcePath)
    try {
      $outputStream = $entry.Open()
      try {
        $inputStream.CopyTo($outputStream)
      } finally {
        $outputStream.Dispose()
      }
    } finally {
      $inputStream.Dispose()
    }
  }
} finally {
  $archive.Dispose()
}
