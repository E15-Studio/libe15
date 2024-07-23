# config.mk
# configs and envs

BUILD_DIR := build
EXECUTION_LOG_DIR := logs
SOURCE_DIR := src
TOOLS_SRC_DIR := tools
TESTS_DIR := test
WORKING_DIR := $(abspath $(shell pwd))

AUTOCONF_PATH := $(BUILD_DIR)/generated-conf.h


DOCUMENT_DIR := $(BUILD_DIR)/doc/html
DOCUMENT_TARGET := $(DOCUMENT_DIR)/index.html

CC := gcc
CXX := g++

PYTHON3 := python

SHELL := sh

CFLAGS :=
LDLIBS := -lm

ifeq ($(OS), Windows_NT)
MKDIR := mkdir
else
MKDIR := mkdir -p
endif