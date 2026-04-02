message:
	-@echo  Specify one of the following toolset as the target of make:
	-@echo    nmake vcc   (for Microsoft Visual C++ x64)
	-@echo    make gcc64  (for MinGW64 - x86_64 build)
	-@echo    make clang64(for LLVM Clang 64bit build)
	-@echo  Please make sure that the "make" program you are using is
	-@echo  the one from the toolset.
	-@echo  (GNU make for gcc64/clang64, nmake for vcc)

############################################################################

vcc:
	$(MAKE) -f Makefiles/vcc.mak
gcc64:
	$(MAKE) -f Makefiles/gcc64.mak -j2
clang64:
	$(MAKE) -f Makefiles/clang64.mak -j2
