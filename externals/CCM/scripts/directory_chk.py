#! /usr/bin/env python
# -*- coding: utf-8 -*-

import os
import re
import sys
import optparse   # for command line parameters
import CCDFile

# current dir climate_dir is assumed to be the climate root dir
# first check if we have enough arguments
if (len(sys.argv) != 2):
	print "Syntax: directory_ck.py <climate director>"
	exit()

climate_dir = sys.argv[1]
print "Processing climate directory: " + climate_dir

# test if directory exists
if os.path.exists(climate_dir) == False:
	print "The given directory does not exist!"
	exit()

# list all CCD files

# Filter function for CCD-Files
def filterCCD(ccd_files):
	return ccd_files.endswith(".ccd")==True

all_files = os.listdir(climate_dir)
ccd_files = filter(filterCCD,all_files)

# Check that all files are present
needed_files = ("temperature.ccd", "relhum.ccd")#, "dirrad.ccd", "difrad.ccd", "skyrad.ccd", "winddir.ccd", "windvel.ccd", "precipitation.ccd")
for fname in needed_files:
	if ccd_files.count(fname) == 0:
		print "Climate file " + fname + " is not in the directory"
		exit()

# Check that we have the XML file
if os.path.exists(climate_dir + '/description.xml') == False:
	print "Missing description.xml"
	exit()

			
# Check all files indifidually
for fname in ccd_files:
	# create object of type CCDFile
	ccdfile = CCDFile()
	# read the data
	ccdfile.read(climate_dir, fname)
	# check header
	# check data
	