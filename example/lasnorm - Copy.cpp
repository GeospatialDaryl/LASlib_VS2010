/*
===============================================================================

  FILE:  lasnorm.cpp
  
  CONTENTS:
	This code is templated from the 'lasexample.cpp'.  It uses a 32-bit IMG
	1 band, floating point raster to normalize a coincident LAS LiDAR file.
	
    
  PROGRAMMERS:
	template lasexample by:
		martin.isenburg@gmail.com
	lasnorm:
		daryl_van_dyke@fws.gov
	
  
  COPYRIGHT:
	lasexample copyright:
    (c) 2011, Martin Isenburg, LASSO - tools to catch reality

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
    Use of this template is detailed in the email provided in the readme.

  CHANGE HISTORY:
      28 May 2012 -- MI -- Final Version 
	  3 January 2011 -- MI -- created while too homesick to go to Salzburg with Silke
	  21 July 2012 -- DVD -- Final version of lasnorm
  
===============================================================================
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lasreader.hpp"
#include "laswriter.hpp"

//from GDAL -->
#include "gdal.h"
//#include "gdal_frmts.h"
#include "gdal_frmts.h"
//#include "squid.h"
#include "gdal_alg.h"
//#include "gdal_priv.h"
#include "cpl_string.h"
//#include "cpl_conv.h"
#include "ogr_spatialref.h"
#include "cpl_minixml.h"
#include <vector>
//<-- End GDAL

void usage(bool wait=false)
{
  fprintf(stderr,"LASNorm  -  normalize LAS files with a 32 bit, floating point IMG raster. \n");
  fprintf(stderr,"     BETA1     \n");
  fprintf(stderr,"usage:\n");
  fprintf(stderr,"lasnorm in_DEM.img in.las out.las\n");
//  fprintf(stderr,"lasnorm -i in.las -o out.las -verbose\n");
//  fprintf(stderr,"lasnorm -ilas -olas < in.las > out.las\n");
  fprintf(stderr,"lasnorm -h\n");
  fprintf(stderr,"\n");
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(1);
}

static void byebye(bool error=false, bool wait=false)
{
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(error);
}

static double taketime()
{
  return (double)(clock())/CLOCKS_PER_SEC;
}


int main(int argc, char *argv[])
{
  int i;
  bool verbose = false;
  double start_time = 0.0;

  LASreadOpener lasreadopener;
  LASwriteOpener laswriteopener;
  
  //start GDAL -->
  const char         *pszLocX = NULL, *pszLocY = NULL;
  const char         *pszSrcFilename = NULL;
  const char         *pszSourceSRS = NULL;
  std::vector<int>   anBandList;
  bool               bAsXML = false, bLIFOnly = false;
  bool               bQuiet = false, bValOnly = false;
  double			 adfGeoTransform[6];
  GDALDatasetH       *poDataset;
  
  GDALAllRegister();
  
  // end GDAL <--
  
  if (argc == 1)
  {
    fprintf(stderr,"lasnorm.exe is better run in the command line\n");
    char file_name[256];
    fprintf(stderr,"enter input file: "); fgets(file_name, 256, stdin);
    file_name[strlen(file_name)-1] = '\0';
    lasreadopener.set_file_name(file_name);
    fprintf(stderr,"enter output file: "); fgets(file_name, 256, stdin);
    file_name[strlen(file_name)-1] = '\0';
    laswriteopener.set_file_name(file_name);
  }
  else
  {
    lasreadopener.parse(argc, argv);
    laswriteopener.parse(argc, argv);
  }

  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] == '\0')
    {
      continue;
    }
    else if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-help") == 0)
    {
      usage();
    }
    else if (strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"-verbose") == 0)
    {
      verbose = true;
    }
	
	else if (i == argc - 3 && !lasreadopener.active() && !laswriteopener.active())
    {
	  fprintf(stdout,"Good DEM Name\n");
	  //fprintf(stdout,argv[i]);
	  //fprintf(stdout,"%i\n",i);
	  //fprintf(stdout,"%c\n",argv[i]);
	  //fprintf(stdout,"%c\n",argv[argc - 2]);
	  //fprintf(stdout,"%c\n",argv[argc - 1]);
      pszSrcFilename = (argv[i]);
	  
    }
    
	else if (i == argc - 2 && !lasreadopener.active() && !laswriteopener.active())
    {
      lasreadopener.set_file_name(argv[i]);
    }
    else if (i == argc - 1 && !lasreadopener.active() && !laswriteopener.active())
    {
      lasreadopener.set_file_name(argv[i]);
    }
    else if (i == argc - 1 && lasreadopener.active() && !laswriteopener.active())
    {
      laswriteopener.set_file_name(argv[i]);
    }
    else
    {
      fprintf(stderr, "ERROR: cannot understand argument '%s'\n", argv[i]);
      usage();
    }
  }

  if (verbose) start_time = taketime();

  // check input & output
  
  if (!lasreadopener.active())
  {
    fprintf(stderr,"ERROR: no input specified\n");
    usage(argc == 1);
  }

  if (!laswriteopener.active())
  {
    fprintf(stderr,"ERROR: no output specified\n");
    usage(argc == 1);
  }

  // open lasreader

  LASreader* lasreader = lasreadopener.open();
  if (lasreader == 0)
  {
    fprintf(stderr, "ERROR: could not open lasreader\n");
    byebye(argc==1);
  }

  // open laswriter

  LASwriter* laswriter = laswriteopener.open(&lasreader->header);
  if (laswriter == 0)
  {
    fprintf(stderr, "ERROR: could not open laswriter\n");
    byebye(argc==1);
  }

#ifdef _WIN32
  if (verbose) fprintf(stderr, "reading %I64d points from '%s' and writing them modified to '%s'.\n", lasreader->npoints, lasreadopener.get_file_name(), laswriteopener.get_file_name());
#else
  if (verbose) fprintf(stderr, "reading %lld points from '%s' and writing them modified to '%s'.\n", lasreader->npoints, lasreadopener.get_file_name(), laswriteopener.get_file_name());
#endif

  // loop over points and modify them
  //GDALAllRegister();
  
/* -------------------------------------------------------------------- */
/*      Open source file.                                               */
/* -------------------------------------------------------------------- */
  
  GDALDatasetH hSrcDS = NULL;
  
  hSrcDS = GDALOpen( pszSrcFilename, GA_ReadOnly );
  if( hSrcDS == NULL )
    exit( 1 );
  double xLL, yLL;
  double xDelta, yDelta;
  printf( "DEM size is %d, %d pixels\n",
            GDALGetRasterXSize( hSrcDS ), 
            GDALGetRasterYSize( hSrcDS ) );
			
  if( GDALGetGeoTransform( hSrcDS, adfGeoTransform ) == CE_None )
  {
    if( adfGeoTransform[2] == 0.0 && adfGeoTransform[4] == 0.0 )
    {
        printf( "DEM Origin = (%.15f,%.15f)\n",
                    adfGeoTransform[0], adfGeoTransform[3] );
		xLL = adfGeoTransform[0];
		yLL = adfGeoTransform[3];

        printf( "Pixel Size = (%.15f,%.15f) map units\n",
                    adfGeoTransform[1], adfGeoTransform[5] );
		xDelta = adfGeoTransform[1];
		yDelta = adfGeoTransform[5];
    }
    else
        printf( "GeoTransform =\n"
                    "  %.16g, %.16g, %.16g\n"
                    "  %.16g, %.16g, %.16g\n", 
                    adfGeoTransform[0],
                    adfGeoTransform[1],
                    adfGeoTransform[2],
                    adfGeoTransform[3],
                    adfGeoTransform[4],
                    adfGeoTransform[5] );
  }

  //fprintf(stdout,"Nice Try");
  //fprintf(stdout," SPOT <----- \n");
  
  // where there is a point to read
  while (lasreader->read_point())
  {
    
	// modify the point
	/* -------------------------------------------------------------------- */
	/*      Turn the location into a pixel and line location.               */
	/* -------------------------------------------------------------------- */
	int inputAvailable = 1;
	double dfGeoX;
	double dfGeoY;
	double dfGeoZ;
	// 
	// _______________________declare the input point coords here_(as Double)__________________
	//lasreader->point.z += 10;
	dfGeoX = lasreader->point.X * 0.01;
	dfGeoY = lasreader->point.Y * 0.01;
	dfGeoZ = lasreader->point.Z * 0.01;
	// dfGeoX = lasreader->point.x ;
	// dfGeoY = lasreader->point.y ;
	// dfGeoZ = lasreader->point.z ;
	//DEBUG
//	fprintf (stdout," X,Y :  %18.5f  %18.5f  %10.2f  \n", dfGeoX, dfGeoY, dfGeoZ);
	//__________________________________________________________________________________________

	int iPixel, iLine;


	//fprintf(stdout," SPOT <----- \n");
	//GDALRasterBandH hBand = GDALGetRasterBand( hSrcDS, anBandList[0] );
	GDALRasterBandH hBand = GDALGetRasterBand( hSrcDS, 1 );
	
	//  Start2 **********************************
	float *pafScanline;
        int   nXSize = poBand->GetXSize();

        pafScanline = (float *) CPLMalloc(sizeof(float)*nXSize);
        poBand->RasterIO( GF_Read, 0, 0, nXSize, 1, 
                          pafScanline, nXSize, 1, GDT_Float32, 
                          0, 0 );

	//   ****************************************
		
	if (hBand == NULL) 
		exit( 1 );
		
	/* -------------------------------------------------------------------- */
	double adfPixel[2];
	float adfPixel2[2];//[2]
	//CPLString osValue;

	//DEBUG
	//fprintf (stdout,"This val%.15g", adfPixel[0] );
	//osValue.Printf( "This val%.15g", adfPixel[0] );

	int bSuccess;
		
	double dfOffset = GDALGetRasterOffset( hBand, &bSuccess );
	double dfScale  = GDALGetRasterScale( hBand, &bSuccess );
	//fprintf(stdout, "%.15g  %.15g \n", dfOffset,dfScale);
    
	//iPixel = (int) floor( ( dfGeoX - xLL )/xDelta );
    //iLine  = (int) floor(( dfGeoY - yLL )/yDelta );
	
	double adfGeoTransform[6], adfInvGeoTransform[6];

	if( GDALGetGeoTransform( hSrcDS, adfGeoTransform ) != CE_None )
		exit( 1 );

	GDALInvGeoTransform( adfGeoTransform, adfInvGeoTransform );

	iPixel = (int) floor(
		adfInvGeoTransform[0] 
		+ adfInvGeoTransform[1] * dfGeoX
		+ adfInvGeoTransform[2] * dfGeoY );
		
	iLine = (int) floor(
		adfInvGeoTransform[3] 
		+ adfInvGeoTransform[4] * dfGeoX
		+ adfInvGeoTransform[5] * dfGeoY );	
		
	//fprintf(stdout,"%10i %10i \n",iPixel, iLine);
	
	if( GDALRasterIO( hBand, GF_Read, iPixel, iLine, 1, 1, 
					  adfPixel2, 1, 1, GDT_Float32, 0, 0) == CE_None )
	{
		fprintf(stdout, "This entered IF \n" );

		if( dfOffset != 0.0 || dfScale != 1.0 )
		{
			//adfPixel[0] = adfPixel[0] * dfScale + dfOffset;
			//adfPixel[1] = adfPixel[1] * dfScale + dfOffset;
			adfPixel[1] = adfPixel[1] * dfScale + dfOffset;
			//fprintf(stdout, "This val%10.3d", adfPixel );
			fprintf(stdout, "This val %18.7d \n", adfPixel[1] );
			fprintf(stdout, "%.15g", adfPixel[0]);
		}
		
		//fprintf(stdout, "BThis val%10.3d", adfPixel );
		//fprintf(stdout, "BThis val%18.7f \n", adfPixel[1] );
		//fprintf(stdout, "BThis val%18.7f \n", adfPixel2[0] );
		//fprintf(stdout, "%.15g  \n", adfPixel[0]);
	}
    //lasreader->point.point_source_ID = 1020;
    //lasreader->point.user_data = 42;
    //if (lasreader->point.classification == 12) lasreader->point.classification = 1;
	
	
	
	adfPixel[1] = adfPixel[1] * dfScale + dfOffset;
	fprintf(stdout, "This val %18.7d \n", adfPixel2[1] );
	//DEBUG
	fprintf (stdout," X,Y :  %18.5f  %18.5f  %10.2f %10.2f \n", dfGeoX, dfGeoY, dfGeoZ, adfPixel2[0]);
    //
	lasreader->point.Z = (dfGeoZ - adfPixel[0]) * 100.;
    //fprintf (stdout," X,Y :  %18.5f  %18.5f  %10.2f  \n", dfGeoX, dfGeoY, dfGeoZ - adfPixel2[0]);
    // write the modified point
    laswriter->write_point(&lasreader->point);
    // add it to the inventory
    laswriter->update_inventory(&lasreader->point);
    } 

  laswriter->update_header(&lasreader->header, TRUE);

  I64 total_bytes = laswriter->close();
  delete laswriter;

#ifdef _WIN32
  if (verbose) fprintf(stderr,"total time: %g sec %I64d bytes for %I64d points\n", taketime()-start_time, total_bytes, lasreader->p_count);
#else
  if (verbose) fprintf(stderr,"total time: %g sec %lld bytes for %lld points\n", taketime()-start_time, total_bytes, lasreader->p_count);
#endif

  lasreader->close();
  delete lasreader;

  return 0;
} 