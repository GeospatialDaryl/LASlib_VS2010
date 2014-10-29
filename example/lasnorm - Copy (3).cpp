/*
===============================================================================

  FILE:  lasexample.cpp
  
  CONTENTS:
  
    This source code serves as an example how you can easily use laslib to
    write your own processing tools or how to import from and export to the
    LAS format or - its compressed, but identical twin - the LAZ format.

  PROGRAMMERS:
  
		daryl_van_dyke@fws.gov
	
	from template lasexample by:
		martin.isenburg@gmail.com
  
  COPYRIGHT:
  
    (c) 2011, Martin Isenburg, LASSO - tools to catch reality

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
      28 May 2012 -- Final Version 
	  3 January 2011 -- created while too homesick to go to Salzburg with Silke
  
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
#include "gdal_frmts.h"
//#include "gdal_frmts.h"
//#include "squid.h"
//#include "gdal_alg.h"
//#include "gdal_priv.h"
#include "cpl_string.h"
//#include "cpl_conv.h"
#include "ogr_spatialref.h"
#include "cpl_minixml.h"
#include <vector>
//<-- End GDAL

	struct  bands{
float  Val;
};
bands rBand;

void usage(bool wait=false)
{
  fprintf(stderr,"usage:\n");
  fprintf(stderr,"lastest in_DEM.img in.las out.las\n");
  fprintf(stderr,"lastest -i in.las -o out.las -verbose\n");
  fprintf(stderr,"lastest -ilas -olas < in.las > out.las\n");
  fprintf(stderr,"lastest -h\n");
  fprintf(stderr,"Pret-ty Good!\n");
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

  assert(CHAR_BIT * sizeof (float) == 32);
  
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
  GDALRasterBandH     *poBand;

  GDALAllRegister();
  
  // end GDAL <--
  
  if (argc == 1)
  {
    fprintf(stderr,"lastest.exe is better run in the command line\n");
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
	  fprintf(stdout,"%i\n",i);
	  fprintf(stdout,"%c\n",argv[i]);
	  fprintf(stdout,"%c\n",argv[argc - 2]);
	  fprintf(stdout,"%c\n",argv[argc - 1]);
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
  printf( "Size is %d, %d\n",
            GDALGetRasterXSize( hSrcDS ), 
            GDALGetRasterYSize( hSrcDS ) );
			
  if( GDALGetGeoTransform( hSrcDS, adfGeoTransform ) == CE_None )
  {
    if( adfGeoTransform[2] == 0.0 && adfGeoTransform[4] == 0.0 )
    {
        printf( "Origin = (%.15f,%.15f)\n",
                    adfGeoTransform[0], adfGeoTransform[3] );
		xLL = adfGeoTransform[0];
		yLL = adfGeoTransform[3];

        printf( "Pixel Size = (%.15f,%.15f)\n",
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
  
  fprintf(stdout,"header  /n");
  printf( "Origin = (%.15f,%.15f)\n",
                    adfGeoTransform[0], adfGeoTransform[3] );
  printf( "Pixel Size = (%.15f,%.15f)\n",
                    adfGeoTransform[1], adfGeoTransform[5] );
  printf( "GeoTransform =\n"
                    "  %.16g, %.16g, %.16g\n"
                    "  %.16g, %.16g, %.16g\n", 
					adfGeoTransform[0],
                    adfGeoTransform[1],
                    adfGeoTransform[2],
                    adfGeoTransform[3],
                    adfGeoTransform[4],
                    adfGeoTransform[5] );
 
  xDelta = adfGeoTransform[1];
  
    //  START Header Info
  GDALDriverH   hDriver;
  hDriver = GDALGetDatasetDriver( hSrcDS );
  printf( "Driver: %s/%s\n",
          GDALGetDriverShortName( hSrcDS ),
          GDALGetDriverLongName( hSrcDS ) );

  printf( "Size is %dx%dx%d\n",
          GDALGetRasterXSize( hSrcDS ), 
          GDALGetRasterYSize( hSrcDS ),
          GDALGetRasterCount( hSrcDS ) );

  if( GDALGetProjectionRef( hSrcDS ) != NULL )
      printf( "Projection is `%s'\n", GDALGetProjectionRef( hSrcDS ) );

  if( GDALGetGeoTransform( hSrcDS, adfGeoTransform ) == CE_None )
  {
       printf( "Origin = (%.6f,%.6f)\n",
              adfGeoTransform[0], adfGeoTransform[3] );

      printf( "Pixel Size = (%.6f,%.6f)\n",
              adfGeoTransform[1], adfGeoTransform[5] );
  }
	//  END Header Info

  /*
  adfGeoTransform[0] /* top left x 
  adfGeoTransform[1] /* w-e pixel resolution 
    adfGeoTransform[2] /* 0 
    adfGeoTransform[3] /* top left y 
    adfGeoTransform[4] /* 0 
    adfGeoTransform[5] /* n-s pixel resolution (negative value) 
  */

  fprintf(stdout,"Nice Try \n");
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
	fprintf (stdout,"\n ln332 \n las: X,Y :  %18.5f  %18.5f  %10.2f  \n",
		     dfGeoX, dfGeoY, dfGeoZ);
	//__________________________________________________________________________________________

	int iPixel, iLine;


	//fprintf(stdout," SPOT <----- \n");
	//GDALRasterBandH hBand = GDALGetRasterBand( hSrcDS, anBandList[0] );
	//fprintf (stdout, anBandList[0]);
	GDALRasterBandH hBand = GDALGetRasterBand( hSrcDS, 1 );
		
	if (hBand == NULL) 
		exit( 1 );
		
	/* -------------------------------------------------------------------- */
	//double adfPixel[2];
	float bufPixel[8];
	float adfPixel2[2];//[2]
	CPLString osValue;
	//osValue.Printf( "This val   %.15g   \n", adfPixel[0] );
	//fprintf(stdout, "This val  (fprintf)  %8.2f \n", adfPixel2[1] );
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

	int pX = int( (dfGeoX - adfGeoTransform[0] )/ adfGeoTransform[1] );
    int pY = int( (dfGeoY - adfGeoTransform[3] )/ adfGeoTransform[5] );

	/*
	iPixel = (int) floor(
		adfInvGeoTransform[0] 
		+ adfInvGeoTransform[1] * dfGeoX
		+ adfInvGeoTransform[2] * dfGeoY );
		
	iLine = (int) floor(
		adfInvGeoTransform[3] 
		+ adfInvGeoTransform[4] * dfGeoX
		+ adfInvGeoTransform[5] * dfGeoY );	
	*/
	fprintf(stdout,"iPixel, iLine:  %10i %10i \n",pX, pY);

	
	/*A1
	if( GDALRasterIO( hBand, GF_Read, iPixel, iLine, 1, 1, 
					  adfPixel, 1, 1, GDT_Float32, 0, 0) == CE_None )
	{

		if( dfOffset != 0.0 || dfScale != 1.0 )
		{
			//adfPixel[0] = adfPixel[0] * dfScale + dfOffset;
			//adfPixel[1] = adfPixel[1] * dfScale + dfOffset;
			adfPixel[1] = adfPixel[1] * dfScale + dfOffset;
			//fprintf(stdout, "This val%10.3d", adfPixel );
			fprintf(stdout, "This val%18.7d \n", adfPixel[1] );
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
	A2*/

	// Removed IF blocek from *A1 to A2*
	//GDALRasterIO( hBand, GF_Read, pX, pY, 1, 1, 
	//				  bufPixel, 1, 1, GDT_Float32, 0, 0) ;
	
	typedef std::vector<float> raster_data_t;
	raster_data_t scanline(1);

	//GDALDatasetRasterIO( hSrcDS, GF_Read, pX, pY, 1, 1, 
					 // bufPixel, 1, 1, GDT_Float32, 1, 1, 0) ;
	//int bandArray[] = 1;
	GDALDatasetRasterIO(hSrcDS,GF_Read,100,100,1,1,bufPixel,1,1,GDT_Float32, 1, NULL, 0, 0, 0);

	GDALRasterBandH  *poBand;
	int             nBlockXSize, nBlockYSize;
    int             bGotMin, bGotMax;
    double          adfMinMax[2];
	float			*bufferPixel;
	float			*pafScanline;
    GDALGetBlockSize( hBand, &nBlockXSize, &nBlockYSize );
	


    //  bands struct definition originally here 
	//rBand = RasterIO( GF_Read, pX, pY , 1, 1, &scanline[0], 1,
    //                  1, GDT_Float32, 0, 0 );

	//fprintf(stdout,"Bands : %5.2f \n", bufPixel);
    /*
	printf( "Block=%dx%d Type=%s, ColorInterp=%s\n",
            nBlockXSize, nBlockYSize,
            GDALGetDataTypeName(GDALGetRasterDataType(hBand)),
            GDALGetColorInterpretationName(
                GDALGetRasterColorInterpretation(hBand)) );
    
	    adfMinMax[0] = GDALGetRasterMinimum( hBand, &bGotMin );
        adfMinMax[1] = GDALGetRasterMaximum( hBand, &bGotMax );
        if( ! (bGotMin && bGotMax) )
            GDALComputeRasterMinMax( hBand, TRUE, adfMinMax );

        printf( "Min=%.3fd, Max=%.3f\n", adfMinMax[0], adfMinMax[1] );
        
        if( GDALGetOverviewCount(hBand) > 0 )
            printf( "Band has %d overviews.\n", GDALGetOverviewCount(hBand));

        if( GDALGetRasterColorTable( hBand ) != NULL )
            printf( "Band has a color table with %d entries.\n", 
                     GDALGetColorEntryCount(
                         GDALGetRasterColorTable( hBand ) ) );
						 */
	//

	//fprintf(stdout, "This dfScale, dfOutput %10.3d %10.3d \n", dfScale, dfOffset );
	//adfPixel[0] = adfPixel[0] * dfScale + dfOffset;
	//adfPixel[1] = adfPixel[1] * dfScale + dfOffset;
	//adfPixel[1] = adfPixel[1] * dfScale + dfOffset;
	fprintf(stdout, "This val bufPixel %10.3f \n", bufPixel );
	fprintf(stdout, "This val bp 0 1 2 %18.7f %18.7f %18.7f \n", bufPixel[0], bufPixel[1], bufPixel[2] );
	fprintf(stdout, "%.15g  \n" , bufPixel);

	//fprintf(stdout, "This val  (fprintf)  %8.2f  %8.2f  %8.2f  %8.2f \n", bufPixel[0] ,bufPixel[1] ,bufPixel2[0] ,bufPixel2[1] );
	
    lasreader->point.Z = (dfGeoZ - bufPixel[0]) * 100.;
    fprintf (stdout,"line447 X,Y,Z,demZ :  %18.2f  %18.2f  %10.2f %10.2f \n", dfGeoX, dfGeoY, dfGeoZ, bufPixel[0]);
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

