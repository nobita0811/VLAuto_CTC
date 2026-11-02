# BuildRelease.ps1 : build 3 ban phat hanh VLCTCPro.exe + VLCTCdll.dll (Release|Win32)
#   User          ADMIN_MODE=0                 nguoi dung: can license, khong co DEBUG
#   Tester        ADMIN_MODE=1 ADMIN_TESTER=1  tester online: co DEBUG, can license
#   TesterOffline ADMIN_MODE=1 ADMIN_TESTER=0  tester offline: co DEBUG, khong can license
# Ket qua: _Bin\Publish\<ban>\  (obj rieng o _Obj\Publish\<ban>\). Khong dung toi _Bin\VLCTCPro.exe / _Obj cua VS.
# Cach chay:  powershell -ExecutionPolicy Bypass -File BuildRelease.ps1 [-Only User,Tester,TesterOffline]
param([string[]]$Only = @('User', 'Tester', 'TesterOffline'))

$ErrorActionPreference = 'Stop'
$Root = $PSScriptRoot
$Variants = [ordered]@{
	User          = '/DADMIN_MODE=0 /DADMIN_TESTER=0'
	Tester        = '/DADMIN_MODE=1 /DADMIN_TESTER=1'
	TesterOffline = '/DADMIN_MODE=1 /DADMIN_TESTER=0'
}

# Phase 2.3: post-build stamper nhung CRC file (no-op neu binary khong co magic,
# vd ban TesterOffline LP_HARDEN=0). Khong co node -> bo qua (runtime fail-open).
$FileChk = Join-Path $Root '..\LPROTECT\tools\filechk\patch_filechk.js'

$VsWhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
$MSBuild = & $VsWhere -latest -requires Microsoft.Component.MSBuild -find 'MSBuild\**\Bin\MSBuild.exe' | Select-Object -First 1
if (-not $MSBuild) { throw 'Khong tim thay MSBuild.exe' }

# VLAuto link VLCTCdll.lib o _Obj\VLCTCdll\Release (bang export giong nhau o moi ban): chua co thi build VLHook thuong 1 lan
if (-not (Test-Path "$Root\_Obj\VLCTCdll\Release\VLCTCdll.lib")) {
	& $MSBuild "$Root\VLHook\VLHook.vcxproj" /nologo /v:m /p:Configuration=Release /p:Platform=Win32 "/p:SolutionDir=$Root\"
	if ($LASTEXITCODE) { throw 'Build VLHook (Release) loi' }
}

$OldCL = $env:CL
try {
	foreach ($Name in $Variants.Keys) {
		if ($Only -notcontains $Name) { continue }
		Write-Host "===== $Name ($($Variants[$Name])) =====" -ForegroundColor Cyan
		$env:CL = $Variants[$Name]	# cl.exe doc them tuy chon tu bien moi truong CL
		$Out = "$Root\_Bin\Publish\$Name\"
		foreach ($Proj in @('VLHook', 'VLAuto')) {
			& $MSBuild "$Root\$Proj\$Proj.vcxproj" /t:Rebuild /nologo /v:m /p:Configuration=Release /p:Platform=Win32 `
				"/p:SolutionDir=$Root\" "/p:OutDir=$Out" "/p:IntDir=$Root\_Obj\Publish\$Name\$Proj\" /p:BuildProjectReferences=false
			if ($LASTEXITCODE) { throw "Build $Proj ($Name) loi" }
		}
		$Exe = Join-Path $Out 'VLCTCPro.exe'
			if ((Test-Path $FileChk) -and (Test-Path $Exe)) {   # Phase 2.3: nhung CRC file (buoc CUOI)
				try { & node $FileChk $Exe } catch { Write-Warning "filechk skip ($Name): $_" }
			}
			Get-ChildItem $Out -Include *.exe, *.dll -Recurse | ForEach-Object { Write-Host ("  {0}  {1:N0} bytes" -f $_.FullName, $_.Length) }
	}
}
finally {
	$env:CL = $OldCL
}
