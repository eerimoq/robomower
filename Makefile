#
# @file Makefile
# @version 1.0
#
# @section License
# Copyright (C) 2014-2015, Erik Moqvist
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# This file is part of the Robomower project.
#

TESTS = $(addprefix robomower/tst/, filter movement robot watchdog)
APPS = $(addprefix app/, base_station robot)
TESTS_AND_APPS = $(TESTS) $(APPS)

all: $(TESTS_AND_APPS:%=%.all)

release: $(TESTS_AND_APPS:%=%.release)

clean: $(TESTS_AND_APPS:%=%.clean)

run: $(TESTS:%=%.run)

report:
	for t in $(TESTS) ; do $(MAKE) -C $(basename $$t) report ; echo ; done

test: run
	$(MAKE) report

size: $(TESTS_AND_APPS:%=%.size)

jenkins-coverage: $(TESTS:%=%.jc)

$(TESTS_AND_APPS:%=%.all):
	$(MAKE) -C $(basename $@) all

$(TESTS_AND_APPS:%=%.release):
	$(MAKE) -C $(basename $@) release

$(TESTS_AND_APPS:%=%.clean):
	$(MAKE) -C $(basename $@) clean

$(TESTS:%=%.run):
	$(MAKE) -C $(basename $@) run

$(TESTS:%=%.report):
	$(MAKE) -C $(basename $@) report

$(TESTS_AND_APPS:%=%.size):
	$(MAKE) -C $(basename $@) size

$(TESTS:%=%.jc):
	$(MAKE) -C $(basename $@) jenkins-coverage

help:
	@echo "--------------------------------------------------------------------------------"
	@echo "  target                      description"
	@echo "--------------------------------------------------------------------------------"
	@echo "  all                         compile and link the application"
	@echo "  clean                       remove all generated files and folders"
	@echo "  run                         run the application"
	@echo "  report                      print test report"
	@echo "  test                        run + report"
	@echo "  release                     compile with NDEBUG=yes and NPROFILE=yes"
	@echo "  size                        print executable size information"
	@echo "  help                        show this help"
	@echo
