#
# This is a project Makefile. It is assumed the directory this Makefile resides in is a
# project subdirectory.
#

EXTRA_COMPONENT_DIRS += $(PROJECT_PATH)/components
PROJECT_NAME := hapsnap
include $(IDF_PATH)/make/project.mk
