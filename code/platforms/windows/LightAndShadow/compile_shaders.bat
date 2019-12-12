IF EXIST "build/basic_vert.spv" (DEL "build/basic_vert.spv")
IF EXIST "build/basic_frag.spv" (DEL "build/basic_frag.spv")

"VulkanSDK 1.1.121.2/Bin/glslc.exe" VulkanParticleSystem/basic.vert -o build/basic_vert.spv
IF %ERRORLEVEL% NEQ 0 (pause)

"VulkanSDK 1.1.121.2/Bin/glslc.exe" VulkanParticleSystem/basic.frag -o build/basic_frag.spv
IF %ERRORLEVEL% NEQ 0 (pause)
