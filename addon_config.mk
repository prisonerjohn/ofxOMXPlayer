# All variables and this file are optional, if they are not present the PG and the
# makefiles will try to parse the correct values from the file system.
#
# Variables that specify exclusions can use % as a wildcard to specify that anything in
# that position will match. A partial path can also be specified to, for example, exclude
# a whole folder from the parsed paths from the file system
#
# Variables can be specified using = or +=
# = will clear the contents of that variable both specified from the file or the ones parsed
# from the file system
# += will add the values to the previous ones in the file or the ones parsed from the file
# system
#
# The PG can be used to detect errors in this file, just create a new project with this addon
# and the PG will write to the console the kind of error and in which line it is

meta:
	ADDON_NAME = ofxOMXPlayer
	ADDON_DESCRIPTION = Hardware accelerated video player for the Raspberry Pi
	ADDON_AUTHOR = Jason Van Cleave
	ADDON_TAGS = "raspberry pi, video player"
	ADDON_URL = https://github.com/jvcleave/ofxOMXPlayer

linuxarmv6l:
	FFMPEG_LIB_VERSION = linuxarmv6l

linuxarmv7l:
	FFMPEG_LIB_VERSION = linuxarmv7l
	ADDITIONAL_LIBS = -lbz2

common:
	FFMPEG_ROOT = libs/$(FFMPEG_LIB_VERSION)/ffmpeg

	ADDON_INCLUDES = src $(FFMPEG_ROOT)/include

	ADDON_CFLAGS = -fPIC -U_FORTIFY_SOURCE -Wall -ftree-vectorize -ftree-vectorize -Wno-deprecated-declarations -Wno-sign-compare -Wno-unknown-pragmas -Wno-unused-function -Wno-unused-but-set-variable

	FFMPEG_LIBS = $(FFMPEG_ROOT)/lib

	ADDON_LIBS = 
	ADDON_LIBS += $(FFMPEG_LIBS)/libavformat.a
	ADDON_LIBS += $(FFMPEG_LIBS)/libavcodec.a
	ADDON_LIBS += $(FFMPEG_LIBS)/libswscale.a
	ADDON_LIBS += $(FFMPEG_LIBS)/libavutil.a
	ADDON_LIBS += $(FFMPEG_LIBS)/libavfilter.a
	ADDON_LIBS += $(FFMPEG_LIBS)/libswresample.a

	#unused but available
	#ADDON_LIBS += $(FFMPEG_LIBS)/libpostproc.a
	#ADDON_LIBS += $(FFMPEG_LIBS)/libavdevice.a

	ADDON_LDFLAGS = -lm $(ADDITIONAL_LIBS)
