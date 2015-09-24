#!/usr/bin/env python
"""
@file    wix.py
@author  Michael Behrisch
@author  Jakob Erdmann
@date    2011
@version $Id$

Builds the installer based on the nightly zip.

SUMO, Simulation of Urban MObility; see http://sumo.dlr.de/
Copyright (C) 2011-2015 DLR (http://www.dlr.de/) and contributors

This file is part of SUMO.
SUMO is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 3 of the License, or
(at your option) any later version.
"""
import optparse
import subprocess
import zipfile
import os
import sys
import tempfile
import glob
import shutil

INPUT_DEFAULT = r"O:\Daten\Sumo\Nightly\sumo-msvc10Win32-svn.zip"
OUTPUT_DEFAULT = r"O:\Daten\Sumo\Nightly\sumo-msvc10Win32-svn.msi"
WIX_DEFAULT = "%sbin" % os.environ.get(
    "WIX", r"D:\Programme\Windows Installer XML v3.5\\")
WXS_DEFAULT = os.path.join(
    os.path.dirname(__file__), "..", "..", "build", "wix", "*.wxs")
LICENSE = os.path.join(
    os.path.dirname(__file__), "..", "..", "build", "wix", "License.rtf")


def buildFragment(wixBin, sourceDir, targetLabel, tmpDir, log=None):
    base = os.path.basename(sourceDir)
    subprocess.call([os.path.join(wixBin, "heat.exe"), "dir", sourceDir,
                     "-cg", base, "-gg", "-dr", targetLabel,
                     "-out", os.path.join(tmpDir, "Fragment.wxs")],
                    stdout=log, stderr=log)
    fragIn = open(os.path.join(tmpDir, "Fragment.wxs"))
    fragOut = open(os.path.join(tmpDir, base + "Fragment.wxs"), "w")
    for l in fragIn:
        fragOut.write(l.replace("SourceDir", sourceDir))
    fragOut.close()
    fragIn.close()
    return fragOut.name


def buildMSI(sourceZip=INPUT_DEFAULT, outFile=OUTPUT_DEFAULT,
             wixBin=WIX_DEFAULT, wxsPattern=WXS_DEFAULT,
             license=LICENSE, log=None):
    tmpDir = tempfile.mkdtemp()
    zipfile.ZipFile(sourceZip).extractall(tmpDir)
    sumoRoot = glob.glob(os.path.join(tmpDir, "sumo-*"))[0]
    fragments = [buildFragment(wixBin, os.path.join(sumoRoot, d), "INSTALLDIR", tmpDir) for d in ["data", "tools"]]
    for d in ["userdoc", "pydoc", "tutorial", "examples"]:
        fragments.append(
            buildFragment(wixBin, os.path.join(sumoRoot, "docs", d), "DOCDIR", tmpDir, log))
    for wxs in glob.glob(wxsPattern):
        with open(wxs) as wxsIn: 
            with open(os.path.join(tmpDir, os.path.basename(wxs)), "w") as wxsOut:
                for l in wxsIn:
                    l = l.replace("License.rtf", license)
                    dataDir = os.path.dirname(license)
                    for data in ["bannrbmp.bmp", "dlgbmp.bmp"]:
                        l = l.replace(data, os.path.join(dataDir, data))
                    wxsOut.write(
                        l.replace(r"O:\Daten\Sumo\Nightly", os.path.join(sumoRoot, "bin")))
        fragments.append(wxsOut.name)
    subprocess.call([os.path.join(wixBin, "candle.exe"),
                     "-o", tmpDir + "\\"] + fragments,
                    stdout=log, stderr=log)
    wixObj = [f.replace(".wxs", ".wixobj") for f in fragments]
    subprocess.call([os.path.join(wixBin, "light.exe"),
                     "-ext", "WixUIExtension", "-o", outFile] + wixObj,
                    stdout=log, stderr=log)
    shutil.rmtree(tmpDir, True)  # comment this out when debugging

if __name__ == "__main__":
    optParser = optparse.OptionParser()
    optParser.add_option("-n", "--nightly-zip", dest="nightlyZip",
                         default=INPUT_DEFAULT, help="full path to nightly zip")
    optParser.add_option("-o", "--output", default=OUTPUT_DEFAULT,
                         help="full path to output file")
    optParser.add_option(
        "-w", "--wix", default=WIX_DEFAULT, help="path to the wix binaries")
    optParser.add_option(
        "-x", "--wxs", default=WXS_DEFAULT, help="pattern for wxs templates")
    optParser.add_option(
        "-l", "--license", default=LICENSE, help="path to the license")
    (options, args) = optParser.parse_args()
    buildMSI(options.nightlyZip, options.output,
             options.wix, options.wxs, options.license)
