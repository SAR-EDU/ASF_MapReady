/****************************************************************
FUNCTION NAME:  meta_get_*

DESCRIPTION:
   Extract relevant parameters from CEOS.
   These are the external entry points
for general routines.

RETURN VALUE:

SPECIAL CONSIDERATIONS:

PROGRAM HISTORY:
  1.0 - O. Lawlor.  9/10/98.  CEOS Independance.
  1.5 - P. Denny    08/02     Update to new metadate structs
  1.6 - P. Denny    01/03     Added meta_get_system
****************************************************************/
#include <assert.h>
#include "asf.h"
#include "asf_meta.h"

#ifndef SQR
# define SQR(x) ((x)*(x))
#endif

/*General Calls:*/

/*********************************************************
 * meta_get_system: Figures byte ordering system
 * straight copy of c_getsys "algorithm" for DDR */
char *meta_get_system(void)
{
#if defined(big_ieee)
    return "big_ieee";
#elif defined(lil_ieee)
    return "lil_ieee";
#elif defined(cray_float)
    return "cray_float";
#else
    return MAGIC_UNSET_STRING;
#endif
}

/* SAR calls */
/************************************************************
 * meta_get_time:
 * Convert a line, sample pair to a time, slant-range pair.
 * These only work for SR and GR images.  They apply the time
 * and slant range correction fudge factors. Returns seconds
 * and meters.*/
double meta_get_time(meta_parameters *meta,double yLine, double xSample)
{
  // No effort has been made to make this routine work with
  // pseudoprojected images.
  assert (meta->projection == NULL
      || meta->projection->type != LAT_LONG_PSEUDO_PROJECTION);

    /*Slant or ground range -- easy.*/
  if (meta->sar->image_type=='S' || meta->sar->image_type=='G') // ||
//        (meta->sar->image_type=='P' && meta->projection->type==SCANSAR_PROJECTION))
    {
        return (yLine+meta->general->start_line)*meta->sar->azimuth_time_per_pixel+
            meta->sar->time_shift;
    }
    /*Map projected -- not as easy.*/
    else if (meta->sar->image_type=='P' || meta->sar->image_type=='R') {
        double time,slant;
        meta_get_timeSlantDop(meta,yLine,xSample,&time,&slant,NULL);
        return time + meta->sar->time_shift;
    }
    else /*Unknown projection type.*/
    {
        printf("Error!  Unknown SAR image type '%c' passed to meta_get_time!\n",
            meta->sar->image_type);
        exit(1);
    }
    return 0.0;/*<- for whining compilers.*/
}
double meta_get_slant(meta_parameters *meta,double yLine, double xSample)
{
  // No effort has been made to make this routine work with
  // pseudoprojected images.
  assert (meta->projection == NULL
      || meta->projection->type != LAT_LONG_PSEUDO_PROJECTION);

    if (meta->sar->image_type=='S')/*Slant range is easy.*/
        return meta->sar->slant_range_first_pixel
            + (xSample+meta->general->start_sample) * meta->general->x_pixel_size
            + meta->sar->slant_shift;
    else if (meta->sar->image_type=='G')/*Ground range is tougher.*/
    {/*We figure out the ground angle, phi, for this pixel, then
    use that and the law of cosines to find the slant range.*/
        double er = meta_get_earth_radius(meta,yLine,xSample);
        double ht = meta_get_sat_height(meta,yLine,xSample);
        double minPhi = acos((SQR(ht)+SQR(er)
            - SQR(meta->sar->slant_range_first_pixel)) / (2.0*ht*er));
        double phi = minPhi+
            (xSample+meta->general->start_sample)*(meta->general->x_pixel_size / er);
        double slantRng = sqrt(SQR(ht)+SQR(er)-2.0*ht*er*cos(phi));
        return slantRng + meta->sar->slant_shift;
    }
    else if (meta->sar->image_type=='P' ||
         meta->sar->image_type=='R')
    {
        double time,slant;
        meta_get_timeSlantDop(meta,yLine,xSample,&time,&slant,NULL);
        return slant+meta->sar->slant_shift;
    }
    else /*Unknown image type.*/
    {
        printf("Error!  Unknown SAR image type '%c' passed to meta_get_slant!\n",
            meta->sar->image_type);
        exit(1);
    }
    return 0.0;/*<- for whining compilers.*/
}

/*******************************************************
 * meta_get_dop:
 * Converts a line, sample pair to the doppler value at
 * that location. Returns Hz. Only works for SR & GR. */
double meta_get_dop(meta_parameters *meta,double yLine, double xSample)
{
  assert (meta->sar && (meta->sar->image_type == 'S'
            || meta->sar->image_type == 'G'));

    yLine += meta->general->start_line;
    xSample += meta->general->start_sample;

    return meta->sar->range_doppler_coefficients[0]+
           meta->sar->range_doppler_coefficients[1]*xSample+
           meta->sar->range_doppler_coefficients[2]*xSample*xSample+
           meta->sar->azimuth_doppler_coefficients[1]*yLine+
           meta->sar->azimuth_doppler_coefficients[2]*yLine*yLine;
}

/*State Vector calls */
/**********************************************************
 * meta_get_stVec:
 * Return fixed-earth state vector for the given time.
 * Steps through state vector list; then interpolates the
 * right pair.*/
stateVector meta_get_stVec(meta_parameters *meta,double time)
{
  // No effort has been made to make this routine work with
  // pseudoprojected images.
  assert (meta->projection == NULL
      || meta->projection->type != LAT_LONG_PSEUDO_PROJECTION);

    int stVecNo;
    stateVector ret;
    if (meta->state_vectors==NULL)
    {
        printf( "* ERROR in meta library function meta_get_stVec:\n"
            "* Requested a state vector, but no state vectors exist in the meta file!\n");
        exit(EXIT_FAILURE);
    }
    if (meta->state_vectors->vector_count<2)
    {
        printf( "* ERROR in meta library function meta_get_stVec:\n"
            "* Only %d state vector%s exist in file!\n",
            meta->state_vectors->vector_count,
            (meta->state_vectors->vector_count != 1) ? "s" : "");
        exit(EXIT_FAILURE);
    }
    stVecNo=0;
    while (stVecNo < meta->state_vectors->vector_count - 2
        && meta->state_vectors->vecs[stVecNo+1].time<time)
        stVecNo++;
    interp_stVec(&meta->state_vectors->vecs[stVecNo].vec,
             meta->state_vectors->vecs[stVecNo].time,
            &meta->state_vectors->vecs[stVecNo+1].vec,
             meta->state_vectors->vecs[stVecNo+1].time,
            &ret,time);
    return ret;
}

/* Calculation calls */
/**********************************************************
 * meta_incid:  Returns the incidence angle
 * This is the angle measured by the target between straight
 * up and the satellite. Returns radians.*/
double meta_incid(meta_parameters *meta,double y,double x)
{
  // No effort has been made to make this routine work with
  // pseudoprojected images.
  assert (meta->projection == NULL
      || meta->projection->type != LAT_LONG_PSEUDO_PROJECTION);

  double sr = meta_get_slant(meta,y,x);

  if (meta->transform) {
    double R = sr/1000.;
    double R2=R*R;
    return
      meta->transform->incid_a[0] +
      meta->transform->incid_a[1] * R +
      meta->transform->incid_a[2] * R2 +
      meta->transform->incid_a[3] * R2 * R +
      meta->transform->incid_a[4] * R2 * R2 +
      meta->transform->incid_a[5] * R2 * R2 * R;

  } else {
    double er = meta_get_earth_radius(meta,y,x);
    double ht = meta_get_sat_height(meta,y,x);
    return PI-acos((SQR(sr) + SQR(er) - SQR(ht)) / (2.0*sr*er));
  }
}

/**********************************************************
 * meta_look: Return the look angle
 * This is the angle measured by the satellite between
 * earth's center and the target point x. Returns radians*/
double meta_look(meta_parameters *meta,double y,double x)
{
  // No effort has been made to make this routine work with
  // pseudoprojected images.
  assert (meta->projection == NULL
      || meta->projection->type != LAT_LONG_PSEUDO_PROJECTION);

    double sr = meta_get_slant(meta,y,x);
    double er = meta_get_earth_radius(meta,y,x);
    double ht = meta_get_sat_height(meta,y,x);
    return acos((SQR(sr) + SQR(ht) - SQR(er)) / (2.0*sr*ht));
}

/**********************************************************
 * slant_from_incide: Calculate the slant range, if we know
 * the incidence angle, the earth radius, and the satellite height
 * incid should be in radians */
double slant_from_incid(double incid,double er,double ht)
{
  double i=PI-incid;
  double D=SQR(ht) - SQR(er*sin(i));
  double sr=er*cos(i) + sqrt(D);

  // check degenerate cases
  if (D<0 || sr<0 || er*cos(i) - sqrt(D) > 0)
    asfPrintError("Satellite orbit is below the Earth's surface!\n");

  return sr;
}

/**********************************************************
 * look_from_incid: Calculate the look angle, if we know the
 * incidence angle, the earth radius, and the satellite height
 * incid should be in radians, returns radians. */
double look_from_incid(double incid,double er,double ht)
{
  double sr=slant_from_incid(incid,er,ht);
  return acos((SQR(sr) + SQR(ht) - SQR(er)) / (2.0*sr*ht));
}
