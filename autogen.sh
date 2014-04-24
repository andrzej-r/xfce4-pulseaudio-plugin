#!/bin/sh
#
# $Id: autogen.sh 2398 2007-01-17 17:47:38Z nick $
#
# Copyright (c) 2002-2007
#         The Xfce development team. All rights reserved.
#

(type xdt-autogen) >/dev/null 2>&1 || {
  cat >&2 <<EOF
autogen.sh: You don't seem to have the Xfce development tools installed on
            your system, which are required to build this software.
            Please install the xfce4-dev-tools package first, it is available
            from http://www.xfce.org/.
EOF
  exit 1
}

XDT_AUTOGEN_REQUIRED_VERSION="4.7.3" exec xdt-autogen $@
