include MF_global.mk

# Adapted from Jankatay's Makefile
MD_DEPS := lib/libmd/libmd.a lib/libmd/md5.h
MD_FLAGS := -L./lib/libmd/
MD_LIBS := -lmd

NAME := nil
CPP_VERSION := 17
COMMON_FLAGS := -std=c++$(CPP_VERSION)
COMPILE_FLAGS := -iquote./$(DIR_SRC) -iquote./$(DIR_LIB)
LINK_FLAGS := $(MD_FLAGS)
LIB_FLAGS := $(MD_LIBS)

DEBUG ?= false
ifeq ($(DEBUG), true)
	COMMON_DEBUG_FLAGS :=
	COMPILE_DEBUG_FLAGS := -D DEBUG_PRINT -g
	LINK_DEBUG_FLAGS :=
	LIB_DEBUG_FLAGS :=
endif

# == Include this file before setting all options ==