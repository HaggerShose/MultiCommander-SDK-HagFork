ImageDimensionsPlugin - Multi Commander FileProperties extension

-------------------------------------------------------------------------------
Build from terminal (Visual Studio or Build Tools installed)
-------------------------------------------------------------------------------

You need MSBuild and the MSVC toolset (v143) on PATH. Easiest: open
"Developer PowerShell for VS 2022" or "x64 Native Tools Command Prompt for VS 2022"
from the Start menu (installed with Visual Studio / Build Tools).

Then:

  cd path\to\MultiCommander-SDK-HagFork\ImageDimensionsPlugin

  msbuild ImageDimensionsPlugin.vcxproj /p:Configuration=Release /p:Platform=x64

Clean rebuild:

  msbuild ImageDimensionsPlugin.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64

Other variants (if you need them):

  msbuild ImageDimensionsPlugin.vcxproj /p:Configuration=Debug /p:Platform=x64
  msbuild ImageDimensionsPlugin.vcxproj /p:Configuration=Release /p:Platform=Win32

Output (linker): Bin\<Platform>\<Configuration>\ImageDimensionsPlugin.dll

After each successful build or rebuild, a post-build step copies that DLL to
this project folder (next to ImageDimensionsPlugin.vcxproj) so you can grab it
without digging through Bin\...

If msbuild is not found, run it by full path.
	`& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" ImageDimensionsPlugin.vcxproj /p:Configuration=Release /p:Platform=x64`
	`& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" ImageDimensionsPlugin.vcxproj /t:Rebuild /p:Configuration=Release /p:Platform=x64`
