include MF_global.mk

NAME := nil
CPP_VERSION := 17
COMMON_FLAGS := -std=c++$(CPP_VERSION)
COMPILE_FLAGS :=
LINK_FLAGS :=

COMMON_DEBUG_FLAGS ?=
COMPILE_DEBUG_FLAGS ?= -D DEBUG_PRINT -g
LINK_DEBUG_FLAGS ?=

# == Include this file before setting all options ==