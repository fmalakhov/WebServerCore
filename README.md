# WebServerCore
General: Open SSL library is used and version for developers should be installed on server befor compilation.

***** ServSysLib - WEB server core library ********
For library compilation following environment variables should be set:
export SERVER_VOB=<path to repo>ServSysLib
export TARGET_OS=ubuntu_x86_64 (See available platforms in: <repo>ServSysLib/platform)

1. got to the <path to repo>ServSysLib directory.
2. Execute "make clean"
3. Execute "make"
4. Place ServSysLib_ubuntu_x86_64.lib to <path to repo>SampleWebServer/lib directory.

***** ServSysLib - Sample of WEB server **********
For sample server compilation following environment variables should be set:
export SERVER_VOB=<path to repo>SampleWebServer
export TARGET_OS=ubuntu_x86_64 (See available platforms in: <repo>SampleWebServer/platform)

1. got to the <path to repo>SampleWebServer directory.
2. Execute "make clean"
3. Execute "make"
4. Copy SampleWebServer to the test server directory.

For installation WEB server bundle create you should:
1. got to the <path to repo>SampleWebServer/install directory.
2  Set: export SERVER_VOB=<path to repo>ServSysLib
4. Run "./install_gen.sh"
5. Download package to an directory on test server and extract it.
6. Go to "database" directory and modify "custom_config.db" file.
	PrimLocalIPAddrServ = "<IP address of primary WEB access channel>"
	SecondLocalIPAddrServ = "<IP address of secondary WEB access channel>"
7. Run server: "sudo ./Sample..._x86_64"

In sample conviguration two channels WEB server will be running. Prinary channel is configured to port number 8000,
secondary channel is configured to port number 9000. Primary channel is supports access to content via HTTPS.
Secondary channel is supports access to content via HTTP.

There are 3 users already registered:
Server's administrator: login: admin  password: password
User Ivan:              login: ivan   password: ivan2017
Workshop kiosk:         login: kiosk  password: kiosk2017
