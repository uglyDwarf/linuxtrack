.PHONY : all_recursive clean

all : linuxtrack-wine.exe

all_recursive :
	make -C client
	make -C controller
	make -C dumper
	make -C tester

clean :
	make -C client clean
	make -C controller clean
	make -C dumper clean
	make -C tester clean
	rm -f *.exe* *.dll* data.txt

Tester.exe.so : tester/Tester.exe
	cp tester/Tester.exe* .

Controller.exe.so : controller/Controller.exe
	cp controller/Controller.exe* .

NPClient.dll : client/NPClient.dll.so
	cp client/NPClient.dll.so NPClient.dll

data.txt : dumper/data.txt
	cp dumper/data.txt .

linuxtrack-wine.exe : all_recursive Tester.exe.so Controller.exe.so NPClient.dll data.txt
	makensis ltr_wine.nsi


