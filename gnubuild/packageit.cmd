set MOJODIR=..\mojo
set PLUGINNAME=lindsay_plugin

call buildit.cmd

copy %PLUGINNAME% %MOJODIR%\%PLUGINNAME%

echo filemode.755=lindsay_plugin > %MOJODIR%\package.properties
palm-package %MOJODIR%

