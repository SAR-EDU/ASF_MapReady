#include "asf_geocode.h"
#include <asf_meta.h>
#include <assert.h>

static void
img_to_latlon(meta_parameters *meta,
              double line, double samp,
              double *lat, double *lon)
{
  int ret = meta_get_latLon(meta, line, samp, 0, lat, lon);
  if (ret != 0) {
    asfPrintError("meta_get_latLon error: line = %f, samp = %f\n",
                  line, samp);
  }
}


static double
bilinear_interp_fn(double y, double x,
                   double p00, double p10, double p01, double p11)
{
  double x1 = 1-x;
  double y1 = 1-y;

  return p00*x1*y1 + p10*x*y1 + p01*x1*y + p11*x*y;
}

static void xy_interp(int line, int samp,
                      int line_lo, int line_hi, int samp_lo, int samp_hi,
                      double *lats, double *lons,
                      double *interp_lat, double *interp_lon)
{
  assert(samp>=samp_lo && samp<=samp_hi);
  assert(line>=line_lo && line<=line_hi);

  double l = (double)(line - line_lo) / (double)(line_hi - line_lo);
  double s = (double)(samp - samp_lo) / (double)(samp_hi - samp_lo);

  *interp_lat = bilinear_interp_fn(l, s, lats[0], lats[1], lats[2], lats[3]);
  *interp_lon = bilinear_interp_fn(l, s, lons[0], lons[1], lons[2], lons[3]);
}

static void
get_interp_params(meta_parameters *meta,
                  int line_lo, int line_hi, int samp_lo, int samp_hi,
                  double *lats, double *lons)
{
  img_to_latlon(meta, line_lo, samp_lo, &lats[0], &lons[0]);
  img_to_latlon(meta, line_lo, samp_hi, &lats[1], &lons[1]);
  img_to_latlon(meta, line_hi, samp_lo, &lats[2], &lons[2]);
  img_to_latlon(meta, line_hi, samp_hi, &lats[3], &lons[3]);
}

static double
calc_err(meta_parameters *meta,
         int line_lo, int line_hi, int samp_lo, int samp_hi,
         double *lats, double *lons)
{
  int l = (line_lo + line_hi) * .5;
  int s = (samp_lo + samp_hi) * .5;

  double interp_lat, interp_lon, real_lat, real_lon;
  xy_interp(l, s, line_lo, line_hi, samp_lo, samp_hi, lats, lons,
            &interp_lat, &interp_lon);

  img_to_latlon(meta, l, s, &real_lat, &real_lon);

  asfPrintStatus("  Interp: (%f,%f)\n", interp_lat, interp_lon);
  asfPrintStatus("  Actual: (%f,%f)\n", real_lat, real_lon);

  return hypot(interp_lat - real_lat, interp_lon - real_lon);
}

static int
find_grid_size(meta_parameters *meta, int initial_size, double tolerance)
{
  // pick a square centered on the center of the image
  // we will start with the biggest square and compare the error at the center
  // point between using (1) bilinear interp and (2) the real mapping
  // if the error is larger than the tolerance, shrink square and try again
  int line = meta->general->line_count / 2;
  int samp = meta->general->sample_count / 2;
  int sz = initial_size;
  double lats[4], lons[4];

  while (1) {
    int sz2 = sz/2;
    int line_lo = line - sz2;
    int line_hi = line_lo + sz;
    int samp_lo = samp - sz2;
    int samp_hi = samp_lo + sz;
    get_interp_params(meta, line_lo, line_hi, samp_lo, samp_hi,
                      lats, lons);
    double err = calc_err(meta, line_lo, line_hi, samp_lo, samp_hi,
                          lats, lons);
    if (err < tolerance) {
      asfPrintStatus("Using square size %d (err %f < %f)\n", sz, err, tolerance);
      break;
    }
    asfPrintStatus("Square size %d, no good (err %f > %f)\n", sz, err, tolerance);
    sz = sz2;
    if (sz <= 2) {
      asfPrintStatus("Proceeding with grid size 1.\n");
      sz = 1;
      break;
    }
  }
  return sz;
}

int geoid_adjust(const char *input_filename, const char *output_filename)
{
  char *input_img = appendExt(input_filename, ".img");
  char *input_meta = appendExt(input_filename, ".meta");

  char *output_img = appendExt(output_filename, ".img");
  char *output_meta = appendExt(output_filename, ".meta");

  if (!fileExists(input_img))
    asfPrintError("File not found: %s\n", input_img);
  if (!fileExists(input_meta))
    asfPrintError("File not found: %s\n", input_meta);

  meta_parameters *meta = meta_read(input_meta);
  int nl = meta->general->line_count;
  int ns = meta->general->sample_count;
  int ii, jj;

  FILE *fpIn = FOPEN(input_img, "rb");
  FILE *fpOut = FOPEN(output_img, "wb");
  float *buf;

  // mode: 0=slow but accurate, 1=fast but interped
  int mode = 1;

  double avg = 0.0;
  int num=0;

  if (mode==0) {
    buf = MALLOC(sizeof(float)*ns);
    for (ii=0; ii<nl; ++ii) {
      get_float_line(fpIn, meta, ii, buf);
      for (jj=0; jj<ns; ++jj) {
        double lat, lon;
        meta_get_latLon(meta, ii, jj, 0, &lat, &lon);
        if (buf[jj] > -900 && buf[jj] != meta->general->no_data)
        {
          float ht = get_geoid_height(lat,lon);
          buf[jj] += ht;
          avg += ht;
          ++num;
        }
      }
      put_float_line(fpOut, meta, ii, buf);
      asfLineMeter(ii,nl);
    }
  }
  else {
    double tol = .0001;
    int size = find_grid_size(meta, 512, .1*tol);

    int test_mode = 1;
    buf = MALLOC(sizeof(float)*ns*size);

    // these are for tracking the quality of the bilinear interp
    // not used if test_mode is false
    int num_out_of_tol = 0;
    int num_checked = 0;
    int num_bad = 0;
    double max_err = 0;
    double avg_err = 0;

    for (ii=0; ii<nl; ii += size) {
      int line_lo = ii;
      int line_hi = ii + size;

      if (ii+size >= ns)
        size = nl-ii;

      get_float_lines(fpIn, meta, ii, size, buf);

      for (jj=0; jj<ns; jj += size) {
        double lats[4], lons[4];

        int samp_lo = jj;
        int samp_hi = jj + size;

        get_interp_params(meta, line_lo, line_hi, samp_lo, samp_hi, lats, lons);

        int iii, jjj;
        for (iii=0; iii<size; ++iii) {
          for (jjj=0; jjj<size && jj+jjj<ns; ++jjj) {
            int index = iii*ns + jj + jjj;
            assert(index < ns*size);

            double lat, lon;
            xy_interp(ii+iii, jj+jjj, line_lo, line_hi, samp_lo, samp_hi, lats, lons,
                      &lat, &lon);

            // random checking of the quality of our interpolations
            if (test_mode && iii%11==0 && jjj%13==0) {
              double real_lat, real_lon;
              img_to_latlon(meta, ii+iii, jj+jjj, &real_lat, &real_lon);
              double err = hypot(real_lat - lat, real_lon - lon);

              avg_err += err;
              if (err > max_err)
                max_err = err;

              if (err > .1*tol) {
                asfPrintStatus("Out of tolerance at %d,%d: (%f,%f) vs (%f,%f) -> %f\n",
                               ii+iii, jj+jjj, lat, lon, real_lat, real_lon,
                               err);
                ++num_out_of_tol;
              }
              if (err > tol) {
                asfPrintStatus("Error is larger than %f!\n", .01*tol);
                ++num_bad;
              }
              ++num_checked;
            }

            if (buf[index] > -900 && buf[index] != meta->general->no_data)
            {
              float ht = get_geoid_height(lat,lon);
              buf[index] += ht;
              avg += ht;
              ++num;
            }
          }  
        }
      }
      put_float_lines(fpOut, meta, ii, size, buf);
      asfPrintStatus("Completed %.1f%%  \r", 100.*ii/(double)nl);
    }
    asfPrintStatus("Completed 100%%   \n");
  }

  avg /= (double)(num);
  asfPrintStatus("Average correction: %f\n", avg);

  meta_write(meta, output_meta);
  meta_free(meta);

  FCLOSE(fpIn);
  FCLOSE(fpOut);

  FREE(buf);
  FREE(input_img);
  FREE(input_meta);
  FREE(output_img);
  FREE(output_meta);

  // success
  return 0;
}

