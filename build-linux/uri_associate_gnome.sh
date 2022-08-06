#!/bin/sh

gconftool-2 -t string -s /desktop/gnome/url-handlers/principia/command "`pwd`/principia %s"
gconftool-2 -s /desktop/gnome/url-handlers/principia/needs_terminal false -t bool
gconftool-2 -t bool -s /desktop/gnome/url-handlers/principia/enabled true
