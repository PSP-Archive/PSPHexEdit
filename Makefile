APP_NAME = PSPHexEdit
BUILD_PRX = 1
TARGET = hexedit
OBJS = cFile.o main.o

DAY = `date +%-d`
MONTH = `date +%-m`
YEAR = `date +%-y`
BUILD_INDEX = 0

CFLAGS = -O2 -G0 -Wall -DBUILD_DAY=$(DAY) -DBUILD_MONTH=$(MONTH) -DBUILD_YEAR=$(YEAR) -DBUILD_INDEX=$(BUILD_INDEX) -DAPP_NAME=\"$(APP_NAME)\"
CXXFLAGS = $(CFLAGS) -fno-exceptions -fno-rtti
ASFLAGS = $(CFLAGS)

LIBDIR =
LIBS = -lstdc++ -lpsppower -lpsprtc
LDFLAGS =

EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_TITLE = $(APP_NAME)

PSPSDK=$(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak