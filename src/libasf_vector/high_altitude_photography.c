#include "asf_vector.h"
#include "shapefil.h"
#include "asf_nan.h"
#include <assert.h>
#include <errno.h>
#include <ctype.h>

typedef struct {
  char filename[1024];
  char date_photography[64];
  char location[128];
  char mission_line[64];
  char roll[64];
  char frame[64];
  double center_lat;
  double center_lon;
  double near_start_lat;
  double near_start_lon;
  double far_start_lat;
  double far_start_lon;
  double near_end_lat;
  double near_end_lon;
  double far_end_lat;
  double far_end_lon;
  char comments[128];
  char *thumbnail;
  char *description;
  char *download;
  double size;
  char *download2;
  double size2;
} hap_t; // high altitude photography

void hap_init(hap_t *hap)
{
  strcpy(hap->date_photography, MAGIC_UNSET_STRING);
  strcpy(hap->location, MAGIC_UNSET_STRING);
  strcpy(hap->mission_line, MAGIC_UNSET_STRING);
  strcpy(hap->roll, MAGIC_UNSET_STRING);
  strcpy(hap->frame, MAGIC_UNSET_STRING);
  hap->center_lat = MAGIC_UNSET_DOUBLE;
  hap->center_lon = MAGIC_UNSET_DOUBLE;
  hap->near_start_lat = MAGIC_UNSET_DOUBLE;
  hap->near_start_lon = MAGIC_UNSET_DOUBLE;
  hap->far_start_lat = MAGIC_UNSET_DOUBLE;
  hap->far_start_lon = MAGIC_UNSET_DOUBLE;
  hap->near_end_lat = MAGIC_UNSET_DOUBLE;
  hap->near_end_lon = MAGIC_UNSET_DOUBLE;
  hap->far_end_lat = MAGIC_UNSET_DOUBLE;
  hap->far_end_lon = MAGIC_UNSET_DOUBLE;
  strcpy(hap->comments, MAGIC_UNSET_STRING);
  hap->thumbnail = NULL;
  hap->description = NULL;
  hap->download = NULL;
  hap->size = MAGIC_UNSET_DOUBLE;
  hap->download2 = NULL;
  hap->size2 = MAGIC_UNSET_DOUBLE;
}

void hap_free(hap_t *hap)
{
  if (hap->thumbnail) {
    FREE(hap->thumbnail);
    hap->thumbnail = NULL;
  }
  if (hap->description) {
    FREE(hap->description);
    hap->description = NULL;
  }
  if (hap->download) {
    FREE(hap->download);
    hap->download = NULL;
  }
  if (hap->download2) {
    FREE(hap->download2);
    hap->download2 = NULL;
  }
}

static void strip_end_whitesp(char *s)
{
    char *p = s + strlen(s) - 1;
    while (isspace(*p) && p>s)
        *p-- = '\0';
}

static void msg(const char *format, ...)
{
    char buf[1024];
    va_list ap;
    va_start(ap, format);
    vsprintf(buf, format, ap);
    va_end(ap);

    // in the future, we'll be putting this in a textview or something!!
    //GtkWidget *tv = get_widget_checked("messages_textview");
    //GtkTextBuffer *tb = gtk_text_view_get_buffer(GTK_TEXT_VIEW(tv));

    //GtkTextIter end;
    //gtk_text_buffer_get_end_iter(tb, &end);
    //gtk_text_buffer_insert(tb, &end, buf, -1);

    printf(buf);
}

static char *my_parse_string(char *p, char *s, int max_len)
{
    if (!p || *p == '\0') {
        strcpy(s, "");
        msg("  --> Unexpected end of string\n");
        return NULL;
    }

    // scan ahead to the comma, or end of string
    char *q = strchr(p, ',');
    if (q) {
      *q = '\0'; // temporarily...
      strncpy_safe(s, p, max_len);
      *q = ',';

      // point to beginning of next item
      return q+1;
    }
    else {
      strncpy_safe(s, p, max_len);

      // no more strings
      return NULL;
    }
}

static char *get_str(char *line, int column_num)
{
    int i;
    char *p = line;
    static char ret[256];

    for (i=0; i<=column_num; ++i)
      p = my_parse_string(p,ret,256);

    return ret;
}

static int get_int(char *line, int column_num)
{
    if (column_num >= 0) {
        char *s = get_str(line, column_num);
        if (s)
          return atoi(s);
        else
          return 0;
    }
    else {
        return 0;
    }
}

static double get_double(char *line, int column_num)
{
    if (column_num >= 0) {
        char *s = get_str(line, column_num);
        if (s)
          return atof(s);
        else
          return 0.0;
    } else
        return 0.0;
}

static double get_req_double(char *line, int column_num, int *ok)
{
    if (column_num >= 0) {
        char *str = get_str(line, column_num);
        if (str && strlen(str)>0) {
            *ok=TRUE;
            return atof(str);
        }
        else {
            *ok=FALSE;
            return 0.0;
        }
    }
    else {
        *ok=FALSE;
        return 0.0;
    }
}

//static char get_char(char *line, int column_num)
//{
//    char *str = get_str(line, column_num);
//    if (str && strlen(str)>0)
//        return str[0];
//    else
//        return '?';
//}

static int find_col(char *line, char *column_header)
{
    char *p = line;
    char val[256];
    int col=0;

    while (p) {
        p=my_parse_string(p,val,256);
        if (strncmp_case(val,column_header,strlen(column_header))==0)
          return col;
        ++col;
    }

    // column heading was not found
    return -1;
}

static void add_to_kml(FILE *fp, hap_t *hap, dbf_header_t *dbf, int nCols)
{
  int ii;
  char begin[10], end[10], line[1024];
  FILE *fpIn;

  // Print out according to configuration
  fprintf(fp, "<Placemark>\n");
  fprintf(fp, "  <description><![CDATA[\n");
  if (hap->thumbnail && hap->description)
    fprintf(fp, "<table width=\"600\">\n");
  else
    fprintf(fp, "<table width=\"350\">\n");
  if (hap->thumbnail || hap->description)
    fprintf(fp, "<tr>\n");
  if (hap->thumbnail)
    fprintf(fp, "<td><img src=\"%s\" width=\"256\" height=\"256\"></td>\n",
	    hap->thumbnail);
  if (hap->description) {
    fpIn = FOPEN(hap->description, "r");
    fgets(line, 1022, fpIn);
    fprintf(fp, "<td><strong>%s</strong><br>\n", line);
    while (fgets(line, 1022, fpIn) != NULL)
      fprintf(fp, "%s", line);
    fprintf(fp, "</td>\n");
  }
  if (hap->thumbnail || hap->description)
    fprintf(fp, "</tr>\n");
  if (hap->download) {
    fprintf(fp, "<br><br>/n<strong><a href=\"%s\">Download data (%.1lf MB)</a>"
	    "</strong><br>", hap->download, hap->size);
    if (hap->download2)
      fprintf(fp, "<strong><a href=\"%s\">Download data (%.1lf MB)</a>"
	      "</strong><br>", hap->download2, hap->size2);
    fprintf(fp, "</td></tr>\n");
  }
  if (hap->thumbnail && hap->description)
    fprintf(fp, "<tr colspan=\"2\"><td>\n");
  else
    fprintf(fp, "<tr><td>\n");
  fprintf(fp, "<!-- Format: HAP (generated by convert2vector "
          "(version %s)) -->\n", SVN_REV);
  for (ii=0; ii<nCols; ii++) {
    if (dbf[ii].visible == 0) {
      strcpy(begin, "<!--");
      strcpy(end, "-->\n");
    }
    else {
      strcpy(begin, "");
      strcpy(end, "\n");
    }
    if (strcmp(dbf[ii].header, "Date_Photography") == 0)
      fprintf(fp, "%s<strong>Date of Photography</strong>: %s <br>%s", 
	      begin, hap->date_photography, end);
    else if (strcmp(dbf[ii].header, "Location") == 0)
      fprintf(fp, "%s<strong>Location</strong>: %s <br>%s", 
	      begin, hap->location, end);
    else if (strcmp(dbf[ii].header, "Mission_Line") == 0)
      fprintf(fp, "%s<strong>Mission/Line</strong>: %s <br>%s",
	      begin, hap->mission_line, end);
    else if (strcmp(dbf[ii].header, "Roll") == 0)
      fprintf(fp, "%s<strong>Roll</strong>: %s <br>%s",
	      begin, hap->roll, end);
    else if (strcmp(dbf[ii].header, "Frame") == 0)
      fprintf(fp, "%s<strong>Frame</strong>: %s <br>%s",
	      begin, hap->frame, end);
    else if (strcmp(dbf[ii].header, "Near_Start_Lat") == 0)
      fprintf(fp, "%s<strong>Near start lat</strong>: %s <br>%s", 
	      begin, lf(hap->near_start_lat), end);
    else if (strcmp(dbf[ii].header, "Near_Start_Lon") == 0)
      fprintf(fp, "%s<strong>Near start lon</strong>: %s <br>%s", 
	      begin, lf(hap->near_start_lon), end);
    else if (strcmp(dbf[ii].header, "Far_Start_Lat") == 0)
      fprintf(fp, "%s<strong>Far start lat</strong>: %s <br>%s", 
	      begin, lf(hap->far_start_lat), end);
    else if (strcmp(dbf[ii].header, "Far_Start_Lon") == 0)
      fprintf(fp, "%s<strong>Far start lon</strong>: %s <br>%s", 
	      begin, lf(hap->far_start_lon), end);
    else if (strcmp(dbf[ii].header, "Near_End_Lat") == 0)
      fprintf(fp, "%s<strong>Near end lat</strong>: %s <br>%s", 
	      begin, lf(hap->near_end_lat), end);
    else if (strcmp(dbf[ii].header, "Near_End_Lon") == 0)
      fprintf(fp, "%s<strong>Near end lon</strong>: %s <br>%s", 
	      begin, lf(hap->near_end_lon), end);
    else if (strcmp(dbf[ii].header, "Far_End_Lat") == 0)
      fprintf(fp, "%s<strong>Far end lat</strong>: %s <br>%s", 
	      begin, lf(hap->far_end_lat), end);
    else if (strcmp(dbf[ii].header, "Far_End_Lon") == 0)
      fprintf(fp, "%s<strong>Far end lon</strong>: %s <br>%s", 
	      begin, lf(hap->far_end_lon), end);
    else if (strcmp(dbf[ii].header, "Comments") == 0)
      fprintf(fp, "%s<strong>Comments</strong>: %s <br>%s",
	      begin, hap->comments, end);
  }
  fprintf(fp, "</td></tr></table>\n");
  fprintf(fp, "  ]]></description>\n");
  fprintf(fp, "  <name>%s</name>\n", hap->filename);
  fprintf(fp, "  <LookAt>\n");
  fprintf(fp, "    <longitude>%.10f</longitude>\n", hap->center_lon);
  fprintf(fp, "    <latitude>%.10f</latitude>\n", hap->center_lat);
  fprintf(fp, "    <range>400000</range>\n");
  fprintf(fp, "  </LookAt>\n");
  fprintf(fp, "  <visibility>1</visibility>\n");
  fprintf(fp, "  <open>1</open>\n");

  write_kml_style_keys(fp);

  fprintf(fp, "  <Polygon>\n");
  fprintf(fp, "    <extrude>1</extrude>\n");
  fprintf(fp, "    <altitudeMode>absolute</altitudeMode>\n");
  fprintf(fp, "    <outerBoundaryIs>\n");
  fprintf(fp, "     <LinearRing>\n");
  fprintf(fp, "      <coordinates>\n");
  fprintf(fp, "       %.12f,%.12f,7000\n", 
	  hap->near_start_lon, hap->near_start_lat);
  fprintf(fp, "       %.12f,%.12f,7000\n", 
	  hap->far_start_lon, hap->far_start_lat);
  fprintf(fp, "       %.12f,%.12f,7000\n", 
	  hap->far_end_lon, hap->far_end_lat);
  fprintf(fp, "       %.12f,%.12f,7000\n", 
	  hap->near_end_lon, hap->near_end_lat);
  fprintf(fp, "       %.12f,%.12f,7000\n", 
	  hap->near_start_lon, hap->near_start_lat);
  fprintf(fp, "      </coordinates>\n");
  fprintf(fp, "     </LinearRing>\n");
  fprintf(fp, "    </outerBoundaryIs>\n");
  fprintf(fp, "  </Polygon>\n");
  fprintf(fp, "</Placemark>\n");
}

static int read_hap_line(char *header, int n,char *line, hap_t *hap)
{
  int ii, ok;
  char *test = (char *) MALLOC(sizeof(char)*255);
  for (ii=0; ii<n; ii++) {
    test = get_column(header, ii);
    if (strcmp(test, "Filename") == 0)
      strcpy(hap->filename, get_str(line, ii));
    else if (strcmp(test, "Date_Photography") == 0)
      strcpy(hap->date_photography, get_str(line, ii));
    else if (strcmp(test, "Location") == 0)
      strcpy(hap->location, get_str(line, ii));
    else if (strcmp(test, "Mission_Line") == 0)
      strcpy(hap->mission_line, get_str(line, ii));
    else if (strcmp(test, "Roll") == 0)
      strcpy(hap->roll, get_str(line, ii));
    else if (strcmp(test, "Frame") == 0)
      strcpy(hap->frame, get_str(line, ii));
    else if (strcmp(test, "Center_Lat") == 0)
      hap->center_lat = get_double(line, ii);
    else if (strcmp(test, "Center_Lon") == 0)
      hap->center_lon = get_double(line, ii);
    else if (strcmp(test, "Near_Start_Lat") == 0)
      hap->near_start_lat = get_req_double(line, ii, &ok);
    else if (strcmp(test, "Near_Start_Lon") == 0)
      hap->near_start_lon = get_req_double(line, ii, &ok);
    else if (strcmp(test, "Far_Start_Lat") == 0) 
      hap->far_start_lat = get_req_double(line, ii, &ok);
    else if (strcmp(test, "Far_Start_Lon") == 0)
      hap->far_start_lon = get_req_double(line, ii, &ok);
    else if (strcmp(test, "Near_End_Lat") == 0)
      hap->near_end_lat = get_req_double(line, ii, &ok);
    else if (strcmp(test, "Near_End_Lon") == 0)
      hap->near_end_lon = get_req_double(line, ii, &ok);
    else if (strcmp(test, "Far_End_Lat") == 0)
      hap->far_end_lat = get_req_double(line, ii, &ok);
    else if (strcmp(test, "Far_End_Lon") == 0)
      hap->far_end_lon = get_req_double(line, ii, &ok);
    else if (strcmp(test, "Comments") == 0)
      strcpy(hap->comments, get_str(line, ii));
    else if (strcmp(test, "Thumbnail") == 0) {
      hap->thumbnail = (char *) MALLOC(sizeof(char)*1024);
      strcpy(hap->thumbnail, get_str(line, ii));
    }
    else if (strcmp(test, "Description") == 0) {
      hap->description = (char *) MALLOC(sizeof(char)*1024);
      strcpy(hap->description, get_str(line, ii));
    }
    else if (strcmp(test, "Download") == 0) {
      hap->download = (char *) MALLOC(sizeof(char)*1024);
      strcpy(hap->download, get_str(line, ii));
    }
    else if (strcmp(test, "Size") == 0) {
      hap->size = get_double(line, ii);
    }
    else if (strcmp(test, "Download2") == 0) {
      hap->download2 = (char *) MALLOC(sizeof(char)*1024);
      strcpy(hap->download2, get_str(line, ii));
    }
    else if (strcmp(test, "Size2") == 0) {
      hap->size2 = get_double(line, ii);
    }
  }
  FREE(test);

  return ok;
}

// Check location information
static int check_hap_location(FILE *ifp, char **header_line, int *n)
{
  dbf_header_t *dbf;
  int ii, nCols;
  char *header = (char *) MALLOC(sizeof(char)*1024);
  fgets(header, 1024, ifp);
  strip_end_whitesp(header);
  int nColumns = get_number_columns(header);
  
  // Read configuration file
  read_header_config("HAP", &dbf, &nCols);
  
  // ensure we have the columns we need
  int name_col = find_col(header, "Filename");
  int near_start_lat_col = find_col(header, "Near_Start_Lat");
  int near_start_lon_col = find_col(header, "Near_Start_Lon");
  int far_start_lat_col = find_col(header, "Far_Start_Lat");
  int far_start_lon_col = find_col(header, "Far_Start_Lon");
  int near_end_lat_col = find_col(header, "Near_End_Lat");
  int near_end_lon_col = find_col(header, "Near_End_Lon");
  int far_end_lat_col = find_col(header, "Far_End_Lat");
  int far_end_lon_col = find_col(header, "Far_End_Lon");
  
  // Check whether all visible columns are actually available in the file
  for (ii=0; ii<nCols; ii++) {
    if (find_col(header, dbf[ii].header) < 0)
      dbf[ii].visible = FALSE;
  }
  
  int all_ok=TRUE;
  if (name_col < 0) {
    printf("Missing: Filename\n");
    all_ok=FALSE;
  }
  if (near_start_lat_col < 0) {
    printf("Missing: Near_Start_Lat\n");
    all_ok=FALSE;
  }
  if (near_start_lon_col < 0) {
    printf("Missing: Near_Start_Lon\n");
    all_ok=FALSE;
  }
  if (far_start_lat_col < 0) {
    printf("Missing: End_Start_Lat\n");
    all_ok=FALSE;
  }
  if (far_start_lon_col < 0) {
    printf("Missing: End_Start_Lon\n");
    all_ok=FALSE;
  }
  if (near_end_lat_col < 0) {
    printf("Missing: Near_End_Lat\n");
    all_ok=FALSE;
  }
  if (near_end_lon_col < 0) {
    printf("Missing: Near_End_Lon\n");
    all_ok=FALSE;
  }
  if (far_end_lat_col < 0) {
    printf("Missing: Far_End_Lat\n");
    all_ok=FALSE;
  }
  if (far_end_lon_col < 0) {
    printf("Missing: Far_End_Lon\n");
    all_ok=FALSE;
  }
  if (!all_ok) {
    printf("Required data columns missing, cannot process this file.\n");
    return 0;
  }
  *header_line = header;
  *n = nColumns;

  return 1;
}

// Convert hap to kml file
int hap2kml(char *in_file, char *out_file, int listFlag)
{
  hap_t hap;
  dbf_header_t *dbf;
  char *header;
  int nCols, nColumns;
  char line[1024];
  
  // Read configuration file
  read_header_config("HAP", &dbf, &nCols);

  FILE *ifp = FOPEN(in_file, "r");
  assert(ifp);
  check_hap_location(ifp, &header, &nColumns);

  FILE *ofp = FOPEN(out_file, "w");
  if (!ofp) {
    printf("Failed to open output file %s: %s\n", out_file, strerror(errno));
    return 0;
  }
  
  kml_header(ofp);
  
  while (fgets(line, 1022, ifp) != NULL) {
    strip_end_whitesp(line);
    
    // ensure all lines end with a comma, that way the final column
    // does not need special treatment
    line[strlen(line)+1] = '\0';
    line[strlen(line)] = ',';
    
    // now get the individual column values
    hap_init(&hap);
    if (read_hap_line(header, nColumns, line, &hap))
      add_to_kml(ofp, &hap, dbf, nCols);
    hap_free(&hap);
  }
  
  kml_footer(ofp);
  
  fclose(ifp);
  fclose(ofp);
  
  return 1;
}

void shape_hap_init(char *inFile, char *header)
{
  char *dbaseFile;
  DBFHandle dbase;
  SHPHandle shape;
  dbf_header_t *dbf;
  int ii, nCols, length=50;

  // Read configuration file
  read_header_config("HAP", &dbf, &nCols);

  // Open database for initialization
  dbaseFile = (char *) MALLOC(sizeof(char)*(strlen(inFile)+5));
  sprintf(dbaseFile, "%s.dbf", inFile);
  dbase = DBFCreate(dbaseFile);
  if (!dbase)
    asfPrintError("Could not create database file '%s'\n", dbaseFile);

  // Add fields to database
  for (ii=0; ii<nCols; ii++) {
    if (strcmp(dbf[ii].header, "Filename") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "FILENAME", FTString, length, 0) == -1)
        asfPrintError("Could not add FILENAME field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Date_Photography") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "DATE_PHOTO", FTString, length, 0) == -1)
        asfPrintError("Could not add DATE_PHOTO field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Location") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "LOCATION", FTString, length, 0) == -1)
        asfPrintError("Could not add LOCATION field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Mission_Line") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "MISSION", FTString, length, 0) == -1)
        asfPrintError("Could not add MISSION field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Roll") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "ROLL", FTString, length, 0) == -1)
        asfPrintError("Could not add ROLL field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Frame") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "FRAME", FTString, length, 0) == -1)
        asfPrintError("Could not add FRAME field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Center_Lat") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SCN_CLAT", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SCN_CLAT field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Center_Lon") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SCN_CLON", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SCN_CLON field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Near_Start_Lat") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SCN_LULAT", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SCN_LULAT field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Near_Start_Lon") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SCN_LULON", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SCN_LULON field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Far_Start_Lat") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SCN_RULAT", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SCN_RULAT field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Far_Start_Lon") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SCN_RULON", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SCN_RULON field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Near_End_Lat") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SCN_LDLAT", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SCN_LDLAT field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Near_End_Lon") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SCN_LDLON", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SCN_LDLON field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Far_End_Lat") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SCN_RDLAT", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SCN_RDLAT field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Far_End_Lon") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SCN_RDLON", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SCN_RDLON field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Comments") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "COMMENTS", FTString, length, 0) == -1)
        asfPrintError("Could not add COMMENTS field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Thumbnail") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "THUMBNAIL", FTString, length, 0) == -1)
        asfPrintError("Could not add THUMBNAIL field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Description") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "DESCRIPTION", FTString, length, 0) == -1)
        asfPrintError("Could not add DESCRIPTION field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Download") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "DOWNLOAD", FTString, length, 0) == -1)
        asfPrintError("Could not add DOWNLOAD field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Size") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SIZE", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SIZE field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Download2") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "DOWNLOAD2", FTString, length, 0) == -1)
        asfPrintError("Could not add DOWNLOAD2 field to database file\n");
    }
    else if (strcmp(dbf[ii].header, "Size2") == 0 && dbf[ii].visible) {
      if (DBFAddField(dbase, "SIZE2", FTDouble, 16, 7) == -1)
        asfPrintError("Could not add SIZE2 field to database file\n");
    }
  }

  // Close the database for initialization
  DBFClose(dbase);
  
  // Open shapefile for initialization
  shape = SHPCreate(inFile, SHPT_POLYGON);
  if (!shape)
    asfPrintError("Could not create shapefile '%s'\n", inFile);
  
  // Close shapefile for initialization
  SHPClose(shape);
  
  FREE(dbaseFile);

  return;
}

static void add_to_shape(DBFHandle dbase, SHPHandle shape, hap_t *hap,
			 dbf_header_t *dbf, int nCols, int n)
{
  int ii, field = 0;

  // Write fields into the database
  for (ii=0; ii<nCols; ii++) {
    if (strcmp(dbf[ii].header, "Filename") == 0 && dbf[ii].visible) {
      DBFWriteStringAttribute(dbase, n, field, hap->filename);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Date_Photography") == 0 && dbf[ii].visible) {
      DBFWriteStringAttribute(dbase, n, field, hap->date_photography);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Location") == 0 && dbf[ii].visible) {
      DBFWriteStringAttribute(dbase, n, field, hap->location);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Mission_Line") == 0 && dbf[ii].visible) {
      DBFWriteStringAttribute(dbase, n, field, hap->mission_line);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Roll") == 0 && dbf[ii].visible) {
      DBFWriteStringAttribute(dbase, n, field, hap->roll);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Frame") == 0 && dbf[ii].visible) {
      DBFWriteStringAttribute(dbase, n, field, hap->frame);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Center_Lat") == 0 && dbf[ii].visible) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->center_lat);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Center_Lon") == 0 && dbf[ii].visible) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->center_lon);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Near_start_lat") == 0 && dbf[ii].visible) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->near_start_lat);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Near_start_lon") == 0 && dbf[ii].visible) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->near_start_lon);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Far_Start_Lat") == 0 && dbf[ii].visible) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->far_start_lat);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Far_Start_Lon") == 0 && dbf[ii].visible) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->far_start_lon);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Near_End_Lat") == 0 && dbf[ii].visible) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->near_end_lat);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Near_End_Lon") == 0 && dbf[ii].visible) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->near_end_lon);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Far_End_Lat") == 0 && dbf[ii].visible) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->far_end_lat);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Far_End_Lon") == 0 && dbf[ii].visible) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->far_end_lon);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Comments") == 0 && dbf[ii].visible) {
      DBFWriteStringAttribute(dbase, n, field, hap->comments);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Thumbnail") == 0 && dbf[ii].visible &&
	     hap->thumbnail) {
      DBFWriteStringAttribute(dbase, n, field, hap->thumbnail);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Description") == 0 && dbf[ii].visible &&
	     hap->description) {
      DBFWriteStringAttribute(dbase, n, field, hap->description);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Download") == 0 && dbf[ii].visible &&
	     hap->download) {
      DBFWriteStringAttribute(dbase, n, field, hap->download);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Size") == 0 && dbf[ii].visible &&
	     hap->size) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->size);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Download2") == 0 && dbf[ii].visible &&
	     hap->download2) {
      DBFWriteStringAttribute(dbase, n, field, hap->download2);
      field++;
    }
    else if (strcmp(dbf[ii].header, "Size2") == 0 && dbf[ii].visible &&
	     hap->size2) {
      DBFWriteDoubleAttribute(dbase, n, field, hap->size2);
      field++;
    }
  }

  double lat[5], lon[5];
  lat[0] = lat[4] = hap->near_start_lat;
  lon[0] = lon[4] = hap->near_start_lon;
  lat[1] = hap->far_start_lat;
  lon[1] = hap->far_start_lon;
  lat[2] = hap->far_end_lat;
  lon[2] = hap->far_end_lon;
  lat[3] = hap->near_end_lat;
  lon[3] = hap->near_end_lon;  

  // Write shape object
  SHPObject *shapeObject=NULL;
  shapeObject = SHPCreateSimpleObject(SHPT_POLYGON, 5, lon, lat, NULL);
  SHPWriteObject(shape, -1, shapeObject);
  SHPDestroyObject(shapeObject);
}

int hap2shape(char *inFile, char *outFile, int listFlag)
{
  DBFHandle dbase;
  SHPHandle shape;
  hap_t hap;
  dbf_header_t *dbf;
  char *header, line[1024];
  int nCols, nColumns, ii=0;

  // Read configuration file
  read_header_config("HAP", &dbf, &nCols);

  // Read hap file
  FILE *ifp = FOPEN(inFile, "r");
  assert(ifp);
  check_hap_location(ifp, &header, &nColumns);

  // Initalize the database file
  shape_hap_init(outFile, header);
  open_shape(outFile, &dbase, &shape);

  while (fgets(line, 1022, ifp) != NULL) {
    strip_end_whitesp(line);

    // ensure all lines end with a comma, that way the final column
    // does not need special treatment
    int n = strlen(line);
    line[n+1] = '\0';
    line[n] = ',';

    // now get the individual column values
    hap_init(&hap);
    if (read_hap_line(header, nColumns, line, &hap)) {
      add_to_shape(dbase, shape, &hap, dbf, nCols, ii);
      ii++;
    }
    hap_free(&hap);
  }

  // Clean up
  close_shape(dbase, shape);
  write_esri_proj_file(outFile);

  FCLOSE(ifp);

  return 1;
}
