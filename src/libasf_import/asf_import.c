#include <asf_contact.h>
#include <asf_license.h>
#include "asf_import.h"
#include "asf_meta.h"
#include "asf_nan.h"
#include "ceos.h"
#include "decoder.h"
#include "find_geotiff_name.h"
#include "get_ceos_names.h"
#include "get_stf_names.h"
#include "asf_raster.h"
#include <ctype.h>

int asf_import(radiometry_t radiometry, int db_flag, int complex_flag,
         int multilook_flag, int amp0_flag, input_format_t format_type, 
	 char *band_id, char *data_type, char *image_data_type, char *lutName, 
	 char *prcPath, double lowerLat, double upperLat,
         int line, int sample, int width, int height,
         double *p_range_scale, double *p_azimuth_scale,
         double *p_correct_y_pixel_size, char *inMetaNameOption,
         char *inBaseName, char *outBaseName)
{
  char outDataName[256], outMetaName[256];

  asfPrintStatus("Importing: %s\n", inBaseName);

  strcpy(outDataName, outBaseName);
  strcpy(outMetaName, outBaseName);
  strcat(outMetaName, TOOLS_META_EXT);

  if ((radiometry == r_SIGMA ||
       radiometry == r_BETA  ||
       radiometry == r_GAMMA ||
       radiometry == r_POWER)   &&
       !(format_type == CEOS || format_type == STF))
  {
    // A power flag is on, but the input file is not CEOS or STF format
    // so it will be ignored
    asfPrintWarning("Power flags %s%s for non-CEOS/non-STF datasets\n"
                    "will be ignored since other data formats do not indicate what\n"
                    "type of data is in the file.  Assuming the input data is an AMPLITUDE\n"
                    "image...\n",
                    radiometry == r_SIGMA ? "SIGMA" :
                    radiometry == r_BETA  ? "BETA"  :
                    radiometry == r_GAMMA ? "GAMMA" :
                    radiometry == r_POWER ? "POWER" : "UNKNOWN",
                    db_flag               ? " scaled to DECIBELS" : "");
  }

  // Ingest all sorts of flavors of CEOS data/
  if (format_type == CEOS) {
    asfPrintStatus("   Data format: CEOS\n");
    import_ceos(inBaseName, outBaseName, band_id, lutName,
                p_range_scale, p_azimuth_scale, p_correct_y_pixel_size,
                line, sample, width, height, inMetaNameOption, radiometry,
                db_flag, complex_flag, multilook_flag, amp0_flag);
  }
  // Ingest Vexcel Sky Telemetry Format (STF) data
  else if (format_type == STF) {
    asfPrintStatus("   Data format: STF\n");
    int lat_constrained = upperLat != -99 && lowerLat != -99;
    import_stf(inBaseName, outBaseName, radiometry, inMetaNameOption,
               lat_constrained, lowerLat, upperLat, prcPath);
  }
  else if (format_type == GENERIC_GEOTIFF) {
    asfPrintStatus("   Data format: GEOTIFF\n");
    if (band_id != NULL &&
      strlen(band_id) > 0 &&
      strncmp(uc(band_id), "ALL", 3) != 0) {
      asfPrintWarning("The -band option is not supported for data files containing\n"
          "multiple bands within a single file (such as GeoTIFF or TIFF)\n"
          "rather than in individual band files (such as ALOS etc).\n"
          "\nThe import will continue, but all available bands will be\n"
          "imported into a single ASF-format file.  You may select any\n"
          "individual band for export however.\n");
    }
    char *ext = findExt(inBaseName);
    if (ext != NULL) {
      *ext = '\0';
    }
    GString *inGeotiffName = find_geotiff_name (inBaseName);
    if ( inGeotiffName == NULL ) {
      asfPrintError ("Couldn't find a GeoTIFF file (i.e. a file with "
         "extension '.tif', '.tiff',\n'.TIF', or '.TIFF') "
         "corresponding to specified inBaseName:\n"
         "%s\n", inBaseName);
    }
    if (strlen(image_data_type)               &&
        strncmp(image_data_type, MAGIC_UNSET_STRING, strlen(MAGIC_UNSET_STRING)) != 0)
    {
      import_generic_geotiff (inGeotiffName->str, outBaseName, image_data_type);
    }
    else {
      import_generic_geotiff (inGeotiffName->str, outBaseName, NULL);
    }
  }
  else if (format_type == BIL) {
    asfPrintStatus("   Data format: BIL\n");
    import_bil(inBaseName, outBaseName);
  }
  else if (format_type == GRIDFLOAT) {
    asfPrintStatus("   Data format: GRIDFLOAT\n");
    import_gridfloat(inBaseName, outBaseName);
  }
  else if (format_type == AIRSAR) {
    asfPrintStatus("   Data format: AIRSAR\n");
    import_airsar(inBaseName, outBaseName);
  }
  else if (format_type == GAMMA_MSP) {
    if (inMetaNameOption && fileExists(inMetaNameOption) &&
	fileExists(inBaseName)) {
      asfPrintStatus("   Data format: GAMMA_MSP\n");
      import_gamma_msp(inBaseName, inMetaNameOption, data_type, 
		       image_data_type, outBaseName);
    }
    else
      asfPrintError("The GAMMA_MSP format requires the data file and metadata"
		    "with their respective extensions.\n");
  }
  else if (format_type == GAMMA_ISP) {
    if (inMetaNameOption && fileExists(inMetaNameOption) &&
	fileExists(inBaseName)) {
      asfPrintStatus("   Data format: GAMMA_ISP\n");
      import_gamma_isp(inBaseName, inMetaNameOption, data_type, 
		       image_data_type, complex_flag, multilook_flag, 
		       outBaseName);
    }
    else
      asfPrintError("The GAMMA_ISP format requires the data file and metadata"
                    "with their respective extensions.\n");
  }
  else if (format_type == VP) {
    asfPrintStatus("   Data format: VP\n");
    import_vexcel_plain(inBaseName, outBaseName);
  }
  // Don't recognize this data format; report & quit
  else {
    asfPrintError("Unrecognized data format: '%s'\n",format_type);
  }

  asfPrintStatus("Import complete.\n");
  return 0;
}
