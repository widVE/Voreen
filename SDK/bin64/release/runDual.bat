if "%ComputerName%" =="C6_V1_HEAD" (
	start S:\apps\dual_test\Voreen\x64\Release\FionaVoreen.exe configFile C:\Users\Admin2\Desktop\FionaConfigDual.txt navigationSpeed 0.02 rotationSpeed 0.015 --file ..\..\..\..\..\..\rtredinnick\volumes\vis_human_color.vws
) else (
	start S:\apps\dual_test\Voreen\x64\Release\FionaVoreen.exe configFile C:\Users\Admin2\Desktop\FionaConfigDP.txt windowX 0 windowY 0 windowW 1920 windowH 1920 navigationSpeed 0.02 rotationSpeed 0.015 kevinOffset -0.033 -0.0266 0.041 --file ..\..\..\..\..\..\rtredinnick\volumes\vis_human_color.vws
	TIMEOUT 3
	start S:\apps\dual_test\Voreen\x64\Release\FionaVoreen.exe configFile C:\Users\Admin2\Desktop\FionaConfigDP.txt windowX 1920 windowY 0 windowW 1920 windowH 1920 navigationSpeed 0.02 rotationSpeed 0.015 kevinOffset 0.033 -0.0266 0.041 --file ..\..\..\..\..\..\rtredinnick\volumes\vis_human_color.vws
)

