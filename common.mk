# Makefile for registering info about the system we're compiling on.

UNAME=$(shell uname -r)
ifeq ('Darwin','$(UNAME)')
	F_MACOSX=yes
endif
ifeq ('Linux','$(UNAME)')
	F_LINUX=yes
endif

