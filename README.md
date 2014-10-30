LASlib_VS2010
=============

A VS2010 ready subset of Martin's LAStools project.  Includes the LAS Normalizer LASNorm.

This project provides two elements:
    -  A general purpose configuration of rapidlasso's LASlib software (http://rapidlasso.com/) - configured for building on the MS Visual Studio 2010 platform, on a 64-bit architecture.
	-  A distribution and collaboration point for LASNorm, a utility for the normalization of LiDAR data.

History:
--------

	2014-10-29		DVD			Init Project, uploads.  Set up DoI intranet site (https://sites.google.com/a/fws.gov/lasnorm/home)
	2014-10-29		DVD			


	
LASNorm
-------

This tool needs a 64-bit version of the GDAL dll ('gdal_i.dll'), built for your system.  I will be adding the GDAL components soon, but you should be able to build and link on your own fairly easily.  Just remember to INCLUDE the GDAL incs, and set the LIB path to include the location of the DLL.

When ready, you can execute the program with:
	lasnorm input_DEM.img input_LAS.las output_LAS_norm.las

Currently, the debug (Verbose) is on.  I'll add a flag for this, and update for clarity soon.

Daryl_Van_Dyke@fws.gov