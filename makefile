# ----------------------------
# Makefile Options
# ----------------------------

NAME = CAD
ICON = icon.png
DESCRIPTION = "Simple CAD Program"
COMPRESSED = NO
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
